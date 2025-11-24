// ============================================================================
// FactionSystem.cpp - Faction System Implementation
// Created: November 18, 2025 - Faction System Implementation
// Location: src/game/faction/FactionSystem.cpp
// ============================================================================

#include "game/faction/FactionSystem.h"
#include "game/faction/FactionComponents.h"
#include "core/types/game_types.h"
#include <cmath>
#include <algorithm>
#include <sstream>

namespace game::faction {

    using namespace game::types;

    // ============================================================================
    // FactionData Implementation
    // ============================================================================

    FactionData::FactionData(FactionID id, FactionType faction_type, const std::string& name)
        : faction_id(id), type(faction_type), faction_name(name) {
    }

    double FactionData::GetEffectivePower() const {
        return power_base * cohesion * (1.0 + wealth_level * 0.5);
    }

    double FactionData::GetRevoltRisk() const {
        double risk = 0.0;
        risk += (1.0 - loyalty) * 0.4;
        risk += (1.0 - satisfaction) * 0.3;
        risk += militancy * 0.2;
        risk += (1.0 - cohesion) * 0.1;
        return std::clamp(risk, 0.0, 1.0);
    }

    bool FactionData::IsAngry() const {
        return satisfaction < 0.4 || loyalty < 0.3;
    }

    bool FactionData::IsContent() const {
        return satisfaction > 0.7 && loyalty > 0.7;
    }

    void FactionData::ProcessMonthlyUpdate() {
        months_since_demand++;
        months_since_concession++;

        // Natural satisfaction decay if no recent concessions
        if (months_since_concession > 6) {
            satisfaction = std::max(0.0, satisfaction - 0.01);
        }
    }

    void FactionData::AdjustSatisfaction(double change) {
        satisfaction = std::clamp(satisfaction + change, 0.0, 1.0);
    }

    void FactionData::AdjustInfluence(double change) {
        influence = std::clamp(influence + change, 0.0, 1.0);
    }

    void FactionData::AdjustLoyalty(double change) {
        loyalty = std::clamp(loyalty + change, 0.0, 1.0);
    }

    Json::Value FactionData::ToJson() const {
        Json::Value data;
        data["faction_id"] = faction_id.get();
        data["type"] = static_cast<int>(type);
        data["faction_name"] = faction_name;
        data["influence"] = influence;
        data["loyalty"] = loyalty;
        data["satisfaction"] = satisfaction;
        data["power_base"] = power_base;
        data["militancy"] = militancy;
        data["cohesion"] = cohesion;
        data["wealth_level"] = wealth_level;
        return data;
    }

    FactionData FactionData::FromJson(const Json::Value& data) {
        FactionData faction;
        faction.faction_id = FactionID(data["faction_id"].asUInt());
        faction.type = static_cast<FactionType>(data["type"].asInt());
        faction.faction_name = data["faction_name"].asString();
        faction.influence = data["influence"].asDouble();
        faction.loyalty = data["loyalty"].asDouble();
        faction.satisfaction = data["satisfaction"].asDouble();
        faction.power_base = data["power_base"].asDouble();
        faction.militancy = data["militancy"].asDouble();
        faction.cohesion = data["cohesion"].asDouble();
        faction.wealth_level = data["wealth_level"].asDouble();
        return faction;
    }

    // ============================================================================
    // Component Implementations
    // ============================================================================

    std::string ProvincialFactionsComponent::GetComponentTypeName() const {
        return "ProvincialFactionsComponent";
    }

    FactionData* ProvincialFactionsComponent::GetFaction(FactionType type) {
        auto it = faction_indices.find(type);
        if (it != faction_indices.end() && it->second < factions.size()) {
            return &factions[it->second];
        }
        return nullptr;
    }

    const FactionData* ProvincialFactionsComponent::GetFaction(FactionType type) const {
        auto it = faction_indices.find(type);
        if (it != faction_indices.end() && it->second < factions.size()) {
            return &factions[it->second];
        }
        return nullptr;
    }

    bool ProvincialFactionsComponent::HasFaction(FactionType type) const {
        return faction_indices.find(type) != faction_indices.end();
    }

    void ProvincialFactionsComponent::AddFaction(const FactionData& faction) {
        faction_indices[faction.type] = factions.size();
        factions.push_back(faction);
        RecalculateMetrics();
    }

    void ProvincialFactionsComponent::RemoveFaction(FactionType type) {
        auto it = faction_indices.find(type);
        if (it != faction_indices.end()) {
            factions.erase(factions.begin() + it->second);
            faction_indices.erase(it);
            // Rebuild indices
            faction_indices.clear();
            for (size_t i = 0; i < factions.size(); ++i) {
                faction_indices[factions[i].type] = i;
            }
            RecalculateMetrics();
        }
    }

    void ProvincialFactionsComponent::RecalculateMetrics() {
        if (factions.empty()) {
            total_faction_influence = 0.0;
            average_faction_loyalty = 0.7;
            average_faction_satisfaction = 0.6;
            faction_stability = 0.8;
            political_tension = 0.0;
            revolt_risk = 0.0;
            return;
        }

        double total_inf = 0.0;
        double total_loy = 0.0;
        double total_sat = 0.0;
        double total_revolt = 0.0;

        for (const auto& faction : factions) {
            total_inf += faction.influence;
            total_loy += faction.loyalty;
            total_sat += faction.satisfaction;
            total_revolt += faction.GetRevoltRisk();
        }

        total_faction_influence = total_inf;
        average_faction_loyalty = total_loy / factions.size();
        average_faction_satisfaction = total_sat / factions.size();
        revolt_risk = total_revolt / factions.size();

        // Calculate stability (inverse of tension)
        political_tension = std::max(0.0, (1.0 - average_faction_satisfaction) * 0.5 + (1.0 - average_faction_loyalty) * 0.3);
        faction_stability = 1.0 - political_tension;

        UpdatePowerDistribution();
    }

    void ProvincialFactionsComponent::UpdatePowerDistribution() {
        power_distribution.clear();
        double total_power = 0.0;

        for (const auto& faction : factions) {
            double power = faction.GetEffectivePower();
            power_distribution[faction.type] = power;
            total_power += power;
        }

        // Normalize to proportions
        if (total_power > 0.0) {
            for (auto& [type, power] : power_distribution) {
                power /= total_power;
            }
        }
    }

    std::vector<FactionType> ProvincialFactionsComponent::GetAngryFactions() const {
        std::vector<FactionType> angry;
        for (const auto& faction : factions) {
            if (faction.IsAngry()) {
                angry.push_back(faction.type);
            }
        }
        return angry;
    }

    FactionType ProvincialFactionsComponent::GetMostPowerfulFaction() const {
        if (factions.empty()) return FactionType::INVALID;

        auto it = std::max_element(factions.begin(), factions.end(),
            [](const FactionData& a, const FactionData& b) {
                return a.GetEffectivePower() < b.GetEffectivePower();
            });

        return it->type;
    }

    std::string NationalFactionsComponent::GetComponentTypeName() const {
        return "NationalFactionsComponent";
    }

    void NationalFactionsComponent::RecalculateNationalMetrics() {
        // This is typically called by the FactionSystem
        // See FactionSystem::UpdateNationalFactionMetrics()
    }

    void NationalFactionsComponent::UpdateCoalitions() {
        // This is typically called by the FactionSystem
        // See FactionSystem::UpdateCoalitions()
    }

    bool NationalFactionsComponent::IsInCoalition(FactionType faction) const {
        for (const auto& [f1, f2] : active_coalitions) {
            if (f1 == faction || f2 == faction) return true;
        }
        return false;
    }

    std::vector<FactionType> NationalFactionsComponent::GetHostileFactions(FactionType faction) const {
        std::vector<FactionType> hostile;
        for (const auto& [f1, f2] : hostile_relations) {
            if (f1 == faction) hostile.push_back(f2);
            if (f2 == faction) hostile.push_back(f1);
        }
        return hostile;
    }

    FactionType NationalFactionsComponent::GetMostInfluentialFaction() const {
        if (national_influence.empty()) return FactionType::INVALID;

        auto it = std::max_element(national_influence.begin(), national_influence.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second;
            });

        return it->first;
    }

    FactionType NationalFactionsComponent::GetMostDissatisfiedFaction() const {
        if (national_satisfaction.empty()) return FactionType::INVALID;

        auto it = std::min_element(national_satisfaction.begin(), national_satisfaction.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second;
            });

        return it->first;
    }

    std::string FactionDemandsComponent::GetComponentTypeName() const {
        return "FactionDemandsComponent";
    }

    void FactionDemandsComponent::AddDemand(const Demand& demand) {
        if (pending_demands.size() < max_simultaneous_demands) {
            pending_demands.push_back(demand);
        }
    }

    void FactionDemandsComponent::FulfillDemand(size_t demand_index) {
        if (demand_index < pending_demands.size()) {
            fulfilled_demands.push_back(pending_demands[demand_index]);
            pending_demands.erase(pending_demands.begin() + demand_index);

            // Trim history
            if (fulfilled_demands.size() > max_history_size) {
                fulfilled_demands.erase(fulfilled_demands.begin());
            }
        }
    }

    void FactionDemandsComponent::RejectDemand(size_t demand_index) {
        if (demand_index < pending_demands.size()) {
            rejected_demands.push_back(pending_demands[demand_index]);
            pending_demands.erase(pending_demands.begin() + demand_index);

            // Trim history
            if (rejected_demands.size() > max_history_size) {
                rejected_demands.erase(rejected_demands.begin());
            }
        }
    }

    std::vector<FactionDemandsComponent::Demand> FactionDemandsComponent::GetDemandsByFaction(FactionType type) const {
        std::vector<Demand> result;
        for (const auto& demand : pending_demands) {
            if (demand.faction == type) {
                result.push_back(demand);
            }
        }
        return result;
    }

    bool FactionDemandsComponent::HasUrgentDemands() const {
        for (const auto& demand : pending_demands) {
            if (demand.urgency > 0.7) return true;
        }
        return false;
    }

    bool FactionDemandsComponent::HasUltimatums() const {
        for (const auto& demand : pending_demands) {
            if (demand.is_ultimatum) return true;
        }
        return false;
    }

    Json::Value FactionDemandsComponent::Demand::ToJson() const {
        Json::Value data;
        data["faction"] = static_cast<int>(faction);
        data["demand_type"] = demand_type;
        data["demand_description"] = demand_description;
        data["urgency"] = urgency;
        data["importance"] = importance;
        data["months_pending"] = months_pending;
        data["is_ultimatum"] = is_ultimatum;
        return data;
    }

    FactionDemandsComponent::Demand FactionDemandsComponent::Demand::FromJson(const Json::Value& data) {
        Demand demand;
        demand.faction = static_cast<FactionType>(data["faction"].asInt());
        demand.demand_type = data["demand_type"].asString();
        demand.demand_description = data["demand_description"].asString();
        demand.urgency = data["urgency"].asDouble();
        demand.importance = data["importance"].asDouble();
        demand.months_pending = data["months_pending"].asUInt();
        demand.is_ultimatum = data["is_ultimatum"].asBool();
        return demand;
    }

    // ============================================================================
    // FactionSystemConfig Implementation
    // ============================================================================

    void FactionSystemConfig::InitializeDefaults() {
        // Nobility: high influence, moderate militancy
        faction_influence_base[FactionType::NOBILITY] = 0.5;
        faction_militancy[FactionType::NOBILITY] = 0.4;
        faction_cohesion_base[FactionType::NOBILITY] = 0.7;

        // Clergy: moderate influence, low militancy
        faction_influence_base[FactionType::CLERGY] = 0.4;
        faction_militancy[FactionType::CLERGY] = 0.2;
        faction_cohesion_base[FactionType::CLERGY] = 0.8;

        // Merchants: moderate influence, low militancy
        faction_influence_base[FactionType::MERCHANTS] = 0.4;
        faction_militancy[FactionType::MERCHANTS] = 0.1;
        faction_cohesion_base[FactionType::MERCHANTS] = 0.6;

        // Military: high influence, high militancy
        faction_influence_base[FactionType::MILITARY] = 0.5;
        faction_militancy[FactionType::MILITARY] = 0.7;
        faction_cohesion_base[FactionType::MILITARY] = 0.9;

        // Burghers: moderate influence, moderate militancy
        faction_influence_base[FactionType::BURGHERS] = 0.3;
        faction_militancy[FactionType::BURGHERS] = 0.3;
        faction_cohesion_base[FactionType::BURGHERS] = 0.5;

        // Peasants: low influence, high militancy
        faction_influence_base[FactionType::PEASANTS] = 0.2;
        faction_militancy[FactionType::PEASANTS] = 0.6;
        faction_cohesion_base[FactionType::PEASANTS] = 0.4;

        // Bureaucrats: moderate influence, very low militancy
        faction_influence_base[FactionType::BUREAUCRATS] = 0.4;
        faction_militancy[FactionType::BUREAUCRATS] = 0.1;
        faction_cohesion_base[FactionType::BUREAUCRATS] = 0.7;
    }

    // ============================================================================
    // FactionSystem Implementation
    // ============================================================================

    FactionSystem::FactionSystem(::core::ecs::ComponentAccessManager& access_manager,
                                 ::core::threading::ThreadSafeMessageBus& message_bus)
        : m_access_manager(access_manager)
        , m_message_bus(message_bus)
        , m_rng(std::random_device{}())
        , m_distribution(0.0, 1.0) {
    }

    void FactionSystem::Initialize() {
        if (m_initialized) return;

        LoadConfiguration();
        SubscribeToEvents();

        m_initialized = true;
    }

    void FactionSystem::Update(float delta_time) {
        if (!m_initialized) return;

        m_accumulated_time += delta_time;
        m_monthly_timer += delta_time;

        ProcessRegularUpdates(delta_time);

        // Process monthly updates
        if (m_monthly_timer >= m_config.monthly_update_interval) {
            ProcessMonthlyUpdates(delta_time);
            m_monthly_timer = 0.0f;
        }
    }

    void FactionSystem::Shutdown() {
        m_initialized = false;
    }

    ::core::threading::ThreadingStrategy FactionSystem::GetThreadingStrategy() const {
        return ::core::threading::ThreadingStrategy::MAIN_THREAD;
    }

    std::string FactionSystem::GetThreadingRationale() const {
        return "Faction system requires sequential processing for political dynamics";
    }

    std::string FactionSystem::GetSystemName() const {
        return "FactionSystem";
    }

    Json::Value FactionSystem::Serialize(int version) const {
        Json::Value data;
        data["version"] = version;
        data["system_name"] = GetSystemName();
        data["initialized"] = m_initialized;
        return data;
    }

    bool FactionSystem::Deserialize(const Json::Value& data, int version) {
        if (data["system_name"].asString() != GetSystemName()) {
            return false;
        }
        m_initialized = data["initialized"].asBool();
        return true;
    }

    void FactionSystem::LoadConfiguration() {
        m_config.InitializeDefaults();
    }

    void FactionSystem::SubscribeToEvents() {
        // Subscribe to faction-related events
        // Event subscriptions can be added here as the system integrates with other systems
        // For example:
        // - Administrative events (reforms, corruption investigations)
        // - Economic events (economic growth/decline)
        // - Military events (victories, defeats)
        // - Policy changes (tax changes, religious policies)

        // Note: MessageBus subscriptions would be set up here if using a callback-based system
        // The current implementation uses manual event polling
    }

    void FactionSystem::ProcessRegularUpdates(float delta_time) {
        // Continuous updates (currently minimal)
    }

    void FactionSystem::ProcessMonthlyUpdates(float delta_time) {
        // Get all entities with faction components
        auto entities = m_access_manager.GetEntityManager()->GetEntitiesWithComponent<ProvincialFactionsComponent>();

        for (const auto& entity_id : entities) {
            ProcessProvincialFactions(entity_id.id);
        }
    }

    void FactionSystem::ProcessProvincialFactions(EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        // Update each faction
        for (auto& faction : factions_comp->factions) {
            faction.ProcessMonthlyUpdate();
        }

        // Update dynamics
        UpdateFactionInfluence(entity_id);
        UpdateFactionLoyalty(entity_id);
        UpdateFactionSatisfaction(entity_id);
        UpdateFactionPower(entity_id);
        UpdateFactionRelationships(entity_id);

        // Generate demands
        ProcessDemandGeneration(entity_id);

        // Check for revolts
        ProcessRevoltAttempts(entity_id);

        // Recalculate metrics
        factions_comp->RecalculateMetrics();
    }

    void FactionSystem::InitializeFactions(EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponent<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) {
            // Create component using EntityManager
            auto* entity_manager = m_access_manager.GetEntityManager();
            if (entity_manager) {
                entity_manager->AddComponent<ProvincialFactionsComponent>(core::ecs::EntityID(entity_id, 1));
            }
        }

        // Re-fetch after potentially creating
        auto factions_comp_write = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (factions_comp_write && factions_comp_write->factions.empty()) {
            CreateDefaultFactions(entity_id);
        }
    }

    void FactionSystem::CreateDefaultFactions(EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        // Create standard factions
        std::vector<FactionType> default_factions = {
            FactionType::NOBILITY,
            FactionType::CLERGY,
            FactionType::MERCHANTS,
            FactionType::MILITARY,
            FactionType::BURGHERS,
            FactionType::PEASANTS
        };

        uint32_t faction_id_counter = 1;
        for (auto type : default_factions) {
            FactionData faction = CreateDefaultFaction(type, FactionID(faction_id_counter++));
            factions_comp->AddFaction(faction);
        }

        factions_comp->RecalculateMetrics();
    }

    FactionData FactionSystem::CreateDefaultFaction(FactionType type, FactionID id) const {
        FactionData faction(id, type, GetFactionName(type));

        // Set defaults from config
        auto inf_it = m_config.faction_influence_base.find(type);
        if (inf_it != m_config.faction_influence_base.end()) {
            faction.influence = inf_it->second;
        }

        auto mil_it = m_config.faction_militancy.find(type);
        if (mil_it != m_config.faction_militancy.end()) {
            faction.militancy = mil_it->second;
        }

        auto coh_it = m_config.faction_cohesion_base.find(type);
        if (coh_it != m_config.faction_cohesion_base.end()) {
            faction.cohesion = coh_it->second;
        }

        faction.loyalty = m_config.base_loyalty;
        faction.satisfaction = m_config.base_satisfaction;
        faction.power_base = faction.influence;

        return faction;
    }

    std::string FactionSystem::GetFactionName(FactionType type) const {
        switch (type) {
            case FactionType::NOBILITY: return "Nobility";
            case FactionType::CLERGY: return "Clergy";
            case FactionType::MERCHANTS: return "Merchants";
            case FactionType::MILITARY: return "Military";
            case FactionType::BURGHERS: return "Burghers";
            case FactionType::PEASANTS: return "Peasants";
            case FactionType::BUREAUCRATS: return "Bureaucrats";
            default: return "Unknown Faction";
        }
    }

    void FactionSystem::AddFaction(EntityID entity_id, const FactionData& faction) {
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (factions_comp) {
            factions_comp->AddFaction(faction);
        }
    }

    void FactionSystem::RemoveFaction(EntityID entity_id, FactionType type) {
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (factions_comp) {
            factions_comp->RemoveFaction(type);
        }
    }

    FactionData* FactionSystem::GetFaction(EntityID entity_id, FactionType type) {
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (factions_comp) {
            return factions_comp->GetFaction(type);
        }
        return nullptr;
    }

    void FactionSystem::AdjustInfluence(EntityID entity_id, FactionType faction_type, double change, const std::string& reason) {
        auto* faction = GetFaction(entity_id, faction_type);
        if (faction) {
            double old_influence = faction->influence;
            faction->AdjustInfluence(change);

            // Send event
            FactionInfluenceChangeEvent event(entity_id, faction_type, old_influence, faction->influence, reason);
            m_message_bus.Publish(event);
        }
    }

    void FactionSystem::AdjustLoyalty(EntityID entity_id, FactionType faction_type, double change, const std::string& reason) {
        auto* faction = GetFaction(entity_id, faction_type);
        if (faction) {
            faction->AdjustLoyalty(change);
        }
    }

    void FactionSystem::AdjustSatisfaction(EntityID entity_id, FactionType faction_type, double change, const std::string& reason) {
        auto* faction = GetFaction(entity_id, faction_type);
        if (faction) {
            double old_satisfaction = faction->satisfaction;
            faction->AdjustSatisfaction(change);

            // Send event
            FactionSatisfactionChangeEvent event(entity_id, faction_type, old_satisfaction, faction->satisfaction, reason);
            m_message_bus.Publish(event);
        }
    }

    void FactionSystem::UpdateFactionInfluence(EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        // Natural decay
        for (auto& faction : factions_comp->factions) {
            faction.influence = std::max(m_config.min_influence,
                                        faction.influence - m_config.base_influence_decay);
        }

        RedistributePower(entity_id);
    }

    void FactionSystem::UpdateFactionLoyalty(EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        for (auto& faction : factions_comp->factions) {
            // Loyalty decays slowly over time
            faction.loyalty = std::max(0.0, faction.loyalty - m_config.loyalty_decay_rate);
        }
    }

    void FactionSystem::UpdateFactionSatisfaction(EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        for (auto& faction : factions_comp->factions) {
            // Satisfaction decays if no recent concessions
            if (faction.months_since_concession > 6) {
                faction.satisfaction = std::max(0.0, faction.satisfaction - m_config.satisfaction_decay_rate);
            }
        }
    }

    void FactionSystem::UpdateFactionPower(EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        factions_comp->UpdatePowerDistribution();
        UpdateDominantFaction(entity_id);
    }

    void FactionSystem::UpdateFactionRelationships(EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        // Update inter-faction relationships based on power dynamics and compatibility
        for (size_t i = 0; i < factions_comp->factions.size(); ++i) {
            for (size_t j = i + 1; j < factions_comp->factions.size(); ++j) {
                const auto& faction1 = factions_comp->factions[i];
                const auto& faction2 = factions_comp->factions[j];

                std::string key = std::to_string(static_cast<int>(faction1.type)) + "_" +
                                 std::to_string(static_cast<int>(faction2.type));

                // Calculate relationship based on compatibility and power balance
                double compatibility = CalculateCoalitionCompatibility(faction1.type, faction2.type);
                double power_diff = std::abs(faction1.GetEffectivePower() - faction2.GetEffectivePower());

                // Similar power levels create tension, very different levels create dominance
                double relationship = compatibility - (power_diff * 0.3);

                factions_comp->inter_faction_relations[key] = std::clamp(relationship, -1.0, 1.0);
            }
        }

        // Calculate overall political tension
        double tension = 0.0;
        for (const auto& [key, value] : factions_comp->inter_faction_relations) {
            if (value < 0) {
                tension += std::abs(value);
            }
        }
        if (!factions_comp->inter_faction_relations.empty()) {
            factions_comp->political_tension = std::clamp(tension / factions_comp->inter_faction_relations.size(), 0.0, 1.0);
        }
    }

    void FactionSystem::RedistributePower(EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        // Power shifts based on various factors
        // This is a simplified model - could be much more complex
        for (auto& faction : factions_comp->factions) {
            double power_change = 0.0;

            // Satisfied factions gain power
            if (faction.satisfaction > 0.7) {
                power_change += m_config.power_shift_rate;
            }

            // Dissatisfied factions lose power (unless they're militant)
            if (faction.satisfaction < 0.4 && faction.militancy < 0.5) {
                power_change -= m_config.power_shift_rate;
            }

            faction.power_base = std::clamp(faction.power_base + power_change, 0.1, 0.9);
        }
    }

    void FactionSystem::UpdateDominantFaction(EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        factions_comp->dominant_faction = factions_comp->GetMostPowerfulFaction();
    }

    void FactionSystem::ProcessDemandGeneration(EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponent<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        for (const auto& faction : factions_comp->factions) {
            double demand_chance = m_config.demand_base_rate;

            // Dissatisfied factions make more demands
            if (faction.satisfaction < 0.4) {
                demand_chance = m_config.demand_rate_if_dissatisfied;
            }

            if (GetRandomValue() < demand_chance) {
                GenerateDemand(entity_id, faction.type);
            }
        }
    }

    void FactionSystem::GenerateDemand(EntityID entity_id, FactionType faction_type) {
        std::string demand_type = GenerateDemandType(faction_type);
        std::string description = GenerateDemandDescription(faction_type, demand_type);

        auto* faction = GetFaction(entity_id, faction_type);
        bool is_ultimatum = faction && faction->satisfaction < m_config.ultimatum_threshold;

        // Send event
        FactionDemandEvent event(entity_id, faction_type, demand_type, description, is_ultimatum);
        m_message_bus.Publish(event);
    }

    std::string FactionSystem::GenerateDemandType(FactionType faction) const {
        switch (faction) {
            case FactionType::NOBILITY: return "council_seat";
            case FactionType::CLERGY: return "religious_authority";
            case FactionType::MERCHANTS: return "trade_privileges";
            case FactionType::MILITARY: return "increased_funding";
            case FactionType::BURGHERS: return "self_governance";
            case FactionType::PEASANTS: return "tax_reduction";
            case FactionType::BUREAUCRATS: return "administrative_reform";
            default: return "generic_demand";
        }
    }

    std::string FactionSystem::GenerateDemandDescription(FactionType faction, const std::string& demand_type) const {
        std::ostringstream desc;
        desc << GetFactionName(faction) << " demand " << demand_type;
        return desc.str();
    }

    void FactionSystem::ProcessRevoltAttempts(EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponent<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        for (const auto& faction : factions_comp->factions) {
            if (ShouldRevolt(faction)) {
                if (GetRandomValue() < CalculateRevoltChance(faction)) {
                    TriggerFactionRevolt(entity_id, faction.type);
                }
            }
        }
    }

    bool FactionSystem::ShouldRevolt(const FactionData& faction) const {
        return faction.GetRevoltRisk() > m_config.revolt_risk_threshold;
    }

    double FactionSystem::CalculateRevoltChance(const FactionData& faction) const {
        double chance = m_config.revolt_base_chance;
        chance += (1.0 - faction.loyalty) * m_config.revolt_loyalty_modifier;
        chance += (1.0 - faction.satisfaction) * m_config.revolt_satisfaction_modifier;
        chance *= faction.militancy;
        return std::clamp(chance, 0.0, 1.0);
    }

    void FactionSystem::TriggerFactionRevolt(EntityID entity_id, FactionType faction_type) {
        auto* faction = GetFaction(entity_id, faction_type);
        if (!faction) return;

        faction->is_in_revolt = true;
        double revolt_strength = faction->GetEffectivePower();

        std::string reason = GetFactionName(faction_type) + " revolts due to low satisfaction and loyalty";

        FactionRevoltEvent event(entity_id, faction_type, revolt_strength, reason);
        m_message_bus.Publish(event);
    }

    void FactionSystem::CheckRevoltRisk(EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (factions_comp) {
            factions_comp->RecalculateMetrics();
        }
    }

    double FactionSystem::GetFactionInfluence(EntityID entity_id, FactionType faction_type) const {
        auto factions_comp = m_access_manager.GetComponent<ProvincialFactionsComponent>(entity_id);
        if (factions_comp) {
            const auto* faction = factions_comp->GetFaction(faction_type);
            return faction ? faction->influence : 0.0;
        }
        return 0.0;
    }

    double FactionSystem::GetFactionLoyalty(EntityID entity_id, FactionType faction_type) const {
        auto factions_comp = m_access_manager.GetComponent<ProvincialFactionsComponent>(entity_id);
        if (factions_comp) {
            const auto* faction = factions_comp->GetFaction(faction_type);
            return faction ? faction->loyalty : 0.7;
        }
        return 0.7;
    }

    double FactionSystem::GetFactionSatisfaction(EntityID entity_id, FactionType faction_type) const {
        auto factions_comp = m_access_manager.GetComponent<ProvincialFactionsComponent>(entity_id);
        if (factions_comp) {
            const auto* faction = factions_comp->GetFaction(faction_type);
            return faction ? faction->satisfaction : 0.6;
        }
        return 0.6;
    }

    double FactionSystem::GetRevoltRisk(EntityID entity_id, FactionType faction_type) const {
        auto factions_comp = m_access_manager.GetComponent<ProvincialFactionsComponent>(entity_id);
        if (factions_comp) {
            const auto* faction = factions_comp->GetFaction(faction_type);
            return faction ? faction->GetRevoltRisk() : 0.0;
        }
        return 0.0;
    }

    std::vector<FactionType> FactionSystem::GetAngryFactions(EntityID entity_id) const {
        auto factions_comp = m_access_manager.GetComponent<ProvincialFactionsComponent>(entity_id);
        if (factions_comp) {
            return factions_comp->GetAngryFactions();
        }
        return {};
    }

    FactionType FactionSystem::GetDominantFaction(EntityID entity_id) const {
        auto factions_comp = m_access_manager.GetComponent<ProvincialFactionsComponent>(entity_id);
        if (factions_comp) {
            return factions_comp->dominant_faction;
        }
        return FactionType::INVALID;
    }

    const FactionSystemConfig& FactionSystem::GetConfiguration() const {
        return m_config;
    }

    void FactionSystem::SetConfiguration(const FactionSystemConfig& config) {
        m_config = config;
    }

    double FactionSystem::Clamp(double value, double min_val, double max_val) const {
        return std::clamp(value, min_val, max_val);
    }

    double FactionSystem::GetRandomValue() const {
        return const_cast<FactionSystem*>(this)->m_distribution(const_cast<FactionSystem*>(this)->m_rng);
    }

    // ============================================================================
    // Additional Method Implementations
    // ============================================================================

    void FactionSystem::FulfillDemand(EntityID entity_id, FactionType faction_type, const std::string& demand_type) {
        auto* faction = GetFaction(entity_id, faction_type);
        if (!faction) return;

        // Increase satisfaction and loyalty
        AdjustSatisfaction(entity_id, faction_type, m_config.satisfaction_from_demand_fulfilled, "Demand fulfilled: " + demand_type);
        AdjustLoyalty(entity_id, faction_type, m_config.loyalty_gain_from_concession, "Demand fulfilled");

        // Reset demand timer
        faction->months_since_concession = 0;
        faction->months_since_demand = 0;

        // Remove from active demands
        auto& demands = faction->active_demands;
        demands.erase(std::remove(demands.begin(), demands.end(), demand_type), demands.end());
        faction->fulfilled_demands.push_back(demand_type);

        // Recalculate metrics
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (factions_comp) {
            factions_comp->months_since_last_concession = 0;
            factions_comp->RecalculateMetrics();
        }
    }

    void FactionSystem::RejectDemand(EntityID entity_id, FactionType faction_type, const std::string& demand_type) {
        auto* faction = GetFaction(entity_id, faction_type);
        if (!faction) return;

        // Decrease satisfaction and loyalty
        AdjustSatisfaction(entity_id, faction_type, m_config.satisfaction_from_demand_rejected, "Demand rejected: " + demand_type);
        AdjustLoyalty(entity_id, faction_type, -m_config.loyalty_loss_from_rejection, "Demand rejected");

        // Increase militancy
        faction->militancy = std::min(1.0, faction->militancy + 0.05);

        // Remove from active demands
        auto& demands = faction->active_demands;
        demands.erase(std::remove(demands.begin(), demands.end(), demand_type), demands.end());

        // Check if this triggers a crisis
        if (faction->satisfaction < 0.3) {
            faction->has_pending_crisis = true;
        }
    }

    void FactionSystem::RecalculatePowerBalance(EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponentForWrite<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        factions_comp->UpdatePowerDistribution();
        UpdateDominantFaction(entity_id);
    }

    void FactionSystem::ResolveRevolt(EntityID entity_id, FactionType faction_type, bool success) {
        auto* faction = GetFaction(entity_id, faction_type);
        if (!faction) return;

        faction->is_in_revolt = false;

        if (success) {
            // Successful revolt - faction gains power
            faction->influence = std::min(0.9, faction->influence + 0.2);
            faction->satisfaction = 0.8;
            faction->loyalty = 0.4; // Still not very loyal after rebelling
            faction->militancy = std::max(0.0, faction->militancy - 0.1);
        } else {
            // Failed revolt - faction is weakened
            faction->influence = std::max(0.1, faction->influence - 0.3);
            faction->satisfaction = 0.2;
            faction->loyalty = std::max(0.0, faction->loyalty - 0.2);
            faction->cohesion = std::max(0.3, faction->cohesion - 0.2);
        }

        RecalculatePowerBalance(entity_id);
    }

    void FactionSystem::FormCoalition(FactionType faction1, FactionType faction2, const std::string& reason) {
        // Get national factions component (assumes entity 0 is the nation)
        auto national_factions = m_access_manager.GetComponentForWrite<NationalFactionsComponent>(0);
        if (!national_factions) return;

        // Add coalition
        national_factions->active_coalitions.push_back({faction1, faction2});

        // Send event
        FactionCoalitionEvent event(faction1, faction2, true, reason);
        m_message_bus.Publish(event);
    }

    void FactionSystem::DissolveCoalition(FactionType faction1, FactionType faction2, const std::string& reason) {
        auto national_factions = m_access_manager.GetComponentForWrite<NationalFactionsComponent>(0);
        if (!national_factions) return;

        // Remove coalition
        auto& coalitions = national_factions->active_coalitions;
        coalitions.erase(
            std::remove_if(coalitions.begin(), coalitions.end(),
                [faction1, faction2](const auto& pair) {
                    return (pair.first == faction1 && pair.second == faction2) ||
                           (pair.first == faction2 && pair.second == faction1);
                }),
            coalitions.end()
        );

        // Send event
        FactionCoalitionEvent event(faction1, faction2, false, reason);
        m_message_bus.Publish(event);
    }

    void FactionSystem::UpdateCoalitions() {
        auto national_factions = m_access_manager.GetComponentForWrite<NationalFactionsComponent>(0);
        if (!national_factions) return;

        // Check existing coalitions for stability
        auto& coalitions = national_factions->active_coalitions;
        for (auto it = coalitions.begin(); it != coalitions.end();) {
            if (!ShouldMaintainCoalition(it->first, it->second)) {
                FactionCoalitionEvent event(it->first, it->second, false, "Coalition dissolved due to low compatibility");
                m_message_bus.Publish(event);
                it = coalitions.erase(it);
            } else {
                ++it;
            }
        }
    }

    void FactionSystem::UpdateNationalFactionMetrics(EntityID nation_id) {
        auto national_factions = m_access_manager.GetComponentForWrite<NationalFactionsComponent>(nation_id);
        if (!national_factions) return;

        // Aggregate from all provinces
        auto province_entities = m_access_manager.GetEntityManager()->GetEntitiesWithComponent<ProvincialFactionsComponent>();

        // Reset national metrics
        national_factions->national_influence.clear();
        national_factions->national_loyalty.clear();
        national_factions->national_satisfaction.clear();

        std::unordered_map<FactionType, int> faction_counts;

        for (const auto& province_handle : province_entities) {
            auto provincial_factions = m_access_manager.GetComponent<ProvincialFactionsComponent>(province_handle.id);
            if (!provincial_factions) continue;

            for (const auto& faction : provincial_factions->factions) {
                national_factions->national_influence[faction.type] += faction.influence;
                national_factions->national_loyalty[faction.type] += faction.loyalty;
                national_factions->national_satisfaction[faction.type] += faction.satisfaction;
                faction_counts[faction.type]++;
            }
        }

        // Average the values
        for (auto& [type, influence] : national_factions->national_influence) {
            if (faction_counts[type] > 0) {
                influence /= faction_counts[type];
                national_factions->national_loyalty[type] /= faction_counts[type];
                national_factions->national_satisfaction[type] /= faction_counts[type];
            }
        }
    }

    void FactionSystem::ProcessNationalDemands(EntityID nation_id) {
        auto national_factions = m_access_manager.GetComponentForWrite<NationalFactionsComponent>(nation_id);
        if (!national_factions) return;

        // Check if any faction has low satisfaction across the nation
        for (const auto& [type, satisfaction] : national_factions->national_satisfaction) {
            if (satisfaction < 0.4) {
                // Generate national-level demand if not already pending
                auto& demands = national_factions->national_demands[type];
                if (demands.empty() || national_factions->demands_pending_months[type] > 12) {
                    std::string demand = GenerateDemandType(type);
                    demands.push_back(demand);
                    national_factions->demands_pending_months[type] = 0;
                }
            }
        }

        // Increment pending months
        for (auto& [type, months] : national_factions->demands_pending_months) {
            months++;
        }
    }

    bool FactionSystem::ShouldFormCoalition(FactionType faction1, FactionType faction2) const {
        double compatibility = CalculateCoalitionCompatibility(faction1, faction2);
        return compatibility >= m_config.coalition_formation_threshold;
    }

    bool FactionSystem::ShouldMaintainCoalition(FactionType faction1, FactionType faction2) const {
        double compatibility = CalculateCoalitionCompatibility(faction1, faction2);
        // Use random factor to determine if coalition survives
        return GetRandomValue() < m_config.coalition_stability * compatibility;
    }

    double FactionSystem::CalculateCoalitionCompatibility(FactionType faction1, FactionType faction2) const {
        // Define compatibility matrix
        // This is a simplified model - could be much more sophisticated

        if (faction1 == faction2) return 1.0;

        // Natural alliances
        if ((faction1 == FactionType::NOBILITY && faction2 == FactionType::MILITARY) ||
            (faction2 == FactionType::NOBILITY && faction1 == FactionType::MILITARY)) return 0.8;

        if ((faction1 == FactionType::CLERGY && faction2 == FactionType::NOBILITY) ||
            (faction2 == FactionType::CLERGY && faction1 == FactionType::NOBILITY)) return 0.7;

        if ((faction1 == FactionType::MERCHANTS && faction2 == FactionType::BURGHERS) ||
            (faction2 == FactionType::MERCHANTS && faction1 == FactionType::BURGHERS)) return 0.9;

        // Natural oppositions
        if ((faction1 == FactionType::NOBILITY && faction2 == FactionType::PEASANTS) ||
            (faction2 == FactionType::NOBILITY && faction1 == FactionType::PEASANTS)) return 0.2;

        if ((faction1 == FactionType::MERCHANTS && faction2 == FactionType::PEASANTS) ||
            (faction2 == FactionType::MERCHANTS && faction1 == FactionType::PEASANTS)) return 0.3;

        // Default moderate compatibility
        return 0.5;
    }

    void FactionSystem::HandleAdministrativeEvent(const std::string& event_type, EntityID entity_id) {
        auto factions_comp = m_access_manager.GetComponent<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        // Administrative reforms please bureaucrats and nobility
        if (event_type == "reform") {
            AdjustSatisfaction(entity_id, FactionType::BUREAUCRATS, 0.1, "Administrative reform");
            AdjustSatisfaction(entity_id, FactionType::NOBILITY, 0.05, "Administrative reform");
        }

        // Corruption investigations anger corrupt officials but please general population
        if (event_type == "corruption_investigation") {
            AdjustSatisfaction(entity_id, FactionType::PEASANTS, 0.05, "Corruption crackdown");
            AdjustSatisfaction(entity_id, FactionType::BURGHERS, 0.05, "Corruption crackdown");
        }
    }

    void FactionSystem::HandleEconomicChange(EntityID entity_id, double economic_change) {
        auto factions_comp = m_access_manager.GetComponent<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        // Economic growth pleases merchants and burghers
        if (economic_change > 0) {
            double satisfaction_change = std::min(0.1, economic_change * 0.01);
            AdjustSatisfaction(entity_id, FactionType::MERCHANTS, satisfaction_change, "Economic growth");
            AdjustSatisfaction(entity_id, FactionType::BURGHERS, satisfaction_change, "Economic growth");
            AdjustSatisfaction(entity_id, FactionType::PEASANTS, satisfaction_change * 0.5, "Economic growth");
        }
        // Economic decline angers everyone
        else {
            double satisfaction_change = std::max(-0.15, economic_change * 0.02);
            for (const auto& faction : factions_comp->factions) {
                AdjustSatisfaction(entity_id, faction.type, satisfaction_change, "Economic decline");
            }
        }
    }

    void FactionSystem::HandleMilitaryEvent(EntityID entity_id, bool is_victory) {
        auto factions_comp = m_access_manager.GetComponent<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        if (is_victory) {
            // Victory pleases military and nobility
            AdjustSatisfaction(entity_id, FactionType::MILITARY, 0.15, "Military victory");
            AdjustSatisfaction(entity_id, FactionType::NOBILITY, 0.1, "Military victory");
            AdjustLoyalty(entity_id, FactionType::MILITARY, 0.05, "Military victory");
        } else {
            // Defeat angers military and nobility
            AdjustSatisfaction(entity_id, FactionType::MILITARY, -0.2, "Military defeat");
            AdjustSatisfaction(entity_id, FactionType::NOBILITY, -0.15, "Military defeat");
            AdjustLoyalty(entity_id, FactionType::MILITARY, -0.1, "Military defeat");
        }
    }

    void FactionSystem::HandlePolicyChange(EntityID entity_id, const std::string& policy_type) {
        auto factions_comp = m_access_manager.GetComponent<ProvincialFactionsComponent>(entity_id);
        if (!factions_comp) return;

        // Tax increase angers peasants and burghers
        if (policy_type == "tax_increase") {
            AdjustSatisfaction(entity_id, FactionType::PEASANTS, -0.15, "Tax increase");
            AdjustSatisfaction(entity_id, FactionType::BURGHERS, -0.1, "Tax increase");
            AdjustSatisfaction(entity_id, FactionType::MERCHANTS, -0.1, "Tax increase");
        }

        // Tax decrease pleases everyone except bureaucrats
        if (policy_type == "tax_decrease") {
            AdjustSatisfaction(entity_id, FactionType::PEASANTS, 0.15, "Tax decrease");
            AdjustSatisfaction(entity_id, FactionType::BURGHERS, 0.1, "Tax decrease");
            AdjustSatisfaction(entity_id, FactionType::MERCHANTS, 0.1, "Tax decrease");
            AdjustSatisfaction(entity_id, FactionType::BUREAUCRATS, -0.05, "Tax decrease");
        }

        // Religious policy changes affect clergy
        if (policy_type == "religious_tolerance") {
            AdjustSatisfaction(entity_id, FactionType::CLERGY, -0.1, "Religious tolerance policy");
            AdjustSatisfaction(entity_id, FactionType::PEASANTS, 0.05, "Religious tolerance policy");
        }

        if (policy_type == "religious_enforcement") {
            AdjustSatisfaction(entity_id, FactionType::CLERGY, 0.15, "Religious enforcement");
            AdjustSatisfaction(entity_id, FactionType::PEASANTS, -0.05, "Religious enforcement");
        }
    }

} // namespace game::faction
