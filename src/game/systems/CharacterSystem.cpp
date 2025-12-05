// ============================================================================
// CharacterSystem.cpp - Character entity management system implementation
// Created: December 3, 2025
// Location: src/game/systems/CharacterSystem.cpp
// ============================================================================

#include "game/systems/CharacterSystem.h"
#include "core/logging/Logger.h"
#include "game/realm/RealmManager.h"  // For RealmCreated event

#include <json/json.h>
#include <fstream>
#include <algorithm>

namespace game {
namespace character {

// ============================================================================
// Constructor / Destructor
// ============================================================================

CharacterSystem::CharacterSystem(
    core::ecs::ComponentAccessManager& componentAccess,
    core::threading::ThreadSafeMessageBus& messageBus
)
    : m_componentAccess(componentAccess)
    , m_messageBus(messageBus)
    , m_ageTimer(0.0f)
    , m_relationshipTimer(0.0f)
{
    // Subscribe to realm creation events to link rulers to realms
    //
    // NOTE: We use a shutdown flag instead of RAII SubscriptionHandle because:
    // - MessageBus::Subscribe() returns void (no subscription ID)
    // - MessageBus::Unsubscribe<T>() removes ALL handlers for type T
    // - Cannot unsubscribe individual handlers without MessageBus API changes
    // - Shutdown flag prevents use-after-free if events fire during destruction
    m_messageBus.Subscribe<game::realm::events::RealmCreated>(
        [this](const game::realm::events::RealmCreated& event) {
            // Early return if system is shutting down
            if (m_shuttingDown) return;

            OnRealmCreated(event.realmId, event.rulerId);
        }
    );

    CORE_STREAM_INFO("CharacterSystem") << "CharacterSystem initialized";
}

CharacterSystem::~CharacterSystem() {
    // Set shutdown flag FIRST to prevent event handlers from executing during destruction
    // This prevents use-after-free if events are still queued in the message bus
    m_shuttingDown = true;

    CORE_STREAM_INFO("CharacterSystem")
        << "CharacterSystem shutting down with " << m_allCharacters.size() << " characters";

    // Note: We cannot unsubscribe individual subscriptions because:
    // - MessageBus API doesn't provide subscription IDs/handles
    // - Unsubscribe<MessageType>() would affect ALL subscribers of that type
    // - Shutdown flag is the correct pattern given current API limitations
    //
    // Future improvement: Modify MessageBus::Subscribe() to return subscription ID
}

// ============================================================================
// Entity Creation and Management
// ============================================================================

core::ecs::EntityID CharacterSystem::CreateCharacter(
    const std::string& name,
    uint32_t age,
    const CharacterStats& stats
) {
    // 1. Validate input parameters
    if (name.empty()) {
        CORE_STREAM_ERROR("CharacterSystem")
            << "CreateCharacter failed: character name cannot be empty";
        return core::ecs::EntityID{};
    }

    if (name.length() > 64) {
        CORE_STREAM_ERROR("CharacterSystem")
            << "CreateCharacter failed: character name too long (" << name.length()
            << " chars, max 64): " << name;
        return core::ecs::EntityID{};
    }

    if (age > 120) {
        CORE_STREAM_ERROR("CharacterSystem")
            << "CreateCharacter failed: invalid age " << age << " for character '"
            << name << "' (max 120)";
        return core::ecs::EntityID{};
    }

    // Validate stats are within reasonable ranges (0-20 for attributes)
    if (stats.diplomacy > 20 || stats.martial > 20 || stats.stewardship > 20 ||
        stats.intrigue > 20 || stats.learning > 20) {
        CORE_STREAM_ERROR("CharacterSystem")
            << "CreateCharacter failed: stats out of range (0-20) for character '"
            << name << "' (dip=" << static_cast<int>(stats.diplomacy)
            << " mar=" << static_cast<int>(stats.martial)
            << " stw=" << static_cast<int>(stats.stewardship)
            << " int=" << static_cast<int>(stats.intrigue)
            << " lrn=" << static_cast<int>(stats.learning) << ")";
        return core::ecs::EntityID{};
    }

    if (stats.health < 0.0f || stats.health > 100.0f) {
        CORE_STREAM_ERROR("CharacterSystem")
            << "CreateCharacter failed: invalid health " << stats.health
            << " for character '" << name << "' (valid range: 0-100)";
        return core::ecs::EntityID{};
    }

    // 2. Get EntityManager reference
    auto* entity_manager = m_componentAccess.GetEntityManager();
    if (!entity_manager) {
        CORE_STREAM_ERROR("CharacterSystem")
            << "CreateCharacter failed: EntityManager is null (system not initialized)";
        return core::ecs::EntityID{};  // Default constructor: id=0, version=0 (invalid)
    }

    // 2. Create entity - returns versioned handle
    core::ecs::EntityID id = entity_manager->CreateEntity();
    if (!id.IsValid()) {
        CORE_STREAM_ERROR("CharacterSystem")
            << "Failed to create entity for character: " << name;
        return core::ecs::EntityID{};
    }

    // 3. Add CharacterComponent
    // CRITICAL: AddComponent creates the component and returns shared_ptr
    auto charComp = entity_manager->AddComponent<CharacterComponent>(id);
    if (charComp) {
        charComp->SetName(name);
        charComp->SetAge(age);
        charComp->SetDiplomacy(stats.diplomacy);
        charComp->SetMartial(stats.martial);
        charComp->SetStewardship(stats.stewardship);
        charComp->SetIntrigue(stats.intrigue);
        charComp->SetLearning(stats.learning);
        charComp->SetHealth(stats.health);
        charComp->SetPrestige(stats.prestige);
        charComp->SetGold(stats.gold);
    } else {
        CORE_STREAM_ERROR("CharacterSystem")
            << "Failed to add CharacterComponent for: " << name;
        entity_manager->DestroyEntity(id);
        return core::ecs::EntityID{};
    }

    // 4. Add supporting components
    // Each AddComponent returns shared_ptr - check if needed, then configure
    auto traitsComp = entity_manager->AddComponent<TraitsComponent>(id);

    auto relComp = entity_manager->AddComponent<CharacterRelationshipsComponent>(id);
    if (relComp) {
        relComp->character_id = id;  // Initialize character ID reference
    }

    auto eduComp = entity_manager->AddComponent<CharacterEducationComponent>(id);
    if (eduComp) {
        eduComp->character_id = id;
    }

    auto eventsComp = entity_manager->AddComponent<CharacterLifeEventsComponent>(id);
    if (eventsComp) {
        eventsComp->character_id = id;

        // Add birth event
        LifeEvent birthEvent = LifeEventGenerator::CreateBirthEvent(
            name, "Unknown", 0, 0);
        eventsComp->AddEvent(birthEvent);
    }

    auto artsComp = entity_manager->AddComponent<NobleArtsComponent>(id);

    // 5. Track character in lookup maps
    m_characterNames[id] = name;
    m_nameToEntity[name] = id;
    m_allCharacters.push_back(id);

    // Track legacy ID mapping (for RealmManager integration)
    // RealmManager uses types::EntityID (uint32_t), so we need bidirectional mapping
    m_legacyToVersioned[static_cast<game::types::EntityID>(id.id)] = id;

    // 6. Post creation event to message bus
    CharacterCreatedEvent event{id, name, age, false};
    m_messageBus.Publish(event);

    CORE_STREAM_INFO("CharacterSystem")
        << "Created character: " << name << " (age " << age
        << "), entity " << id.ToString();

    return id;
}

void CharacterSystem::DestroyCharacter(core::ecs::EntityID characterId) {
    auto* entity_manager = m_componentAccess.GetEntityManager();
    if (!entity_manager) {
        CORE_STREAM_ERROR("CharacterSystem") << "EntityManager is null!";
        return;
    }

    // Remove from tracking maps
    auto nameIt = m_characterNames.find(characterId);
    if (nameIt != m_characterNames.end()) {
        std::string name = nameIt->second;

        // Remove from name-to-entity map
        m_nameToEntity.erase(name);

        // Remove from character names map
        m_characterNames.erase(nameIt);

        // Remove from legacy ID mapping
        m_legacyToVersioned.erase(static_cast<game::types::EntityID>(characterId.id));

        // Remove from all characters vector
        auto vecIt = std::find(m_allCharacters.begin(), m_allCharacters.end(), characterId);
        if (vecIt != m_allCharacters.end()) {
            m_allCharacters.erase(vecIt);
        }

        CORE_STREAM_INFO("CharacterSystem")
            << "Destroyed character: " << name << " (entity " << characterId.ToString() << ")";
    }

    // Destroy entity
    entity_manager->DestroyEntity(characterId);
}

// ============================================================================
// Data Loading
// ============================================================================

bool CharacterSystem::LoadHistoricalCharacters(const std::string& json_path) {
    CORE_STREAM_INFO("CharacterSystem") << "Loading historical characters from: " << json_path;

    std::ifstream file(json_path);
    if (!file.is_open()) {
        CORE_STREAM_ERROR("CharacterSystem") << "Failed to open file: " << json_path;
        return false;
    }

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;

    if (!Json::parseFromStream(builder, file, &root, &errs)) {
        CORE_STREAM_ERROR("CharacterSystem") << "Failed to parse JSON: " << errs;
        return false;
    }

    file.close();

    // Parse characters array
    if (!root.isMember("characters") || !root["characters"].isArray()) {
        CORE_STREAM_ERROR("CharacterSystem")
            << "LoadHistoricalCharacters failed: JSON file '" << json_path
            << "' missing 'characters' array";
        return false;
    }

    const Json::Value& characters = root["characters"];
    size_t total_count = characters.size();
    size_t loaded_count = 0;
    size_t failed_count = 0;

    for (size_t i = 0; i < total_count; ++i) {
        const auto& charData = characters[static_cast<int>(i)];

        if (!charData.isObject()) {
            CORE_STREAM_WARN("CharacterSystem")
                << "Skipping character [" << i << "]: not a valid JSON object";
            failed_count++;
            continue;
        }

        // Extract character data
        std::string name = charData.get("name", "Unknown").asString();
        uint32_t age = charData.get("age", 30).asUInt();

        // Extract stats
        CharacterStats stats;
        if (charData.isMember("stats")) {
            const auto& statsObj = charData["stats"];
            stats.diplomacy = static_cast<uint8_t>(statsObj.get("diplomacy", 5).asInt());
            stats.martial = static_cast<uint8_t>(statsObj.get("martial", 5).asInt());
            stats.stewardship = static_cast<uint8_t>(statsObj.get("stewardship", 5).asInt());
            stats.intrigue = static_cast<uint8_t>(statsObj.get("intrigue", 5).asInt());
            stats.learning = static_cast<uint8_t>(statsObj.get("learning", 5).asInt());
            stats.health = statsObj.get("health", 100.0f).asFloat();
            stats.prestige = statsObj.get("prestige", 0.0f).asFloat();
            stats.gold = statsObj.get("gold", 0.0f).asFloat();
        } else {
            // Use default stats if not specified
            stats = CharacterStats::AverageNoble();
        }

        // Create character
        core::ecs::EntityID charId = CreateCharacter(name, age, stats);
        if (charId.IsValid()) {
            loaded_count++;

            // TODO: Load traits from JSON and add to TraitsComponent
            // TODO: Load relationships from JSON
            // TODO: Load life events from JSON
        } else {
            CORE_STREAM_ERROR("CharacterSystem")
                << "Failed to create character [" << i << "]: '" << name
                << "' (validation or entity creation failure - see previous error)";
            failed_count++;
        }
    }

    // Report final statistics
    if (failed_count > 0) {
        CORE_STREAM_WARN("CharacterSystem")
            << "LoadHistoricalCharacters completed with errors: "
            << loaded_count << " loaded, " << failed_count << " failed out of "
            << total_count << " total characters in " << json_path;
    } else {
        CORE_STREAM_INFO("CharacterSystem")
            << "Successfully loaded " << loaded_count << " historical characters from "
            << json_path;
    }

    return loaded_count > 0;
}

// ============================================================================
// Character Queries
// ============================================================================

core::ecs::EntityID CharacterSystem::GetCharacterByName(const std::string& name) const {
    auto it = m_nameToEntity.find(name);
    if (it != m_nameToEntity.end()) {
        return it->second;
    }
    return core::ecs::EntityID{};  // Invalid entity
}

const std::vector<core::ecs::EntityID>& CharacterSystem::GetAllCharacters() const {
    return m_allCharacters;
}

std::vector<core::ecs::EntityID> CharacterSystem::GetCharactersByRealm(core::ecs::EntityID realmId) const {
    std::vector<core::ecs::EntityID> result;

    // Convert realmId to legacy types::EntityID for comparison
    // RealmManager uses types::EntityID (uint32_t), so we extract the raw ID
    game::types::EntityID legacy_realm_id = static_cast<game::types::EntityID>(realmId.id);

    auto* entity_manager = m_componentAccess.GetEntityManager();
    if (!entity_manager) {
        CORE_STREAM_WARN("CharacterSystem")
            << "GetCharactersByRealm: EntityManager is null";
        return result;
    }

    // Scan all characters and filter by primary_title
    // NOTE: This is O(N) performance - consider indexing by realm for optimization
    for (const auto& charId : m_allCharacters) {
        auto charComp = entity_manager->GetComponent<CharacterComponent>(charId);
        if (charComp && charComp->GetPrimaryTitle() == legacy_realm_id) {
            result.push_back(charId);
        }
    }

    return result;
}

// ============================================================================
// System Update
// ============================================================================

void CharacterSystem::Update(float deltaTime) {
    // Age characters (once per in-game year)
    UpdateAging(deltaTime);

    // Update relationships (periodic decay/growth)
    UpdateRelationships(deltaTime);

    // Process active education
    UpdateEducation(deltaTime);

    // Remove expired temporary traits
    UpdateTraits(deltaTime);

    // Trigger pending life events
    UpdateLifeEvents(deltaTime);
}

// ============================================================================
// Integration Hooks
// ============================================================================

void CharacterSystem::OnRealmCreated(game::types::EntityID realmId, game::types::EntityID rulerId) {
    // Check if ruler exists (0 = no ruler)
    if (rulerId == 0) {
        return;
    }

    // Convert legacy types::EntityID to versioned core::ecs::EntityID
    core::ecs::EntityID versionedRulerId = LegacyToVersionedEntityID(rulerId);
    if (!versionedRulerId.IsValid()) {
        CORE_STREAM_WARN("CharacterSystem")
            << "OnRealmCreated: ruler ID " << rulerId << " not found in character system";
        return;
    }

    // Link character's primary title to this realm
    auto* entity_manager = m_componentAccess.GetEntityManager();
    if (entity_manager) {
        auto charComp = entity_manager->GetComponent<CharacterComponent>(versionedRulerId);
        if (charComp) {
            // RealmManager uses types::EntityID (uint32_t)
            charComp->SetPrimaryTitle(realmId);

            CORE_STREAM_INFO("CharacterSystem")
                << "Linked character " << charComp->GetName()
                << " as ruler of realm " << realmId;
        }
    }

    // Publish event for AI system to create AI actor
    CharacterNeedsAIEvent event{versionedRulerId, "", true, false};

    // Get character name if available
    auto nameIt = m_characterNames.find(versionedRulerId);
    if (nameIt != m_characterNames.end()) {
        event.name = nameIt->second;
    }

    m_messageBus.Publish(event);

    CORE_STREAM_INFO("CharacterSystem")
        << "Published CharacterNeedsAIEvent for ruler: " << versionedRulerId.ToString();
}

void CharacterSystem::OnCharacterDeath(core::ecs::EntityID characterId) {
    // TODO: Publish CharacterDiedEvent
    // TODO: Remove from tracking
    // TODO: Handle succession if character was a ruler
}

// ============================================================================
// Helper Functions (Private)
// ============================================================================

core::ecs::EntityID CharacterSystem::LegacyToVersionedEntityID(game::types::EntityID legacy_id) const {
    auto it = m_legacyToVersioned.find(legacy_id);
    if (it != m_legacyToVersioned.end()) {
        return it->second;
    }

    // Not found - return invalid EntityID
    return core::ecs::EntityID{};
}

// ============================================================================
// Update Subsystems (Private)
// ============================================================================

void CharacterSystem::UpdateAging(float deltaTime) {
    // Accumulate time
    m_ageTimer += deltaTime;

    // Age characters once per in-game year (simplified: 30 seconds = 1 year for now)
    const float YEAR_IN_SECONDS = 30.0f;

    if (m_ageTimer >= YEAR_IN_SECONDS) {
        m_ageTimer -= YEAR_IN_SECONDS;

        auto* entity_manager = m_componentAccess.GetEntityManager();
        if (!entity_manager) return;

        // Age all characters
        for (const auto& charId : m_allCharacters) {
            auto charComp = entity_manager->GetComponent<CharacterComponent>(charId);
            if (charComp) {
                uint32_t current_age = charComp->GetAge();
                charComp->SetAge(current_age + 1);

                // TODO: Check for death due to old age
                // TODO: Trigger coming-of-age events
            }
        }
    }
}

void CharacterSystem::UpdateEducation(float deltaTime) {
    // TODO: Process active education sessions
    // TODO: Grant XP based on tutor quality
    // TODO: Trigger education completion events
}

void CharacterSystem::UpdateRelationships(float deltaTime) {
    // Accumulate time
    m_relationshipTimer += deltaTime;

    // Update relationships periodically (every 30 seconds real-time)
    const float RELATIONSHIP_UPDATE_INTERVAL = 30.0f;

    if (m_relationshipTimer >= RELATIONSHIP_UPDATE_INTERVAL) {
        m_relationshipTimer -= RELATIONSHIP_UPDATE_INTERVAL;

        auto* entity_manager = m_componentAccess.GetEntityManager();
        if (!entity_manager) return;

        // Decay relationships for all characters
        for (const auto& charId : m_allCharacters) {
            auto relComp = entity_manager->GetComponent<CharacterRelationshipsComponent>(charId);
            if (relComp) {
                // TODO: Call relationship decay methods
                // relComp->DecayRelationships(deltaTime);
            }
        }
    }
}

void CharacterSystem::UpdateLifeEvents(float deltaTime) {
    // TODO: Process pending life events
    // TODO: Trigger appropriate events based on character state
}

void CharacterSystem::UpdateTraits(float deltaTime) {
    auto* entity_manager = m_componentAccess.GetEntityManager();
    if (!entity_manager) return;

    // Remove expired temporary traits
    for (const auto& charId : m_allCharacters) {
        auto traitsComp = entity_manager->GetComponent<TraitsComponent>(charId);
        if (traitsComp) {
            traitsComp->RemoveExpiredTraits();
        }
    }
}

// ============================================================================
// Serialization (Phase 6)
// ============================================================================

std::string CharacterSystem::GetSystemName() const {
    return "CharacterSystem";
}

Json::Value CharacterSystem::Serialize(int version) const {
    Json::Value data;
    data["system_name"] = "CharacterSystem";
    data["version"] = version;

    // Serialize update timers
    data["age_timer"] = m_ageTimer;
    data["relationship_timer"] = m_relationshipTimer;

    // Serialize all character EntityIDs with their versions
    Json::Value characters_array(Json::arrayValue);
    for (const auto& charId : m_allCharacters) {
        Json::Value char_entry;
        char_entry["id"] = Json::UInt64(charId.id);
        char_entry["version"] = Json::UInt64(charId.version);
        characters_array.append(char_entry);
    }
    data["all_characters"] = characters_array;

    // Serialize character names mapping
    Json::Value names_map(Json::objectValue);
    for (const auto& [entityId, name] : m_characterNames) {
        // Use "id_version" as key for JSON object
        std::string key = std::to_string(entityId.id) + "_" + std::to_string(entityId.version);
        names_map[key] = name;
    }
    data["character_names"] = names_map;

    // Serialize name-to-entity lookup
    Json::Value name_lookup(Json::objectValue);
    for (const auto& [name, entityId] : m_nameToEntity) {
        Json::Value entity_data;
        entity_data["id"] = Json::UInt64(entityId.id);
        entity_data["version"] = Json::UInt64(entityId.version);
        name_lookup[name] = entity_data;
    }
    data["name_to_entity"] = name_lookup;

    // Serialize legacy ID mapping
    Json::Value legacy_map(Json::objectValue);
    for (const auto& [legacy_id, versioned_id] : m_legacyToVersioned) {
        Json::Value versioned_data;
        versioned_data["id"] = Json::UInt64(versioned_id.id);
        versioned_data["version"] = Json::UInt64(versioned_id.version);
        legacy_map[std::to_string(legacy_id)] = versioned_data;
    }
    data["legacy_to_versioned"] = legacy_map;

    // Note: Individual character components (CharacterComponent, TraitsComponent, etc.)
    // are serialized by the ECS system separately. We only save the system-level state here.

    return data;
}

bool CharacterSystem::Deserialize(const Json::Value& data, int version) {
    if (!data.isObject()) {
        core::logging::Logger::Error("CharacterSystem::Deserialize - Invalid data format");
        return false;
    }

    if (data["system_name"].asString() != "CharacterSystem") {
        core::logging::Logger::Error("CharacterSystem::Deserialize - System name mismatch");
        return false;
    }

    // Clear existing state
    m_allCharacters.clear();
    m_characterNames.clear();
    m_nameToEntity.clear();
    m_legacyToVersioned.clear();

    // Deserialize update timers
    m_ageTimer = data["age_timer"].asFloat();
    m_relationshipTimer = data["relationship_timer"].asFloat();

    // Deserialize all character EntityIDs
    const Json::Value& characters_array = data["all_characters"];
    if (characters_array.isArray()) {
        for (const auto& char_entry : characters_array) {
            core::ecs::EntityID charId;
            charId.id = char_entry["id"].asUInt64();
            charId.version = char_entry["version"].asUInt64();
            m_allCharacters.push_back(charId);
        }
    }

    // Deserialize character names mapping
    const Json::Value& names_map = data["character_names"];
    if (names_map.isObject()) {
        for (const auto& key : names_map.getMemberNames()) {
            // Parse "id_version" key
            size_t underscore_pos = key.find('_');
            if (underscore_pos == std::string::npos) continue;

            uint64_t id = std::stoull(key.substr(0, underscore_pos));
            uint64_t ver = std::stoull(key.substr(underscore_pos + 1));

            core::ecs::EntityID entityId{id, ver};
            std::string name = names_map[key].asString();
            m_characterNames[entityId] = name;
        }
    }

    // Deserialize name-to-entity lookup
    const Json::Value& name_lookup = data["name_to_entity"];
    if (name_lookup.isObject()) {
        for (const auto& name : name_lookup.getMemberNames()) {
            const Json::Value& entity_data = name_lookup[name];
            core::ecs::EntityID entityId;
            entityId.id = entity_data["id"].asUInt64();
            entityId.version = entity_data["version"].asUInt64();
            m_nameToEntity[name] = entityId;
        }
    }

    // Deserialize legacy ID mapping
    const Json::Value& legacy_map = data["legacy_to_versioned"];
    if (legacy_map.isObject()) {
        for (const auto& key : legacy_map.getMemberNames()) {
            game::types::EntityID legacy_id = std::stoul(key);
            const Json::Value& versioned_data = legacy_map[key];

            core::ecs::EntityID versioned_id;
            versioned_id.id = versioned_data["id"].asUInt64();
            versioned_id.version = versioned_data["version"].asUInt64();

            m_legacyToVersioned[legacy_id] = versioned_id;
        }
    }

    core::logging::Logger::Info("CharacterSystem::Deserialize - Loaded " +
                               std::to_string(m_allCharacters.size()) + " characters");

    // Note: Individual character components are deserialized by the ECS system

    return true;
}

} // namespace character
} // namespace game
