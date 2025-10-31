// ============================================================================
// AdministrativeSystem.cpp - Administrative System Implementation
// Strategic Rebuild: October 21, 2025 - Following PopulationSystem pattern
// Location: src/game/administration/AdministrativeSystem.cpp
// ============================================================================

#include "game/administration/AdministrativeSystem.h"
#include "game/administration/AdministrativeComponents.h"
#include "game/population/PopulationEvents.h"
#include "core/logging/Logger.h"
#include "core/types/game_types.h"
#include <json/json.h>
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
    // Configuration is already initialized with default values in the header
    // This method can load from file or override defaults if needed
    
    // For now, log that we're using default configuration
    ::core::logging::LogInfo("AdministrativeSystem", 
        "Administrative System using default configuration values");
    ::core::logging::LogInfo("AdministrativeSystem", 
        "Base efficiency: " + std::to_string(m_config.base_efficiency) +
        ", Corruption rate: " + std::to_string(m_config.corruption_base_rate));
}

void AdministrativeSystem::SubscribeToEvents() {
    // Subscribe to internal administrative events
    m_message_bus.Subscribe<AdminAppointmentEvent>(
        [this](const AdminAppointmentEvent& event) {
            HandleOfficialAppointment(event);
        });

    m_message_bus.Subscribe<AdminCorruptionEvent>(
        [this](const AdminCorruptionEvent& event) {
            HandleCorruptionDetection(event);
        });

    m_message_bus.Subscribe<AdminDismissalEvent>(
        [this](const AdminDismissalEvent& event) {
            HandleOfficialDismissal(event);
        });

    m_message_bus.Subscribe<AdminReformEvent>(
        [this](const AdminReformEvent& event) {
            HandleAdministrativeReform(event);
        });

    // Subscribe to population system events affecting administration
    m_message_bus.Subscribe<game::population::messages::PopulationCrisis>(
        [this](const game::population::messages::PopulationCrisis& event) {
            HandlePopulationCrisis(event);
        });

    m_message_bus.Subscribe<game::population::messages::TaxationPolicyUpdate>(
        [this](const game::population::messages::TaxationPolicyUpdate& event) {
            HandleTaxationUpdate(event);
        });

    m_message_bus.Subscribe<game::population::messages::MilitaryRecruitmentResult>(
        [this](const game::population::messages::MilitaryRecruitmentResult& event) {
            HandleRecruitmentCompletion(event);
        });

    m_message_bus.Subscribe<game::population::messages::PopulationEconomicUpdate>(
        [this](const game::population::messages::PopulationEconomicUpdate& event) {
            HandleEconomicUpdate(event);
        });

    ::core::logging::LogInfo("AdministrativeSystem", "Event subscriptions established successfully");
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

    // Generate unique official ID
    static uint32_t next_official_id = 1;
    uint32_t official_id = next_official_id++;
    
    // Create official using unified structure with proper constructor
    AdministrativeOfficial new_official(official_id, name, type, entity_id);
    
    // Assign salary based on type from config
    switch (type) {
        case OfficialType::TAX_COLLECTOR:
            new_official.salary_cost = m_config.tax_collector_salary;
            break;
        case OfficialType::TRADE_MINISTER:
            new_official.salary_cost = m_config.trade_minister_salary;
            break;
        case OfficialType::MILITARY_GOVERNOR:
            new_official.salary_cost = m_config.military_governor_salary;
            break;
        case OfficialType::COURT_ADVISOR:
            new_official.salary_cost = m_config.court_advisor_salary;
            break;
        case OfficialType::PROVINCIAL_GOVERNOR:
            new_official.salary_cost = m_config.provincial_governor_salary;
            break;
        case OfficialType::JUDGE:
            new_official.salary_cost = m_config.judge_salary;
            break;
        case OfficialType::SCRIBE:
            new_official.salary_cost = m_config.scribe_salary;
            break;
        case OfficialType::CUSTOMS_OFFICER:
            new_official.salary_cost = m_config.customs_officer_salary;
            break;
        default:
            new_official.salary_cost = 50.0;
            break;
    }

    governance_component->appointed_officials.push_back(new_official);
    
    // Update administrative costs
    governance_component->monthly_administrative_costs += new_official.GetMonthlyUpkeepCost();

    // Publish appointment event to MessageBus
    AdminAppointmentEvent appointment_event(entity_id, official_id, type, name);
    m_message_bus.PublishMessage(appointment_event);

    ::core::logging::LogInfo("AdministrativeSystem", 
        "Appointed official: " + name + " (Type: " + std::to_string(static_cast<int>(type)) + 
        ", ID: " + std::to_string(official_id) + ")");

    return true;
}

bool AdministrativeSystem::DismissOfficial(game::types::EntityID entity_id, uint32_t official_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return false;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    
    if (!governance_component) return false;

    auto& officials = governance_component->appointed_officials;
    auto it = std::find_if(officials.begin(), officials.end(),
        [official_id](const AdministrativeOfficial& official) {
            return official.official_id == official_id;
        });
    
    if (it != officials.end()) {
        double salary_reduction = it->GetMonthlyUpkeepCost();
        std::string dismissed_name = it->name;
        
        // Publish dismissal event
        AdminDismissalEvent dismissal_event(entity_id, official_id, "Administrative decision");
        m_message_bus.PublishMessage(dismissal_event);
        
        officials.erase(it);
        governance_component->monthly_administrative_costs -= salary_reduction;
        
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
        double efficiency_change = m_config.reform_efficiency_gain;
        double reform_cost = m_config.reform_cost_multiplier * 1000.0;

        governance_component->administrative_efficiency += efficiency_change;
        if (governance_component->administrative_efficiency > m_config.max_efficiency) {
            governance_component->administrative_efficiency = m_config.max_efficiency;
        }

        // Publish reform event to MessageBus
        AdminReformEvent reform_event(entity_id, "Administrative efficiency reform",
                                     reform_cost, efficiency_change);
        m_message_bus.PublishMessage(reform_event);

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
    
    if (!governance_component) return;
    
    // Start with base efficiency
    double efficiency = m_config.base_efficiency;
    
    // Calculate official contribution (properly normalized)
    double total_competence = 0.0;
    int official_count = 0;
    int corrupt_count = 0;
    
    for (const auto& official : governance_component->appointed_officials) {
        // Use GetEffectiveCompetence() which already applies trait bonuses
        double effective_comp = official.GetEffectiveCompetence();
        total_competence += effective_comp;
        official_count++;
        
        if (official.IsCorrupt()) {
            corrupt_count++;
        }
    }
    
    // Average official competence contributes to efficiency
    if (official_count > 0) {
        double avg_competence = total_competence / official_count;
        efficiency += (avg_competence - 0.5) * 0.4; // ±20% based on avg competence
    }
    
    // Corruption penalty from config
    if (corrupt_count > 0) {
        efficiency -= corrupt_count * m_config.corruption_penalty_efficiency;
    }
    
    // Apply systemic corruption from bureaucracy
    if (bureaucracy_component) {
        efficiency -= bureaucracy_component->corruption_level;
        
        // Bureaucracy size bonus (diminishing returns)
        uint32_t total_staff = bureaucracy_component->scribes_employed + 
                              bureaucracy_component->clerks_employed + 
                              bureaucracy_component->administrators_employed;
        double bureaucracy_bonus = std::min(0.2, total_staff * 0.001);
        efficiency += bureaucracy_bonus;
    }
    
    // Clamp to valid range
    efficiency = std::clamp(efficiency, m_config.min_efficiency, m_config.max_efficiency);
    
    governance_component->administrative_efficiency = efficiency;
}

void AdministrativeSystem::ProcessCorruption(game::types::EntityID entity_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    auto bureaucracy_component = entity_manager->GetComponent<BureaucracyComponent>(entity_handle);
    
    if (!bureaucracy_component) return;
    
    // Base corruption growth
    double corruption_increase = m_config.corruption_base_rate * 0.01;
    
    // Officials influence corruption
    if (governance_component) {
        for (auto& official : governance_component->appointed_officials) {
            // Process monthly update for each official
            official.ProcessMonthlyUpdate(m_config.competence_drift_rate, 
                                         m_config.satisfaction_decay_rate);
            
            // Low loyalty or corrupt officials increase corruption
            if (official.GetLoyaltyModifier() < 0.5) {
                corruption_increase += 0.005;
            }
            
            if (official.IsCorrupt()) {
                corruption_increase += 0.01;
                
                // Publish corruption event if suspicion crosses threshold
                if (official.corruption_suspicion > 80 && !official.has_pending_event) {
                    AdminCorruptionEvent corruption_event(
                        entity_id,
                        official.official_id,
                        static_cast<double>(official.corruption_suspicion) / 100.0,
                        "Official corruption detected: " + official.name
                    );
                    m_message_bus.PublishMessage(corruption_event);
                    official.has_pending_event = true;
                }
            }
        }
    }
    
    bureaucracy_component->corruption_level += corruption_increase;
    
    // Clamp corruption
    bureaucracy_component->corruption_level = std::clamp(bureaucracy_component->corruption_level, 
                                                         0.0, 1.0);
}

void AdministrativeSystem::UpdateSalaries(game::types::EntityID entity_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    
    if (!governance_component) return;
    
    // Calculate total salaries from all appointed officials
    double total_salary = 0.0;
    for (const auto& official : governance_component->appointed_officials) {
        total_salary += official.GetMonthlyUpkeepCost();
    }
    
    governance_component->monthly_administrative_costs = total_salary;
}

void AdministrativeSystem::GenerateAdministrativeEvents(game::types::EntityID entity_id) {
    // TODO: Implement random event generation (promotions, scandals, discoveries)
    // Future integration: Link to character system for official events
}

// ============================================================================
// Event Handler Methods
// ============================================================================

void AdministrativeSystem::HandleOfficialAppointment(const AdminAppointmentEvent& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(event.province_id), 1);
    auto events_component = entity_manager->GetComponent<AdministrativeEventsComponent>(entity_handle);

    if (events_component) {
        std::string appointment_record = "Appointed " + event.official_name +
            " as " + std::to_string(static_cast<int>(event.official_type));
        events_component->active_appointments.push_back(appointment_record);

        // Update administrative reputation based on official quality
        events_component->administrative_reputation += 0.01;

        // Limit history size
        if (events_component->active_appointments.size() > events_component->max_history_size) {
            events_component->active_appointments.erase(events_component->active_appointments.begin());
        }
    }

    ::core::logging::LogInfo("AdministrativeSystem",
        "Handled appointment event for official: " + event.official_name);
}

void AdministrativeSystem::HandleCorruptionDetection(const AdminCorruptionEvent& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(event.province_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    auto events_component = entity_manager->GetComponent<AdministrativeEventsComponent>(entity_handle);

    if (governance_component) {
        // Reduce governance stability due to corruption scandal
        governance_component->governance_stability -= event.corruption_level * 0.1;
        governance_component->governance_stability = std::max(0.0, governance_component->governance_stability);
    }

    if (events_component) {
        events_component->corruption_investigations.push_back(event.incident_description);
        events_component->administrative_reputation -= event.corruption_level * 0.05;
        events_component->public_trust -= event.corruption_level * 0.1;

        // Limit history size
        if (events_component->corruption_investigations.size() > events_component->max_history_size) {
            events_component->corruption_investigations.erase(events_component->corruption_investigations.begin());
        }
    }

    ::core::logging::LogWarning("AdministrativeSystem",
        "Corruption detected - Level: " + std::to_string(event.corruption_level) +
        " - " + event.incident_description);
}

void AdministrativeSystem::HandleOfficialDismissal(const AdminDismissalEvent& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(event.province_id), 1);
    auto events_component = entity_manager->GetComponent<AdministrativeEventsComponent>(entity_handle);

    if (events_component) {
        std::string dismissal_record = "Dismissed official " +
            std::to_string(event.official_id) + " - Reason: " + event.reason;
        events_component->pending_dismissals.push_back(dismissal_record);

        // Dismissal can have small negative or positive impact depending on reason
        if (event.reason.find("corruption") != std::string::npos) {
            // Dismissing corrupt officials improves reputation
            events_component->administrative_reputation += 0.02;
            events_component->public_trust += 0.01;
        }

        // Limit history size
        if (events_component->pending_dismissals.size() > events_component->max_history_size) {
            events_component->pending_dismissals.erase(events_component->pending_dismissals.begin());
        }
    }

    ::core::logging::LogInfo("AdministrativeSystem",
        "Handled dismissal event for official ID: " + std::to_string(event.official_id) +
        " - Reason: " + event.reason);
}

void AdministrativeSystem::HandleAdministrativeReform(const AdminReformEvent& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(event.province_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    auto bureaucracy_component = entity_manager->GetComponent<BureaucracyComponent>(entity_handle);
    auto events_component = entity_manager->GetComponent<AdministrativeEventsComponent>(entity_handle);

    if (governance_component) {
        // Apply efficiency change from reform
        governance_component->administrative_efficiency += event.efficiency_change;
        governance_component->administrative_efficiency = std::clamp(
            governance_component->administrative_efficiency,
            m_config.min_efficiency,
            m_config.max_efficiency
        );
    }

    if (bureaucracy_component) {
        bureaucracy_component->recent_reforms.push_back(event.reform_type);

        // Limit reform history
        if (bureaucracy_component->recent_reforms.size() > 10) {
            bureaucracy_component->recent_reforms.erase(bureaucracy_component->recent_reforms.begin());
        }
    }

    if (events_component) {
        events_component->reform_initiatives.push_back(event.reform_type);
        events_component->months_since_last_reform = 0;
        events_component->government_legitimacy += 0.02; // Reforms show active governance

        // Limit history size
        if (events_component->reform_initiatives.size() > events_component->max_history_size) {
            events_component->reform_initiatives.erase(events_component->reform_initiatives.begin());
        }
    }

    ::core::logging::LogInfo("AdministrativeSystem",
        "Handled reform event: " + event.reform_type +
        " - Cost: " + std::to_string(event.cost) +
        " - Efficiency change: " + std::to_string(event.efficiency_change));
}

void AdministrativeSystem::HandlePopulationCrisis(const game::population::messages::PopulationCrisis& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(event.province), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    auto events_component = entity_manager->GetComponent<AdministrativeEventsComponent>(entity_handle);

    if (governance_component) {
        // Population crises strain administrative capacity
        double crisis_impact = event.severity * 0.15;
        governance_component->administrative_efficiency -= crisis_impact;
        governance_component->administrative_efficiency = std::max(
            m_config.min_efficiency,
            governance_component->administrative_efficiency
        );

        governance_component->governance_stability -= event.severity * 0.1;
        governance_component->public_order_maintenance -= event.severity * 0.2;
    }

    if (events_component) {
        std::string crisis_record = event.crisis_type + " crisis - Severity: " +
            std::to_string(event.severity) + " - Population affected: " +
            std::to_string(event.population_affected);
        events_component->bureaucratic_failures.push_back(crisis_record);

        events_component->public_trust -= event.severity * 0.1;

        // Limit history size
        if (events_component->bureaucratic_failures.size() > events_component->max_history_size) {
            events_component->bureaucratic_failures.erase(events_component->bureaucratic_failures.begin());
        }
    }

    ::core::logging::LogWarning("AdministrativeSystem",
        "Handling population crisis: " + event.crisis_type +
        " - Severity: " + std::to_string(event.severity));
}

void AdministrativeSystem::HandleTaxationUpdate(const game::population::messages::TaxationPolicyUpdate& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(event.province_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);

    if (governance_component) {
        // Update tax rate and collection efficiency based on compliance
        governance_component->tax_rate = event.new_tax_rate;
        governance_component->tax_collection_efficiency = event.compliance_rate;
        governance_component->total_tax_revenue = event.expected_revenue;

        // Tax changes affect governance stability
        double tax_change = std::abs(event.new_tax_rate - 0.15); // Assuming 0.15 is baseline
        if (tax_change > 0.1) {
            governance_component->governance_stability -= tax_change * 0.1;
        }
    }

    ::core::logging::LogInfo("AdministrativeSystem",
        "Updated taxation policy - New rate: " + std::to_string(event.new_tax_rate) +
        " - Expected revenue: " + std::to_string(event.expected_revenue));
}

void AdministrativeSystem::HandleRecruitmentCompletion(const game::population::messages::MilitaryRecruitmentResult& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(event.province_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);

    if (governance_component) {
        // Update military administration efficiency based on recruitment success
        double success_rate = static_cast<double>(event.actual_recruits) /
                             std::max(1, event.requested_recruits);

        // Good recruitment improves military administration
        governance_component->military_administration_efficiency =
            0.7 * governance_component->military_administration_efficiency +
            0.3 * success_rate;

        governance_component->recruitment_administration = success_rate;
    }

    ::core::logging::LogInfo("AdministrativeSystem",
        "Processed recruitment completion - Requested: " + std::to_string(event.requested_recruits) +
        " - Actual: " + std::to_string(event.actual_recruits));
}

void AdministrativeSystem::HandleEconomicUpdate(const game::population::messages::PopulationEconomicUpdate& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(event.province_id), 1);
    auto governance_component = entity_manager->GetComponent<GovernanceComponent>(entity_handle);

    if (governance_component) {
        // Update economic-related administrative metrics
        governance_component->total_tax_revenue = event.tax_revenue_potential *
                                                   governance_component->tax_collection_efficiency;

        // Trade income affects trade administration efficiency
        if (event.trade_income > 0) {
            governance_component->trade_administration_efficiency =
                std::min(1.0, governance_component->trade_administration_efficiency + 0.01);
        }

        // High unemployment strains administrative capacity
        if (event.unemployment_rate > 0.2) {
            governance_component->population_administration_efficiency -=
                (event.unemployment_rate - 0.2) * 0.1;
            governance_component->population_administration_efficiency =
                std::max(0.1, governance_component->population_administration_efficiency);
        }
    }

    ::core::logging::LogDebug("AdministrativeSystem",
        "Processed economic update - Tax potential: " + std::to_string(event.tax_revenue_potential) +
        " - Unemployment: " + std::to_string(event.unemployment_rate));
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
