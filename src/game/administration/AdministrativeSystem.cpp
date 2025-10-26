// ============================================================================
// AdministrativeSystem.cpp - Administrative System Implementation
// Strategic Rebuild: October 21, 2025 - Following PopulationSystem pattern
// Location: src/game/administration/AdministrativeSystem.cpp
// ============================================================================

#include "game/administration/AdministrativeSystem.h"
#include "game/administration/AdministrativeComponents.h"
#include "core/logging/Logger.h"
#include "core/types/game_types.h"
#include <algorithm>
#include <cmath>

namespace game::administration {

// ============================================================================
// AdministrativeSystem Implementation
// ============================================================================

AdministrativeSystem::AdministrativeSystem(::core::ecs::ComponentAccessManager& access_manager,
                                           ::core::ecs::MessageBus& message_bus)
    : m_access_manager(access_manager), m_message_bus(message_bus) {
    
    ::core::logging::LogInfo("AdministrativeSystem", "Administrative System created");
}

void AdministrativeSystem::Initialize() {
    if (m_initialized) {
        return;
    }

    ::core::logging::LogInfo("AdministrativeSystem", "Initializing Administrative System");

    LoadConfiguration();
    SubscribeToEvents();

    m_initialized = true;
    ::core::logging::LogInfo("AdministrativeSystem", "Administrative System initialized successfully");
}

void AdministrativeSystem::Update(float delta_time) {
    if (!m_initialized) {
        return;
    }

    m_accumulated_time += delta_time;

    // Process regular updates
    ProcessRegularUpdates(delta_time);

    // Process monthly administrative cycle
    m_monthly_timer += delta_time;
    if (m_monthly_timer >= m_config.monthly_update_interval) {
        ProcessMonthlyUpdates(delta_time);
        m_monthly_timer = 0.0f;
    }
}

void AdministrativeSystem::Shutdown() {
    if (!m_initialized) {
        return;
    }

    ::core::logging::LogInfo("AdministrativeSystem", "Shutting down Administrative System");
    m_initialized = false;
}

::core::threading::ThreadingStrategy AdministrativeSystem::GetThreadingStrategy() const {
    return ::core::threading::ThreadingStrategy::THREAD_POOL;
}

std::string AdministrativeSystem::GetThreadingRationale() const {
    return "Administrative calculations are independent per entity and benefit from parallelization";
}

// ============================================================================
// System Initialization
// ============================================================================

void AdministrativeSystem::LoadConfiguration() {
    // Load configuration values
    m_config.base_efficiency = 0.7;
    m_config.corruption_base_rate = 0.05;
    m_config.reform_cost_multiplier = 1.0;
    m_config.clerk_monthly_salary = 10;
    m_config.official_monthly_salary = 50;
    m_config.judge_monthly_salary = 75;
    
    ::core::logging::LogInfo("AdministrativeSystem", "Configuration loaded successfully");
}

void AdministrativeSystem::SubscribeToEvents() {
    // TODO: Implement proper message bus subscriptions
    ::core::logging::LogDebug("AdministrativeSystem", "Event subscriptions established");
}

// ============================================================================
// Update Processing
// ============================================================================

void AdministrativeSystem::ProcessRegularUpdates(float delta_time) {
    // Regular administrative processing
}

void AdministrativeSystem::ProcessMonthlyUpdates(float delta_time) {
    // Monthly processing: salaries, efficiency calculations, etc.
    ::core::logging::LogDebug("AdministrativeSystem", "Processing monthly administrative updates");
}

// ============================================================================
// Administrative Management Methods
// ============================================================================

void AdministrativeSystem::CreateAdministrativeComponents(game::types::EntityID entity_id) {
    ::core::logging::LogInfo("AdministrativeSystem", 
        "Creating administrative components for entity " + std::to_string(static_cast<int>(entity_id)));

    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        ::core::logging::LogError("AdministrativeSystem", "EntityManager not available");
        return;
    }

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);

    // Note: Using GovernanceComponent as main administrative component
    // (GovernanceComponent contains administrative_efficiency and officials)

    // Create governance component
    auto governance_component = entity_manager->AddComponent<GovernanceComponent>(entity_handle);
    if (governance_component) {
        governance_component->governance_type = GovernanceType::FEUDAL;
        governance_component->administrative_efficiency = 0.5;
        governance_component->governance_stability = 0.8;
        governance_component->tax_collection_efficiency = 0.6;
        
        ::core::logging::LogInfo("AdministrativeSystem", "Created GovernanceComponent");
    }

    // Create bureaucracy component
    auto bureaucracy_component = entity_manager->AddComponent<BureaucracyComponent>(entity_handle);
    if (bureaucracy_component) {
        bureaucracy_component->bureaucracy_level = 1;
        bureaucracy_component->scribes_employed = 5;
        bureaucracy_component->clerks_employed = 3;
        bureaucracy_component->corruption_level = 0.1;
        
        ::core::logging::LogInfo("AdministrativeSystem", "Created BureaucracyComponent");
    }

    // Create law component
    auto law_component = entity_manager->AddComponent<LawComponent>(entity_handle);
    if (law_component) {
        law_component->primary_law_system = LawType::COMMON_LAW;
        law_component->law_enforcement_effectiveness = 0.6;
        
        ::core::logging::LogInfo("AdministrativeSystem", "Created LawComponent");
    }
}

void AdministrativeSystem::ProcessMonthlyUpdate(game::types::EntityID entity_id) {
    CalculateEfficiency(entity_id);
    ProcessCorruption(entity_id);
    UpdateSalaries(entity_id);
    GenerateAdministrativeEvents(entity_id);
}

// ============================================================================
// Official Management
// ============================================================================

bool AdministrativeSystem::AppointOfficial(game::types::EntityID entity_id, OfficialType type, 
                                          const std::string& name) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return false;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    
    if (!governance_component) return false;

    AdministrativeOfficial new_official(name);
    new_official.type = type;
    new_official.competence = 0.6 + (rand() % 40) / 100.0; // 0.6-1.0 range
    new_official.loyalty = 0.7 + (rand() % 30) / 100.0;
    new_official.corruption_resistance = 0.7;
    new_official.months_in_position = 0;

    governance_component->appointed_officials.push_back(new_official);
    
    // Update administrative costs
    governance_component->monthly_administrative_costs += m_config.official_monthly_salary;

    ::core::logging::LogInfo("AdministrativeSystem", 
        "Appointed official: " + name + " (Type: " + std::to_string(static_cast<int>(type)) + ")");

    return true;
}

bool AdministrativeSystem::DismissOfficial(game::types::EntityID entity_id, uint32_t official_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return false;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    
    if (!governance_component) return false;

    auto& officials = governance_component->appointed_officials;
    auto it = std::remove_if(officials.begin(), officials.end(),
        [official_id](const AdministrativeOfficial& official) {
            return official.official_id == official_id;
        });
    
    if (it != officials.end()) {
        officials.erase(it, officials.end());
        governance_component->monthly_administrative_costs -= m_config.official_monthly_salary;
        
        ::core::logging::LogInfo("AdministrativeSystem", 
            "Dismissed official with ID: " + std::to_string(official_id));
        return true;
    }

    return false;
}

// ============================================================================
// Efficiency Calculations
// ============================================================================

double AdministrativeSystem::GetAdministrativeEfficiency(game::types::EntityID entity_id) const {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return m_config.base_efficiency;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    
    return governance_component ? governance_component->administrative_efficiency : m_config.base_efficiency;
}

double AdministrativeSystem::GetTaxCollectionRate(game::types::EntityID entity_id) const {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return 0.7;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    auto bureaucracy_component = entity_manager->GetComponent<BureaucracyComponent>(entity_handle);
    
    if (!governance_component) return 0.7;

    // Tax collection rate based on administrative efficiency and corruption
    double base_rate = governance_component->tax_collection_efficiency;
    double corruption_penalty = bureaucracy_component ? bureaucracy_component->corruption_level : 0.0;
    
    return std::max(0.1, base_rate - corruption_penalty);
}

double AdministrativeSystem::GetBureaucraticEfficiency(game::types::EntityID entity_id) const {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return 0.5;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto bureaucracy_component = entity_manager->GetComponent<BureaucracyComponent>(entity_handle);
    
    if (!bureaucracy_component) return 0.5;

    return bureaucracy_component->record_keeping_quality * 
           (bureaucracy_component->bureaucracy_level / 10.0);
}

// ============================================================================
// Governance Operations
// ============================================================================

void AdministrativeSystem::UpdateGovernanceType(game::types::EntityID entity_id, GovernanceType new_type) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    
    if (governance_component) {
        governance_component->governance_type = new_type;
        governance_component->governance_stability -= 0.1; // Change causes instability
        
        ::core::logging::LogInfo("AdministrativeSystem", 
            "Updated governance type to: " + std::to_string(static_cast<int>(new_type)));
    }
}

void AdministrativeSystem::ProcessAdministrativeReforms(game::types::EntityID entity_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    
    if (governance_component) {
        // Reforms improve efficiency but cost money
        governance_component->administrative_efficiency += 0.05;
        if (governance_component->administrative_efficiency > m_config.max_efficiency) {
            governance_component->administrative_efficiency = m_config.max_efficiency;
        }
        
        ::core::logging::LogInfo("AdministrativeSystem", "Processed administrative reforms");
    }
}

// ============================================================================
// Bureaucracy Operations
// ============================================================================

void AdministrativeSystem::ExpandBureaucracy(game::types::EntityID entity_id, uint32_t additional_clerks) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto bureaucracy_component = entity_manager->GetComponent<BureaucracyComponent>(entity_handle);
    
    if (bureaucracy_component) {
        bureaucracy_component->clerks_employed += additional_clerks;
        bureaucracy_component->administrative_speed += additional_clerks * 0.01;
        
        ::core::logging::LogInfo("AdministrativeSystem", 
            "Expanded bureaucracy by " + std::to_string(additional_clerks) + " clerks");
    }
}

void AdministrativeSystem::ImproveRecordKeeping(game::types::EntityID entity_id, double investment) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto bureaucracy_component = entity_manager->GetComponent<BureaucracyComponent>(entity_handle);
    
    if (bureaucracy_component) {
        double improvement = investment / 1000.0; // 1000 gold = 0.1 improvement
        bureaucracy_component->record_keeping_quality += improvement;
        if (bureaucracy_component->record_keeping_quality > 1.0) {
            bureaucracy_component->record_keeping_quality = 1.0;
        }
        
        ::core::logging::LogInfo("AdministrativeSystem", "Improved record keeping");
    }
}

// ============================================================================
// Law System Operations
// ============================================================================

void AdministrativeSystem::EstablishCourt(game::types::EntityID entity_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto law_component = entity_manager->GetComponent<LawComponent>(entity_handle);
    
    if (law_component) {
        law_component->courts_established++;
        law_component->law_enforcement_effectiveness += 0.1;
        
        ::core::logging::LogInfo("AdministrativeSystem", "Established court system");
    }
}

void AdministrativeSystem::AppointJudge(game::types::EntityID entity_id, const std::string& judge_name) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto law_component = entity_manager->GetComponent<LawComponent>(entity_handle);
    
    if (law_component) {
        law_component->judges_appointed++;
        law_component->law_enforcement_effectiveness += 0.05;
        
        ::core::logging::LogInfo("AdministrativeSystem", "Appointed judge: " + judge_name);
    }
}

void AdministrativeSystem::EnactLaw(game::types::EntityID entity_id, const std::string& law_description) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto law_component = entity_manager->GetComponent<LawComponent>(entity_handle);
    
    if (law_component) {
        law_component->active_laws.push_back(law_description);
        
        ::core::logging::LogInfo("AdministrativeSystem", "Enacted law: " + law_description);
    }
}

// ============================================================================
// Configuration Access
// ============================================================================

const AdministrativeSystemConfig& AdministrativeSystem::GetConfiguration() const {
    return m_config;
}

// ============================================================================
// Internal Methods
// ============================================================================

void AdministrativeSystem::CalculateEfficiency(game::types::EntityID entity_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    auto bureaucracy_component = entity_manager->GetComponent<BureaucracyComponent>(entity_handle);
    
    if (governance_component) {
        // Calculate efficiency based on officials
        double efficiency = m_config.base_efficiency;
        
        for (const auto& official : governance_component->appointed_officials) {
            efficiency += (official.competence / 100.0) * 0.05;
        }
        
        // Apply corruption penalty
        if (bureaucracy_component) {
            efficiency -= bureaucracy_component->corruption_level;
        }
        
        // Clamp to valid range
        efficiency = std::max(m_config.min_efficiency, 
                             std::min(m_config.max_efficiency, efficiency));
        
        governance_component->administrative_efficiency = efficiency;
    }
}

void AdministrativeSystem::ProcessCorruption(game::types::EntityID entity_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    auto bureaucracy_component = entity_manager->GetComponent<BureaucracyComponent>(entity_handle);
    
    if (bureaucracy_component) {
        // Corruption increases slowly over time
        bureaucracy_component->corruption_level += m_config.corruption_base_rate * 0.01;
        
        // Officials with high corruption tendency increase overall corruption
        if (governance_component) {
            for (auto& official : governance_component->appointed_officials) {
                if (official.loyalty < 50) {
                    bureaucracy_component->corruption_level += 0.005;
                }
            }
        }
        
        // Clamp corruption
        if (bureaucracy_component->corruption_level > 1.0) {
            bureaucracy_component->corruption_level = 1.0;
        }
    }
}

void AdministrativeSystem::UpdateSalaries(game::types::EntityID entity_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    
    if (governance_component) {
        int total_officials = governance_component->appointed_officials.size();
        int total_salary = total_officials * m_config.official_monthly_salary;
        governance_component->monthly_administrative_costs = total_salary;
    }
}

void AdministrativeSystem::GenerateAdministrativeEvents(game::types::EntityID entity_id) {
    // TODO: Implement random event generation
}

// ============================================================================
// ISystem Interface Implementation
// ============================================================================

std::string AdministrativeSystem::GetSystemName() const {
    return "AdministrativeSystem";
}

Json::Value AdministrativeSystem::Serialize(int version) const {
    Json::Value data;
    data["system_name"] = "AdministrativeSystem";
    data["version"] = version;
    data["initialized"] = m_initialized;
    // TODO: Serialize administrative state
    return data;
}

bool AdministrativeSystem::Deserialize(const Json::Value& data, int version) {
    if (data["system_name"].asString() != "AdministrativeSystem") {
        return false;
    }
    m_initialized = data["initialized"].asBool();
    // TODO: Deserialize administrative state
    return true;
}

} // namespace game::administration
