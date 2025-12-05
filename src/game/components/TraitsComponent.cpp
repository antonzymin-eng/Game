// Created: November 19, 2025
// Location: src/game/components/TraitsComponent.cpp

#include "game/components/TraitsComponent.h"
#include "core/logging/Logger.h"
#include <algorithm>
#include <fstream>
#include <json/json.h>
#include <sstream>

namespace game {
namespace character {

// ============================================================================
// TraitsComponent Implementation
// ============================================================================

bool TraitsComponent::AddTrait(const std::string& trait_id, const Trait* trait_def) {
    // Check if already has this trait
    if (HasTrait(trait_id)) {
        return false;
    }

    // If trait definition provided, check for incompatibilities
    if (trait_def) {
        for (const auto& opposite : trait_def->opposite_traits) {
            if (HasTrait(opposite)) {
                CORE_STREAM_WARN("TraitsComponent")
                    << "Cannot add trait " << trait_id
                    << " - incompatible with " << opposite;
                return false;
            }
        }
    }

    // Add the trait
    active_traits.emplace_back(trait_id);
    cached_modifiers.needs_recalculation = true;

    CORE_STREAM_INFO("TraitsComponent") << "Added trait: " << trait_id;
    return true;
}

bool TraitsComponent::RemoveTrait(const std::string& trait_id) {
    auto it = std::find_if(active_traits.begin(), active_traits.end(),
        [&trait_id](const ActiveTrait& t) { return t.trait_id == trait_id; });

    if (it != active_traits.end()) {
        active_traits.erase(it);
        cached_modifiers.needs_recalculation = true;
        CORE_STREAM_INFO("TraitsComponent") << "Removed trait: " << trait_id;
        return true;
    }

    return false;
}

bool TraitsComponent::HasTrait(const std::string& trait_id) const {
    return std::any_of(active_traits.begin(), active_traits.end(),
        [&trait_id](const ActiveTrait& t) { return t.trait_id == trait_id; });
}

std::vector<std::string> TraitsComponent::GetTraitsByCategory(TraitCategory category) const {
    std::vector<std::string> result;
    const auto& db = TraitDatabase::Instance();

    for (const auto& active : active_traits) {
        const Trait* trait = db.GetTrait(active.trait_id);
        if (trait && trait->category == category) {
            result.push_back(active.trait_id);
        }
    }

    return result;
}

bool TraitsComponent::HasAnyTrait(const std::vector<std::string>& trait_ids) const {
    for (const auto& trait_id : trait_ids) {
        if (HasTrait(trait_id)) {
            return true;
        }
    }
    return false;
}

size_t TraitsComponent::GetTraitCount(TraitCategory category) const {
    return GetTraitsByCategory(category).size();
}

void TraitsComponent::AddTemporaryTrait(const std::string& trait_id,
                                       std::chrono::hours duration,
                                       const Trait* trait_def) {
    if (AddTrait(trait_id, trait_def)) {
        // Mark last added trait as temporary
        if (!active_traits.empty()) {
            auto& trait = active_traits.back();
            trait.is_temporary = true;
            trait.expiry_date = std::chrono::system_clock::now() + duration;
        }
    }
}

void TraitsComponent::RemoveExpiredTraits() {
    auto now = std::chrono::system_clock::now();
    bool removed_any = false;

    active_traits.erase(
        std::remove_if(active_traits.begin(), active_traits.end(),
            [&now, &removed_any](const ActiveTrait& t) {
                if (t.is_temporary && t.expiry_date < now) {
                    removed_any = true;
                    return true;
                }
                return false;
            }),
        active_traits.end()
    );

    if (removed_any) {
        cached_modifiers.needs_recalculation = true;
    }
}

void TraitsComponent::RecalculateModifiers(const std::unordered_map<std::string, Trait>& trait_database) {
    // Reset all modifiers
    cached_modifiers = TraitModifiers();

    // Sum up all trait effects
    for (const auto& active : active_traits) {
        auto it = trait_database.find(active.trait_id);
        if (it != trait_database.end()) {
            const Trait& trait = it->second;

            cached_modifiers.total_diplomacy += trait.diplomacy_modifier;
            cached_modifiers.total_martial += trait.martial_modifier;
            cached_modifiers.total_stewardship += trait.stewardship_modifier;
            cached_modifiers.total_intrigue += trait.intrigue_modifier;
            cached_modifiers.total_learning += trait.learning_modifier;

            cached_modifiers.total_ambition += trait.ambition_modifier;
            cached_modifiers.total_loyalty += trait.loyalty_modifier;
            cached_modifiers.total_honor += trait.honor_modifier;
            cached_modifiers.total_greed += trait.greed_modifier;
            cached_modifiers.total_boldness += trait.boldness_modifier;
            cached_modifiers.total_compassion += trait.compassion_modifier;

            cached_modifiers.total_health += trait.health_modifier;
            cached_modifiers.total_prestige += trait.prestige_modifier;
            cached_modifiers.total_fertility += trait.fertility_modifier;
            cached_modifiers.total_opinion += trait.opinion_modifier;
        }
    }

    cached_modifiers.needs_recalculation = false;
}

const TraitsComponent::TraitModifiers& TraitsComponent::GetModifiers(
    const std::unordered_map<std::string, Trait>& trait_database) {

    if (cached_modifiers.needs_recalculation) {
        RecalculateModifiers(trait_database);
    }

    return cached_modifiers;
}

// ============================================================================
// TraitDatabase Implementation
// ============================================================================

bool TraitDatabase::LoadTraits(const std::string& filepath) {
    // TODO: Implement JSON loading
    // For now, we rely on InitializeDefaultTraits()
    CORE_STREAM_INFO("TraitDatabase") << "Loading traits from: " << filepath;
    return true;
}

const Trait* TraitDatabase::GetTrait(const std::string& trait_id) const {
    auto it = m_traits.find(trait_id);
    return (it != m_traits.end()) ? &it->second : nullptr;
}

std::vector<const Trait*> TraitDatabase::GetTraitsByCategory(TraitCategory category) const {
    std::vector<const Trait*> result;
    for (const auto& [id, trait] : m_traits) {
        if (trait.category == category) {
            result.push_back(&trait);
        }
    }
    return result;
}

bool TraitDatabase::AreTraitsIncompatible(const std::string& trait1, const std::string& trait2) const {
    const Trait* t1 = GetTrait(trait1);
    if (t1) {
        return std::find(t1->opposite_traits.begin(), t1->opposite_traits.end(), trait2)
               != t1->opposite_traits.end();
    }
    return false;
}

void TraitDatabase::InitializeDefaultTraits() {
    // ========================================================================
    // Personality Traits
    // ========================================================================

    {
        Trait brave("brave", "Brave");
        brave.description = "This character is courageous and faces danger without fear";
        brave.category = TraitCategory::PERSONALITY;
        brave.martial_modifier = 2;
        brave.boldness_modifier = 0.2f;
        brave.opposite_traits = {"craven"};
        m_traits[brave.id] = brave;
    }

    {
        Trait craven("craven", "Craven");
        craven.description = "This character is cowardly and avoids danger";
        craven.category = TraitCategory::PERSONALITY;
        craven.martial_modifier = -2;
        craven.boldness_modifier = -0.3f;
        craven.opinion_modifier = -5.0f;
        craven.opposite_traits = {"brave"};
        m_traits[craven.id] = craven;
    }

    {
        Trait ambitious("ambitious", "Ambitious");
        ambitious.description = "This character seeks power and glory";
        ambitious.category = TraitCategory::PERSONALITY;
        ambitious.diplomacy_modifier = 1;
        ambitious.intrigue_modifier = 1;
        ambitious.ambition_modifier = 0.3f;
        ambitious.loyalty_modifier = -0.1f;
        ambitious.opposite_traits = {"content"};
        m_traits[ambitious.id] = ambitious;
    }

    {
        Trait content("content", "Content");
        content.description = "This character is satisfied with their position";
        content.category = TraitCategory::PERSONALITY;
        content.stewardship_modifier = 1;
        content.ambition_modifier = -0.3f;
        content.loyalty_modifier = 0.2f;
        content.opposite_traits = {"ambitious"};
        m_traits[content.id] = content;
    }

    {
        Trait cruel("cruel", "Cruel");
        cruel.description = "This character enjoys inflicting pain and suffering";
        cruel.category = TraitCategory::PERSONALITY;
        cruel.intrigue_modifier = 2;
        cruel.honor_modifier = -0.3f;
        cruel.compassion_modifier = -0.4f;
        cruel.opinion_modifier = -10.0f;
        cruel.opposite_traits = {"kind"};
        m_traits[cruel.id] = cruel;
    }

    {
        Trait kind("kind", "Kind");
        kind.description = "This character is compassionate and caring";
        kind.category = TraitCategory::PERSONALITY;
        kind.diplomacy_modifier = 2;
        kind.compassion_modifier = 0.4f;
        kind.opinion_modifier = 10.0f;
        kind.opposite_traits = {"cruel"};
        m_traits[kind.id] = kind;
    }

    {
        Trait greedy("greedy", "Greedy");
        greedy.description = "This character desires wealth above all";
        greedy.category = TraitCategory::PERSONALITY;
        greedy.stewardship_modifier = 1;
        greedy.greed_modifier = 0.4f;
        greedy.honor_modifier = -0.1f;
        greedy.opinion_modifier = -5.0f;
        greedy.opposite_traits = {"generous"};
        m_traits[greedy.id] = greedy;
    }

    {
        Trait generous("generous", "Generous");
        generous.description = "This character gives freely to others";
        generous.category = TraitCategory::PERSONALITY;
        generous.diplomacy_modifier = 2;
        generous.greed_modifier = -0.3f;
        generous.opinion_modifier = 15.0f;
        generous.opposite_traits = {"greedy"};
        m_traits[generous.id] = generous;
    }

    {
        Trait honest("honest", "Honest");
        honest.description = "This character values truth and honesty";
        honest.category = TraitCategory::PERSONALITY;
        honest.intrigue_modifier = -2;
        honest.honor_modifier = 0.3f;
        honest.opinion_modifier = 5.0f;
        honest.opposite_traits = {"deceitful"};
        m_traits[honest.id] = honest;
    }

    {
        Trait deceitful("deceitful", "Deceitful");
        deceitful.description = "This character is skilled at deception";
        deceitful.category = TraitCategory::PERSONALITY;
        deceitful.intrigue_modifier = 3;
        deceitful.honor_modifier = -0.3f;
        deceitful.opinion_modifier = -10.0f;
        deceitful.opposite_traits = {"honest"};
        m_traits[deceitful.id] = deceitful;
    }

    // ========================================================================
    // Education Traits
    // ========================================================================

    {
        Trait genius("genius", "Genius");
        genius.description = "This character has exceptional intelligence";
        genius.category = TraitCategory::EDUCATION;
        genius.diplomacy_modifier = 3;
        genius.martial_modifier = 2;
        genius.stewardship_modifier = 3;
        genius.intrigue_modifier = 3;
        genius.learning_modifier = 5;
        genius.opinion_modifier = 10.0f;
        genius.is_genetic = true;
        genius.is_congenital = true;
        genius.opposite_traits = {"quick", "slow", "imbecile"};
        m_traits[genius.id] = genius;
    }

    {
        Trait quick("quick", "Quick");
        quick.description = "This character learns quickly";
        quick.category = TraitCategory::EDUCATION;
        quick.diplomacy_modifier = 1;
        quick.martial_modifier = 1;
        quick.stewardship_modifier = 1;
        quick.intrigue_modifier = 1;
        quick.learning_modifier = 2;
        quick.is_genetic = true;
        quick.is_congenital = true;
        quick.opposite_traits = {"genius", "slow", "imbecile"};
        m_traits[quick.id] = quick;
    }

    {
        Trait slow("slow", "Slow");
        slow.description = "This character learns slowly";
        slow.category = TraitCategory::EDUCATION;
        slow.diplomacy_modifier = -1;
        slow.martial_modifier = -1;
        slow.stewardship_modifier = -1;
        slow.intrigue_modifier = -1;
        slow.learning_modifier = -2;
        slow.opinion_modifier = -5.0f;
        slow.is_genetic = true;
        slow.is_congenital = true;
        slow.opposite_traits = {"genius", "quick", "imbecile"};
        m_traits[slow.id] = slow;
    }

    {
        Trait scholarly("scholarly", "Scholarly");
        scholarly.description = "This character is well-educated and learned";
        scholarly.category = TraitCategory::EDUCATION;
        scholarly.learning_modifier = 3;
        scholarly.stewardship_modifier = 1;
        scholarly.opposite_traits = {"illiterate"};
        m_traits[scholarly.id] = scholarly;
    }

    // ========================================================================
    // Physical Traits
    // ========================================================================

    {
        Trait strong("strong", "Strong");
        strong.description = "This character has exceptional physical strength";
        strong.category = TraitCategory::PHYSICAL;
        strong.martial_modifier = 2;
        strong.health_modifier = 10.0f;
        strong.is_genetic = true;
        strong.opposite_traits = {"weak"};
        m_traits[strong.id] = strong;
    }

    {
        Trait weak("weak", "Weak");
        weak.description = "This character is physically frail";
        weak.category = TraitCategory::PHYSICAL;
        weak.martial_modifier = -1;
        weak.health_modifier = -10.0f;
        weak.is_genetic = true;
        weak.opposite_traits = {"strong"};
        m_traits[weak.id] = weak;
    }

    {
        Trait attractive("attractive", "Attractive");
        attractive.description = "This character is physically appealing";
        attractive.category = TraitCategory::PHYSICAL;
        attractive.diplomacy_modifier = 1;
        attractive.opinion_modifier = 10.0f;
        attractive.fertility_modifier = 0.1f;
        attractive.is_genetic = true;
        attractive.opposite_traits = {"ugly"};
        m_traits[attractive.id] = attractive;
    }

    {
        Trait ugly("ugly", "Ugly");
        ugly.description = "This character is physically unappealing";
        ugly.category = TraitCategory::PHYSICAL;
        ugly.diplomacy_modifier = -1;
        ugly.opinion_modifier = -10.0f;
        ugly.fertility_modifier = -0.1f;
        ugly.is_genetic = true;
        ugly.opposite_traits = {"attractive"};
        m_traits[ugly.id] = ugly;
    }

    // ========================================================================
    // Health Traits
    // ========================================================================

    {
        Trait wounded("wounded", "Wounded");
        wounded.description = "This character has been wounded in battle";
        wounded.category = TraitCategory::HEALTH;
        wounded.martial_modifier = -1;
        wounded.health_modifier = -20.0f;
        m_traits[wounded.id] = wounded;
    }

    {
        Trait maimed("maimed", "Maimed");
        maimed.description = "This character has been permanently crippled";
        maimed.category = TraitCategory::HEALTH;
        maimed.martial_modifier = -3;
        maimed.health_modifier = -30.0f;
        maimed.opinion_modifier = -5.0f;
        maimed.is_incurable = true;
        m_traits[maimed.id] = maimed;
    }

    {
        Trait ill("ill", "Ill");
        ill.description = "This character is currently ill";
        ill.category = TraitCategory::HEALTH;
        ill.health_modifier = -15.0f;
        m_traits[ill.id] = ill;
    }

    {
        Trait scarred("scarred", "Scarred");
        scarred.description = "This character bears visible scars";
        scarred.category = TraitCategory::HEALTH;
        scarred.opinion_modifier = -3.0f;
        scarred.is_incurable = true;
        m_traits[scarred.id] = scarred;
    }

    // ========================================================================
    // Fame Traits
    // ========================================================================

    {
        Trait famous("famous", "Famous");
        famous.description = "This character is well-known throughout the land";
        famous.category = TraitCategory::FAME;
        famous.prestige_modifier = 0.5f;
        famous.opinion_modifier = 10.0f;
        m_traits[famous.id] = famous;
    }

    {
        Trait renowned("renowned", "Renowned");
        renowned.description = "This character's deeds are legendary";
        renowned.category = TraitCategory::FAME;
        renowned.prestige_modifier = 1.0f;
        renowned.opinion_modifier = 20.0f;
        m_traits[renowned.id] = renowned;
    }

    // ========================================================================
    // Religious Traits
    // ========================================================================

    {
        Trait pious("pious", "Pious");
        pious.description = "This character is deeply religious";
        pious.category = TraitCategory::RELIGIOUS;
        pious.learning_modifier = 1;
        pious.honor_modifier = 0.2f;
        pious.opposite_traits = {"cynical"};
        m_traits[pious.id] = pious;
    }

    {
        Trait zealous("zealous", "Zealous");
        zealous.description = "This character is fanatically religious";
        zealous.category = TraitCategory::RELIGIOUS;
        zealous.martial_modifier = 2;
        zealous.learning_modifier = -1;
        zealous.boldness_modifier = 0.2f;
        zealous.compassion_modifier = -0.2f;
        zealous.opposite_traits = {"cynical"};
        m_traits[zealous.id] = zealous;
    }

    {
        Trait cynical("cynical", "Cynical");
        cynical.description = "This character doubts religious teachings";
        cynical.category = TraitCategory::RELIGIOUS;
        cynical.intrigue_modifier = 2;
        cynical.learning_modifier = 1;
        cynical.opposite_traits = {"pious", "zealous"};
        m_traits[cynical.id] = cynical;
    }

    // ========================================================================
    // Reputation Traits
    // ========================================================================

    {
        Trait honorable("honorable", "Honorable");
        honorable.description = "This character is known for keeping their word";
        honorable.category = TraitCategory::REPUTATION;
        honorable.diplomacy_modifier = 2;
        honorable.honor_modifier = 0.4f;
        honorable.opinion_modifier = 15.0f;
        honorable.opposite_traits = {"dishonorable", "treacherous"};
        m_traits[honorable.id] = honorable;
    }

    {
        Trait dishonorable("dishonorable", "Dishonorable");
        dishonorable.description = "This character cannot be trusted";
        dishonorable.category = TraitCategory::REPUTATION;
        dishonorable.intrigue_modifier = 1;
        dishonorable.honor_modifier = -0.4f;
        dishonorable.opinion_modifier = -15.0f;
        dishonorable.opposite_traits = {"honorable"};
        m_traits[dishonorable.id] = dishonorable;
    }

    {
        Trait treacherous("treacherous", "Treacherous");
        treacherous.description = "This character is known for betrayal";
        treacherous.category = TraitCategory::REPUTATION;
        treacherous.intrigue_modifier = 3;
        treacherous.honor_modifier = -0.5f;
        treacherous.loyalty_modifier = -0.3f;
        treacherous.opinion_modifier = -20.0f;
        treacherous.opposite_traits = {"honorable"};
        m_traits[treacherous.id] = treacherous;
    }

    {
        Trait just("just", "Just");
        just.description = "This character is fair and impartial";
        just.category = TraitCategory::REPUTATION;
        just.stewardship_modifier = 2;
        just.honor_modifier = 0.3f;
        just.opinion_modifier = 10.0f;
        m_traits[just.id] = just;
    }

    CORE_STREAM_INFO("TraitDatabase")
        << "Initialized " << m_traits.size() << " default traits";
}

// ============================================================================
// TraitsComponent Serialization (Phase 6.5)
// ============================================================================

std::string TraitsComponent::Serialize() const {
    Json::Value data;

    // Schema version for future migration support
    data["schema_version"] = 1;

    // Serialize active traits
    Json::Value traits_array(Json::arrayValue);
    for (const auto& trait : active_traits) {
        Json::Value trait_data;
        trait_data["id"] = trait.trait_id;

        // Serialize time_point as milliseconds since epoch
        auto acquired_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            trait.acquired_date.time_since_epoch()).count();
        trait_data["acquired_date"] = Json::Int64(acquired_ms);

        trait_data["is_temporary"] = trait.is_temporary;

        if (trait.is_temporary) {
            auto expiry_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                trait.expiry_date.time_since_epoch()).count();
            trait_data["expiry_date"] = Json::Int64(expiry_ms);
        }

        traits_array.append(trait_data);
    }
    data["active_traits"] = traits_array;

    // Note: cached_modifiers will be recalculated on load

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";  // Compact JSON
    return Json::writeString(builder, data);
}

bool TraitsComponent::Deserialize(const std::string& json_str) {
    Json::Value data;
    Json::CharReaderBuilder builder;
    std::stringstream ss(json_str);
    std::string errors;

    if (!Json::parseFromStream(builder, ss, &data, &errors)) {
        CORE_STREAM_ERROR("TraitsComponent") << "Failed to parse JSON: " << errors;
        return false;
    }

    // Check schema version
    if (data.isMember("schema_version")) {
        int version = data["schema_version"].asInt();
        if (version > 1) {
            CORE_STREAM_WARN("TraitsComponent")
                << "Loading from newer schema version " << version
                << " (current: 1). Data may not load correctly.";
        }
    }

    // Clear existing traits
    active_traits.clear();

    // Deserialize active traits
    if (data.isMember("active_traits") && data["active_traits"].isArray()) {
        const Json::Value& traits_array = data["active_traits"];

        // Validate array size to prevent DoS from corrupted saves
        if (traits_array.size() > 50) {
            CORE_STREAM_WARN("TraitsComponent")
                << "Trait count exceeds maximum (50). Truncating to prevent corruption.";
        }

        size_t count = 0;
        for (const auto& trait_data : traits_array) {
            if (count >= 50) break;  // Enforce max trait limit

            if (!trait_data.isMember("id")) continue;

            std::string trait_id = trait_data["id"].asString();

            // Validate trait ID is not empty
            if (trait_id.empty()) {
                CORE_STREAM_WARN("TraitsComponent") << "Skipping trait with empty ID";
                continue;
            }

            ActiveTrait trait(trait_id);

            // Deserialize time_point from milliseconds with validation
            if (trait_data.isMember("acquired_date")) {
                auto acquired_ms = trait_data["acquired_date"].asInt64();

                // Validate timestamp is in reasonable range (year 1970-2100)
                if (acquired_ms < 0 || acquired_ms > 4102444800000) {
                    CORE_STREAM_WARN("TraitsComponent")
                        << "Invalid acquired_date timestamp: " << acquired_ms
                        << ". Using current time.";
                    trait.acquired_date = std::chrono::system_clock::now();
                } else {
                    trait.acquired_date = std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(acquired_ms));
                }
            }

            if (trait_data.isMember("is_temporary")) {
                trait.is_temporary = trait_data["is_temporary"].asBool();
            }

            if (trait.is_temporary && trait_data.isMember("expiry_date")) {
                auto expiry_ms = trait_data["expiry_date"].asInt64();

                // Validate expiry timestamp
                if (expiry_ms < 0 || expiry_ms > 4102444800000) {
                    CORE_STREAM_WARN("TraitsComponent")
                        << "Invalid expiry_date timestamp: " << expiry_ms;
                    trait.expiry_date = std::chrono::system_clock::now() + std::chrono::hours(24);
                } else {
                    trait.expiry_date = std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(expiry_ms));
                }
            }

            active_traits.push_back(trait);
            count++;
        }
    }

    // Mark modifiers for recalculation
    cached_modifiers.needs_recalculation = true;

    return true;
}

} // namespace character
} // namespace game
