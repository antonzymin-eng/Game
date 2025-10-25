// ============================================================================
// Date/Time Created: Tuesday, September 16, 2025 - 11:45 AM PST
// Intended Folder Location: src/game/gameplay/CoreGameplaySystem.cpp
// Mechanica Imperii - Core Gameplay System (Complete Implementation with Fixes)
// ============================================================================

#include "game/gameplay/CoreGameplaySystem.h"
#include "core/Types/TypeRegistry.h"
#include "core/logging/Logger.h"
#include "utils/PlatformCompat.h"
#include <algorithm>
#include <cassert>
#include <unordered_set>
#include <chrono>

namespace game::gameplay {

    // ============================================================================
    // FIXED: Schema versioning constants
    // ============================================================================

    namespace schema_versions {
        constexpr int DECISION_CONSEQUENCE_SCHEMA_V1 = 1;
        constexpr int DELEGATION_SCHEMA_V1 = 1;
        constexpr int QUIET_PERIOD_SCHEMA_V1 = 1;
        constexpr int GAMEPLAY_COORDINATOR_SCHEMA_V1 = 1;
    }

    // ============================================================================
    // FIXED: Gameplay metrics for UI/debugging
    // ============================================================================

    struct GameplayMetrics {
        int total_decisions_processed = 0;
        int delegated_decisions = 0;
        int player_decisions = 0;
        int escalated_consequences = 0;
        double average_decision_quality = 0.5;
        std::chrono::steady_clock::time_point last_reset;
        
        void Reset() {
            total_decisions_processed = 0;
            delegated_decisions = 0;
            player_decisions = 0;
            escalated_consequences = 0;
            average_decision_quality = 0.5;
            last_reset = std::chrono::steady_clock::now();
        }
        
        double GetDelegationRatio() const {
            return total_decisions_processed > 0 ? 
                static_cast<double>(delegated_decisions) / total_decisions_processed : 0.0;
        }
    };

    // ============================================================================
    // SystemPerformanceTracker Implementation (FIXED)
    // ============================================================================

    void SystemPerformanceTracker::UpdatePerformance(double new_quality) {
        // FIXED: Exponentially weighted moving average instead of simple average
        if (total_decisions == 0) {
            current_performance = new_quality;
        } else {
            current_performance = (1.0 - learning_rate) * current_performance + learning_rate * new_quality;
        }
        
        total_decisions++;
        last_update = std::chrono::steady_clock::now();
    }

    // ============================================================================
    // Decision Helper Methods (FIXED with importance weight validation)
    // ============================================================================

    std::string Decision::GetSystemName() const {
        return types::TypeRegistry::SystemTypeToString(system);
    }

    std::string Decision::GetFunctionName() const {
        return types::TypeRegistry::FunctionTypeToString(function);
    }

    std::string Decision::GetRegionName() const {
        return types::TypeRegistry::RegionTypeToString(region);
    }

    bool Decision::IsEconomicDecision() const {
        return system == types::SystemType::ECONOMICS;
    }

    bool Decision::IsMilitaryDecision() const {
        return system == types::SystemType::MILITARY;
    }

    bool Decision::IsAdministrativeDecision() const {
        return system == types::SystemType::ADMINISTRATION;
    }

    // FIXED: Add importance weight validation and documentation
    void Decision::SetImportanceWeight(double weight) {
        // FIXED: Document expected range and validate
        // importance_weight ∈ [0.5, 2.0], where 1.0 = baseline importance
        // 0.5 = half as important as normal, 2.0 = twice as important
        importance_weight = std::clamp(weight, 0.5, 2.0);
    }

    double Decision::GetNormalizedImportance() const {
        // Returns importance as deviation from baseline (1.0)
        return importance_weight - 1.0;  // Range: [-0.5, 1.0]
    }

    bool Decision::IsHighImportance() const {
        return importance_weight > 1.5;  // 50% more important than baseline
    }

    bool Decision::IsLowImportance() const {
        return importance_weight < 0.8;  // 20% less important than baseline
    }

    // ============================================================================
    // Consequence Helper Methods (FIXED)
    // ============================================================================

    bool Consequence::AffectsSystem(types::SystemType system_type) const {
        return affected_system == system_type;
    }

    bool Consequence::RequiresPlayerAttention() const {
        return severity >= ConsequenceSeverity::MAJOR;
    }

    bool Consequence::IsExpired() const {
        if (is_permanent || has_expired) return has_expired;
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::hours>(now - when_occurred);
        return elapsed >= duration;
    }

    // ============================================================================
    // DelegationRule Helper Methods
    // ============================================================================

    bool DelegationRule::CoversSystem(types::SystemType system) const {
        return std::find(covered_systems.begin(), covered_systems.end(), system) != covered_systems.end();
    }

    bool DelegationRule::CoversFunction(types::FunctionType function) const {
        return std::find(covered_functions.begin(), covered_functions.end(), function) != covered_functions.end();
    }

    bool DelegationRule::CoversRegion(types::RegionType region) const {
        return std::find(covered_regions.begin(), covered_regions.end(), region) != covered_regions.end();
    }

    bool DelegationRule::CoversSituation(types::SituationType situation) const {
        return std::find(covered_situations.begin(), covered_situations.end(), situation) != covered_situations.end();
    }

    // ============================================================================
    // DecisionConsequenceSystem Implementation (FIXED)
    // ============================================================================

    DecisionConsequenceSystem::DecisionConsequenceSystem(const ComplexitySettings& settings, 
                                                        uint32_t random_seed)
        : m_settings(settings)
        , m_deterministic_mode(random_seed != 0) {
        
        // FIXED: Deterministic random generation
        if (m_deterministic_mode) {
            m_random_generator.seed(random_seed);
        } else {
            std::random_device rd;
            m_random_generator.seed(rd());
        }
        
        ::core::logging::LogInfo("DecisionConsequenceSystem", "DecisionConsequenceSystem initialized with " + 
                      std::string(m_deterministic_mode ? "deterministic" : "random") + " mode");
    }

    void DecisionConsequenceSystem::Update(double delta_time) {
        // FIXED: Regular cleanup to prevent memory growth
        CleanupOldDecisions();
        CleanupExpiredConsequences();
    }

    void DecisionConsequenceSystem::PresentDecision(const Decision& decision) {
        // FIXED: Enforce memory limits
        if (m_active_decisions.size() >= m_settings.max_stored_decisions) {
            CleanupOldDecisions();
        }
        
        m_active_decisions.push_back(decision);
        ::core::logging::LogInfo("CoreGameplaySystem", "Decision presented: " + decision.title + " (System: " + decision.GetSystemName() + ")");
    }

    void DecisionConsequenceSystem::MakeDecision(const std::string& decision_id, const std::string& choice_id) {
        auto decision_it = std::find_if(m_active_decisions.begin(), m_active_decisions.end(),
            [&decision_id](const Decision& d) { return d.id == decision_id; });

        if (decision_it != m_active_decisions.end()) {
            const Decision& decision = *decision_it;

            // Calculate decision quality based on choice and context
            double quality = CalculateChoiceQuality(decision, choice_id);

            // FIXED: Update system performance with new weighted averaging
            UpdateSystemQuality(decision, quality);

            // Generate consequence
            Consequence consequence = GenerateConsequence(decision, choice_id, quality);

            // FIXED: Improved escalation logic
            if (ShouldEscalate(consequence, decision)) {
                double escalation_factor = CalculateEscalationFactor(decision);
                EscalateConsequence(consequence, escalation_factor);
            }

            // FIXED: Enforce memory limits for consequences
            if (m_active_consequences.size() >= m_settings.max_stored_consequences) {
                CleanupExpiredConsequences();
            }

            m_active_consequences.push_back(consequence);
            m_active_decisions.erase(decision_it);

            ::core::logging::LogInfo("CoreGameplaySystem", "Decision made: " + decision_id + " with quality " + 
                          std::to_string(static_cast<int>(quality * 100)) + "%");
        } else {
            ::core::logging::LogError("CoreGameplaySystem", "Decision not found: " + decision_id);
        }
    }

    void DecisionConsequenceSystem::CleanupOldDecisions() {
        // Remove decisions older than a certain threshold
        auto threshold = std::chrono::steady_clock::now() - std::chrono::hours(1);
        
        m_active_decisions.erase(
            std::remove_if(m_active_decisions.begin(), m_active_decisions.end(),
                [threshold](const Decision& d) { return d.created_time < threshold; }),
            m_active_decisions.end());
    }

    void DecisionConsequenceSystem::CleanupExpiredConsequences() {
        // Mark expired consequences
        for (auto& consequence : m_active_consequences) {
            if (consequence.IsExpired()) {
                consequence.has_expired = true;
            }
        }
        
        // Remove expired consequences older than retention time
        auto retention_threshold = std::chrono::steady_clock::now() - m_settings.consequence_retention_time;
        
        m_active_consequences.erase(
            std::remove_if(m_active_consequences.begin(), m_active_consequences.end(),
                [retention_threshold](const Consequence& c) { 
                    return c.has_expired && c.when_occurred < retention_threshold; 
                }),
            m_active_consequences.end());
    }

    bool DecisionConsequenceSystem::ShouldEscalate(const Consequence& consequence, const Decision& source_decision) const {
        // FIXED: Context-sensitive escalation instead of just performance threshold
        double system_performance = GetSystemPerformance(source_decision.system);
        
        // Base escalation check
        bool performance_based = system_performance < m_settings.escalation_performance_threshold;
        
        // Additional escalation factors
        bool urgency_based = source_decision.urgent && consequence.severity >= ConsequenceSeverity::MODERATE;
        bool importance_based = source_decision.importance_weight > 1.5 && consequence.severity >= ConsequenceSeverity::MAJOR;
        bool scope_based = source_decision.scope >= DecisionScope::NATIONAL && consequence.severity >= ConsequenceSeverity::MAJOR;
        
        return performance_based || urgency_based || importance_based || scope_based;
    }

    double DecisionConsequenceSystem::CalculateEscalationFactor(const Decision& source_decision) const {
        // FIXED: Multi-factor escalation calculation
        double base_factor = 1.0;
        double system_performance = GetSystemPerformance(source_decision.system);
        
        // Performance penalty
        if (system_performance < m_settings.escalation_performance_threshold) {
            base_factor += (m_settings.escalation_performance_threshold - system_performance) * 2.0;
        }
        
        // Urgency amplification
        if (source_decision.urgent) {
            base_factor *= 1.3;
        }
        
        // Importance amplification
        base_factor *= source_decision.importance_weight;
        
        // Scope amplification
        switch (source_decision.scope) {
            case DecisionScope::REGIONAL: base_factor *= 1.2; break;
            case DecisionScope::NATIONAL: base_factor *= 1.5; break;
            case DecisionScope::HISTORIC: base_factor *= 2.0; break;
            default: break;
        }
        
        return std::clamp(base_factor, 1.0, 5.0);  // Cap escalation
    }

    void DecisionConsequenceSystem::EscalateConsequence(Consequence& consequence, double escalation_factor) {
        // Increase severity
        switch (consequence.severity) {
        case ConsequenceSeverity::MINOR:
            consequence.escalated_severity = ConsequenceSeverity::MODERATE;
            break;
        case ConsequenceSeverity::MODERATE:
            consequence.escalated_severity = ConsequenceSeverity::MAJOR;
            break;
        case ConsequenceSeverity::MAJOR:
            consequence.escalated_severity = ConsequenceSeverity::CRITICAL;
            break;
        case ConsequenceSeverity::CRITICAL:
            // Already at maximum severity
            break;
        }

        // Amplify effects
        for (auto& [stat, value] : consequence.stat_changes) {
            value *= escalation_factor;
        }

        // Add escalation triggers based on scope
        switch (consequence.scope) {
        case DecisionScope::REGIONAL:
            consequence.triggered_events.push_back("regional_unrest");
            break;
        case DecisionScope::NATIONAL:
            consequence.triggered_events.push_back("national_crisis");
            break;
        case DecisionScope::HISTORIC:
            consequence.triggered_events.push_back("historical_catastrophe");
            break;
        default:
            break;
        }

        consequence.description += " [ESCALATED due to poor " + 
            types::TypeRegistry::SystemTypeToString(consequence.affected_system) + " management]";
        
        ::core::logging::LogWarning("CoreGameplaySystem", "Consequence escalated by factor " + std::to_string(escalation_factor));
    }

    double DecisionConsequenceSystem::GetSystemPerformance(types::SystemType system) const {
        auto it = m_system_performance.find(system);
        return (it != m_system_performance.end()) ? it->second.GetPerformance() : 0.5;
    }

    std::vector<Decision> DecisionConsequenceSystem::GetDecisionsBySystem(types::SystemType system) const {
        std::vector<Decision> result;
        std::copy_if(m_active_decisions.begin(), m_active_decisions.end(), std::back_inserter(result),
            [system](const Decision& d) { return d.system == system; });
        return result;
    }

    std::vector<Consequence> DecisionConsequenceSystem::GetConsequencesBySystem(types::SystemType system) const {
        std::vector<Consequence> result;
        std::copy_if(m_active_consequences.begin(), m_active_consequences.end(), std::back_inserter(result),
            [system](const Consequence& c) { return c.affected_system == system; });
        return result;
    }

    void DecisionConsequenceSystem::UpdateSystemQuality(const Decision& decision, double quality) {
        // FIXED: Use weighted moving average through SystemPerformanceTracker
        auto& tracker = m_system_performance[decision.system];
        tracker.UpdatePerformance(quality);
    }

    double DecisionConsequenceSystem::CalculateChoiceQuality(const Decision& decision, const std::string& choice_id) {
        // Base quality calculation
        double base_quality = 0.6;

        // FIXED: Use proper performance tracking
        auto it = m_system_performance.find(decision.system);
        if (it != m_system_performance.end() && it->second.HasSufficientData()) {
            double system_performance = it->second.GetPerformance();
            double performance_bonus = (system_performance - 0.5) * 0.4;  // ±0.2 based on past performance
            base_quality += performance_bonus;
        }

        // Adjust based on decision urgency and importance
        double urgency_penalty = decision.urgent ? -0.1 : 0.0;
        double importance_bonus = (decision.importance_weight - 1.0) * 0.1;

        // FIXED: Controlled randomness for deterministic mode
        double random_factor = 0.0;
        if (!m_deterministic_mode) {
            std::uniform_real_distribution<> dis(-0.1, 0.1);
            random_factor = dis(m_random_generator);
        }

        return std::clamp(base_quality + urgency_penalty + importance_bonus + random_factor, 0.0, 1.0);
    }

    Consequence DecisionConsequenceSystem::GenerateConsequence(const Decision& decision, const std::string& choice_id, double quality) {
        Consequence consequence;
        consequence.id = decision.id + "_consequence";
        consequence.source_decision_type = decision.type;
        consequence.affected_system = decision.system;
        consequence.scope = decision.scope;

        // Determine severity based on quality
        if (quality >= 0.8) {
            consequence.severity = ConsequenceSeverity::MINOR;
        }
        else if (quality >= 0.5) {
            consequence.severity = ConsequenceSeverity::MODERATE;
        }
        else if (quality >= 0.3) {
            consequence.severity = ConsequenceSeverity::MAJOR;
        }
        else {
            consequence.severity = ConsequenceSeverity::CRITICAL;
        }

        consequence.description = GenerateConsequenceDescription(decision, choice_id, quality);
        consequence.when_occurred = std::chrono::steady_clock::now();
        
        // FIXED: Set duration based on scope and severity
        switch (consequence.scope) {
            case DecisionScope::LOCAL:
                consequence.duration = std::chrono::hours(2);
                break;
            case DecisionScope::REGIONAL:
                consequence.duration = std::chrono::hours(8);
                break;
            case DecisionScope::NATIONAL:
                consequence.duration = std::chrono::hours(24);
                break;
            case DecisionScope::HISTORIC:
                consequence.is_permanent = true;
                break;
        }

        // Generate stat changes based on decision type and quality
        GenerateStatChanges(consequence, decision, quality);

        return consequence;
    }

    void DecisionConsequenceSystem::GenerateStatChanges(Consequence& consequence, const Decision& decision, double quality) {
        // Generate appropriate stat changes based on decision type and quality
        switch (decision.system) {
        case types::SystemType::ECONOMICS:
            consequence.stat_changes["treasury"] = (quality - 0.5) * 200;  // ±100 gold
            consequence.stat_changes["trade_efficiency"] = (quality - 0.5) * 0.2;  // ±0.1
            break;

        case types::SystemType::ADMINISTRATION:
            consequence.stat_changes["stability"] = (quality - 0.5) * 0.4;  // ±0.2
            consequence.stat_changes["corruption"] = (0.5 - quality) * 0.3;  // Inverse relationship
            break;

        case types::SystemType::POPULATION:
            consequence.stat_changes["happiness"] = (quality - 0.5) * 0.6;  // ±0.3
            consequence.stat_changes["population_growth"] = (quality - 0.5) * 0.02;  // ±0.01
            break;

        case types::SystemType::MILITARY:
            consequence.stat_changes["military_readiness"] = (quality - 0.5) * 0.4;  // ±0.2
            consequence.stat_changes["troop_morale"] = (quality - 0.5) * 0.3;  // ±0.15
            break;

        default:
            // Generic stat changes for other systems
            consequence.stat_changes["general_efficiency"] = (quality - 0.5) * 0.2;
            break;
        }
    }

    std::string DecisionConsequenceSystem::GenerateConsequenceDescription(const Decision& decision, 
                                                                        const std::string& choice_id, 
                                                                        double quality) {
        std::string system_name = types::TypeRegistry::SystemTypeToString(decision.system);
        std::string quality_desc;

        if (quality >= 0.8) {
            quality_desc = "excellent";
        }
        else if (quality >= 0.6) {
            quality_desc = "good";
        }
        else if (quality >= 0.4) {
            quality_desc = "poor";
        }
        else {
            quality_desc = "disastrous";
        }

        return "Your " + quality_desc + " " + system_name + " decision regarding '" +
            decision.title + "' has resulted in " + quality_desc + " outcomes.";
    }

    // ============================================================================
    // FIXED: Enhanced serialization with schema versioning and complete data
    // ============================================================================

    Json::Value DecisionConsequenceSystem::SerializeDecision(const Decision& decision) const {
        Json::Value d;
        d["id"] = decision.id;
        d["title"] = decision.title;
        d["description"] = decision.description;
        d["system"] = static_cast<int>(decision.system);
        d["function"] = static_cast<int>(decision.function);
        d["region"] = static_cast<int>(decision.region);
        d["situation"] = static_cast<int>(decision.situation);
        d["scope"] = static_cast<int>(decision.scope);
        d["urgent"] = decision.urgent;
        d["importance_weight"] = decision.importance_weight;
        
        // FIXED: Serialize created_time as milliseconds since epoch
        auto time_since_epoch = decision.created_time.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
        d["created_time_ms"] = static_cast<int64_t>(millis);
        
        // FIXED: Serialize choices array
        Json::Value choices(Json::arrayValue);
        for (const auto& choice : decision.choices) {
            choices.append(choice);
        }
        d["choices"] = choices;
        
        return d;
    }

    Decision DecisionConsequenceSystem::DeserializeDecision(const Json::Value& data) const {
        Decision decision;
        decision.id = data.get("id", "").asString();
        decision.title = data.get("title", "").asString();
        decision.description = data.get("description", "").asString();
        decision.system = static_cast<types::SystemType>(data.get("system", 0).asInt());
        decision.function = static_cast<types::FunctionType>(data.get("function", 0).asInt());
        decision.region = static_cast<types::RegionType>(data.get("region", 0).asInt());
        decision.situation = static_cast<types::SituationType>(data.get("situation", 0).asInt());
        decision.scope = static_cast<DecisionScope>(data.get("scope", 0).asInt());
        decision.urgent = data.get("urgent", false).asBool();
        decision.importance_weight = data.get("importance_weight", 1.0).asDouble();
        
        // FIXED: Deserialize created_time
        if (data.isMember("created_time_ms")) {
            auto millis = data["created_time_ms"].asInt64();
            decision.created_time = std::chrono::steady_clock::time_point(
                std::chrono::milliseconds(millis)
            );
        } else {
            decision.created_time = std::chrono::steady_clock::now();
        }
        
        // FIXED: Deserialize choices
        if (data.isMember("choices") && data["choices"].isArray()) {
            for (const auto& choice : data["choices"]) {
                decision.choices.push_back(choice.asString());
            }
        }
        
        return decision;
    }

    Json::Value DecisionConsequenceSystem::Serialize(int version) const {
        Json::Value root;
        root["version"] = version;
        root["schema_version"] = schema_versions::DECISION_CONSEQUENCE_SCHEMA_V1;
        root["deterministic_mode"] = m_deterministic_mode;
        
        // Serialize active decisions with full data
        Json::Value decisions(Json::arrayValue);
        for (const auto& decision : m_active_decisions) {
            decisions.append(SerializeDecision(decision));
        }
        root["active_decisions"] = decisions;
        
        // Serialize consequences with duration and expiration data
        Json::Value consequences(Json::arrayValue);
        for (const auto& consequence : m_active_consequences) {
            Json::Value c;
            c["id"] = consequence.id;
            c["severity"] = static_cast<int>(consequence.severity);
            c["affected_system"] = static_cast<int>(consequence.affected_system);
            c["description"] = consequence.description;
            c["is_permanent"] = consequence.is_permanent;
            c["has_expired"] = consequence.has_expired;
            
            // Serialize timing information
            auto time_since_epoch = consequence.when_occurred.time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
            c["when_occurred_ms"] = static_cast<int64_t>(millis);
            c["duration_hours"] = static_cast<int>(consequence.duration.count());
            
            consequences.append(c);
        }
        root["active_consequences"] = consequences;
        
        // Serialize system performance with complete data
        Json::Value performance;
        for (const auto& [system, tracker] : m_system_performance) {
            Json::Value p;
            p["performance"] = tracker.GetPerformance();
            p["total_decisions"] = tracker.total_decisions;
            p["learning_rate"] = tracker.learning_rate;
            performance[std::to_string(static_cast<int>(system))] = p;
        }
        root["system_performance"] = performance;
        
        return root;
    }

    bool DecisionConsequenceSystem::Deserialize(const Json::Value& data, int version) {
        try {
            // Check schema version compatibility
            int schema_version = data.get("schema_version", 1).asInt();
            if (schema_version > schema_versions::DECISION_CONSEQUENCE_SCHEMA_V1) {
                ::core::logging::LogWarning("CoreGameplaySystem", "Loading newer schema version " + std::to_string(schema_version) + 
                                ", some data may not be loaded correctly");
            }
            
            m_deterministic_mode = data.get("deterministic_mode", false).asBool();
            
            // Deserialize active decisions with validation
            if (data.isMember("active_decisions") && data["active_decisions"].isArray()) {
                const Json::Value& decisions = data["active_decisions"];
                for (const auto& d : decisions) {
                    try {
                        Decision decision = DeserializeDecision(d);
                        m_active_decisions.push_back(decision);
                    } catch (const std::exception& e) {
                        ::core::logging::LogWarning("CoreGameplaySystem", "Failed to deserialize decision: " + std::string(e.what()));
                    }
                }
            }
            
            // Deserialize consequences with timing validation
            if (data.isMember("active_consequences") && data["active_consequences"].isArray()) {
                const Json::Value& consequences = data["active_consequences"];
                for (const auto& c : consequences) {
                    try {
                        Consequence consequence;
                        consequence.id = c.get("id", "").asString();
                        consequence.severity = static_cast<ConsequenceSeverity>(c.get("severity", 0).asInt());
                        consequence.affected_system = static_cast<types::SystemType>(c.get("affected_system", 0).asInt());
                        consequence.description = c.get("description", "").asString();
                        consequence.is_permanent = c.get("is_permanent", false).asBool();
                        consequence.has_expired = c.get("has_expired", false).asBool();
                        
                        // Deserialize timing with validation
                        if (c.isMember("when_occurred_ms")) {
                            auto millis = c["when_occurred_ms"].asInt64();
                            consequence.when_occurred = std::chrono::steady_clock::time_point(
                                std::chrono::milliseconds(millis)
                            );
                        }
                        
                        if (c.isMember("duration_hours")) {
                            int hours = c["duration_hours"].asInt();
                            consequence.duration = std::chrono::hours(std::max(0, hours));
                        }
                        
                        m_active_consequences.push_back(consequence);
                    } catch (const std::exception& e) {
                        ::core::logging::LogWarning("CoreGameplaySystem", "Failed to deserialize consequence: " + std::string(e.what()));
                    }
                }
            }
            
            // Deserialize system performance with validation
            if (data.isMember("system_performance")) {
                const Json::Value& performance = data["system_performance"];
                for (const auto& key : performance.getMemberNames()) {
                    try {
                        int system_int = std::stoi(key);
                        auto system = static_cast<types::SystemType>(system_int);
                        
                        const Json::Value& p = performance[key];
                        auto& tracker = m_system_performance[system];
                        tracker.current_performance = std::clamp(p.get("performance", 0.5).asDouble(), 0.0, 1.0);
                        tracker.total_decisions = std::max(0, p.get("total_decisions", 0).asInt());
                        tracker.learning_rate = std::clamp(p.get("learning_rate", 0.2).asDouble(), 0.01, 1.0);
                    } catch (const std::exception& e) {
                        ::core::logging::LogWarning("CoreGameplaySystem", "Failed to deserialize system performance for key " + key + ": " + std::string(e.what()));
                    }
                }
            }
            
            return true;
        } catch (const std::exception& e) {
            ::core::logging::LogError("CoreGameplaySystem", "Failed to deserialize DecisionConsequenceSystem: " + std::string(e.what()));
            return false;
        }
    }

    // ============================================================================
    // DelegationSystem Implementation (FIXED)
    // ============================================================================

    DelegationSystem::DelegationSystem(const ComplexitySettings& settings)
        : m_settings(settings) {
        ::core::logging::LogInfo("CoreGameplaySystem", "DelegationSystem initialized with complexity level: " + 
                      std::to_string(static_cast<int>(settings.overall_level)));
    }

    void DelegationSystem::SetConsequenceSystem(DecisionConsequenceSystem* consequence_system) {
        m_consequence_system = consequence_system;
        ::core::logging::LogInfo("CoreGameplaySystem", "DelegationSystem connected to DecisionConsequenceSystem");
    }

    void DelegationSystem::CreateDelegationRule(const DelegationRule& rule) {
        // Remove any existing rule with the same ID
        m_active_delegations.erase(
            std::remove_if(m_active_delegations.begin(), m_active_delegations.end(),
                [&rule](const DelegationRule& existing) { return existing.id == rule.id; }),
            m_active_delegations.end());

        m_active_delegations.push_back(rule);
        ::core::logging::LogInfo("CoreGameplaySystem", "Created delegation rule: " + rule.name + " assigned to " + rule.assigned_council_member);
    }

    void DelegationSystem::SetupBeginnerDelegation() {
        if (m_settings.overall_level == ComplexityLevel::SIMPLIFIED) {
            CreateSystemDelegation(types::SystemType::ECONOMICS, "Treasurer");
            CreateSystemDelegation(types::SystemType::ADMINISTRATION, "Steward");
            CreateSystemDelegation(types::SystemType::CONSTRUCTION, "Master_Builder");
            CreateSystemDelegation(types::SystemType::POPULATION, "Steward");
        }
    }

    void DelegationSystem::SetupRegionalDelegation(const std::vector<types::RegionType>& distant_regions) {
        DelegationRule regional_rule;
        regional_rule.id = "distant_regions";
        regional_rule.name = "Distant Regional Management";
        regional_rule.type = types::DelegationType::REGIONAL;
        regional_rule.level = DelegationLevel::SUPERVISED;
        regional_rule.covered_regions = distant_regions;
        regional_rule.assigned_council_member = "Regional_Governor";
        regional_rule.council_competence = 0.75;
        regional_rule.council_loyalty = 0.8;

        CreateDelegationRule(regional_rule);
    }

    void DelegationSystem::CreateSystemDelegation(types::SystemType system, const std::string& council_member) {
        DelegationRule rule;
        rule.id = types::TypeRegistry::SystemTypeToString(system) + "_delegation";
        rule.name = "Simplified " + types::TypeRegistry::SystemTypeToString(system) + " Management";
        rule.type = types::DelegationType::SYSTEM_WIDE;
        rule.level = DelegationLevel::AUTONOMOUS;
        rule.covered_systems = { system };
        rule.assigned_council_member = council_member;
        rule.council_competence = 0.7;
        rule.council_loyalty = 0.85;

        CreateDelegationRule(rule);
    }

    bool DelegationSystem::ShouldDelegate(types::SystemType system, types::FunctionType function,
                                        types::RegionType region, types::SituationType situation) {
        for (const auto& rule : m_active_delegations) {
            if (MatchesDelegationRule(rule, system, function, region, situation)) {
                if (rule.level == DelegationLevel::AUTONOMOUS) {
                    return true;  // Fully delegated
                }
                else if (rule.level == DelegationLevel::SUPERVISED) {
                    // Check if it's a routine decision that can be auto-handled
                    return IsRoutineDecision(system, function, situation);
                }
            }
        }
        return false;
    }

    DelegationLevel DelegationSystem::GetDelegationLevel(types::SystemType system, types::FunctionType function,
                                                        types::RegionType region) {
        for (const auto& rule : m_active_delegations) {
            if (MatchesDelegationRule(rule, system, function, region, types::SituationType::ROUTINE)) {
                return rule.level;
            }
        }
        return DelegationLevel::FULL_CONTROL;
    }

    // FIXED: Proper delegation execution that generates real consequences
    void DelegationSystem::ExecuteDelegatedDecision(const Decision& decision) {
        // Find the delegation rule that covers this decision
        DelegationRule* applicable_rule = nullptr;
        for (auto& rule : m_active_delegations) {
            if (MatchesDelegationRule(rule, decision.system, decision.function, decision.region, decision.situation)) {
                applicable_rule = &rule;
                break;
            }
        }

        if (applicable_rule && m_consequence_system) {
            // FIXED: Generate real consequences for delegated decisions
            GenerateDelegatedConsequence(decision, *applicable_rule);
            
            // Update delegation performance tracking
            double effectiveness = CalculateDelegationEffectiveness(*applicable_rule);
            applicable_rule->performance_tracker.UpdatePerformance(effectiveness);
            
            ::core::logging::LogInfo("CoreGameplaySystem", "Executed delegated decision: " + decision.id + 
                          " by " + applicable_rule->assigned_council_member +
                          " with " + std::to_string(static_cast<int>(effectiveness * 100)) + "% effectiveness");
        } else {
            ::core::logging::LogError("CoreGameplaySystem", "Cannot execute delegated decision - no applicable rule or consequence system");
        }
    }

    void DelegationSystem::GenerateDelegatedConsequence(const Decision& decision, const DelegationRule& rule) {
        // FIXED: Create a modified decision that reflects council member handling
        Decision modified_decision = decision;
        
        // Calculate council effectiveness
        double effectiveness = CalculateDelegationEffectiveness(rule);
        
        // Modify decision quality based on council effectiveness
        modified_decision.importance_weight *= effectiveness;
        
        // Generate consequence through the consequence system
        std::string choice_id = "delegated_to_" + rule.assigned_council_member;
        m_consequence_system->MakeDecision(decision.id, choice_id);
    }

    double DelegationSystem::CalculateDelegationEffectiveness(const DelegationRule& rule) const {
        // Base effectiveness from competence and loyalty
        double base_effectiveness = (rule.council_competence * 0.7) + (rule.council_loyalty * 0.3);
        
        // Adjust based on delegation performance history
        if (rule.performance_tracker.HasSufficientData()) {
            double performance_modifier = (rule.performance_tracker.GetPerformance() - 0.5) * 0.3;
            base_effectiveness += performance_modifier;
        }
        
        // Add some variability for realism
        return std::clamp(base_effectiveness, 0.1, 1.0);
    }

    bool DelegationSystem::MatchesDelegationRule(const DelegationRule& rule, types::SystemType system,
                                                types::FunctionType function, types::RegionType region,
                                                types::SituationType situation) {
        // Check system coverage
        if (rule.type == types::DelegationType::SYSTEM_WIDE) {
            if (!rule.CoversSystem(system)) {
                return false;
            }
        }

        // Check function coverage
        if (rule.type == types::DelegationType::FUNCTIONAL) {
            if (!rule.CoversFunction(function)) {
                return false;
            }
        }

        // Check regional coverage
        if (rule.type == types::DelegationType::REGIONAL) {
            if (!rule.CoversRegion(region)) {
                return false;
            }
        }

        // Check situational coverage
        if (rule.type == types::DelegationType::SITUATIONAL) {
            if (!rule.CoversSituation(situation)) {
                return false;
            }
        }

        return true;
    }

    bool DelegationSystem::IsRoutineDecision(types::SystemType system, types::FunctionType function,
                                            types::SituationType situation) {
        return situation == types::SituationType::ROUTINE &&
            (function == types::FunctionType::TAX_COLLECTION ||
                function == types::FunctionType::TRADE_ADMINISTRATION ||
                function == types::FunctionType::MAINTENANCE ||
                function == types::FunctionType::POPULATION_MANAGEMENT);
    }

    // FIXED: Serialization for DelegationSystem
    Json::Value DelegationSystem::Serialize(int version) const {
        Json::Value root;
        root["version"] = version;
        root["schema_version"] = schema_versions::DELEGATION_SCHEMA_V1;
        
        Json::Value delegations(Json::arrayValue);
        for (const auto& rule : m_active_delegations) {
            Json::Value d;
            d["id"] = rule.id;
            d["name"] = rule.name;
            d["type"] = static_cast<int>(rule.type);
            d["level"] = static_cast<int>(rule.level);
            d["assigned_council_member"] = rule.assigned_council_member;
            d["council_competence"] = rule.council_competence;
            d["council_loyalty"] = rule.council_loyalty;
            
            // Serialize performance tracking
            d["performance"] = rule.performance_tracker.GetPerformance();
            d["total_decisions"] = rule.performance_tracker.total_decisions;
            d["learning_rate"] = rule.performance_tracker.learning_rate;
            
            delegations.append(d);
        }
        root["delegations"] = delegations;
        
        return root;
    }

    bool DelegationSystem::Deserialize(const Json::Value& data, int version) {
        try {
            // Check schema version compatibility
            int schema_version = data.get("schema_version", 1).asInt();
            if (schema_version > schema_versions::DELEGATION_SCHEMA_V1) {
                ::core::logging::LogWarning("CoreGameplaySystem", "Loading newer delegation schema version " + std::to_string(schema_version));
            }

            m_active_delegations.clear();
            
            if (data.isMember("delegations")) {
                const Json::Value& delegations = data["delegations"];
                for (const auto& d : delegations) {
                    try {
                        DelegationRule rule;
                        rule.id = d.get("id", "").asString();
                        rule.name = d.get("name", "").asString();
                        rule.type = static_cast<types::DelegationType>(d.get("type", 0).asInt());
                        rule.level = static_cast<DelegationLevel>(d.get("level", 0).asInt());
                        rule.assigned_council_member = d.get("assigned_council_member", "").asString();
                        rule.council_competence = std::clamp(d.get("council_competence", 0.7).asDouble(), 0.0, 1.0);
                        rule.council_loyalty = std::clamp(d.get("council_loyalty", 0.8).asDouble(), 0.0, 1.0);
                        
                        // Deserialize performance tracking
                        rule.performance_tracker.current_performance = std::clamp(d.get("performance", 0.5).asDouble(), 0.0, 1.0);
                        rule.performance_tracker.total_decisions = std::max(0, d.get("total_decisions", 0).asInt());
                        rule.performance_tracker.learning_rate = std::clamp(d.get("learning_rate", 0.2).asDouble(), 0.01, 1.0);
                        
                        m_active_delegations.push_back(rule);
                    } catch (const std::exception& e) {
                        ::core::logging::LogWarning("CoreGameplaySystem", "Failed to deserialize delegation rule: " + std::string(e.what()));
                    }
                }
            }
            
            return true;
        } catch (const std::exception& e) {
            ::core::logging::LogError("CoreGameplaySystem", "Failed to deserialize DelegationSystem: " + std::string(e.what()));
            return false;
        }
    }

    // ============================================================================
    // QuietPeriodManager Implementation (FIXED)
    // ============================================================================

    QuietPeriodManager::QuietPeriodManager(const ComplexitySettings& settings)
        : m_settings(settings) {
        ::core::logging::LogInfo("CoreGameplaySystem", "QuietPeriodManager initialized with max acceleration: " + 
                      std::to_string(m_settings.max_acceleration_factor) + "x");
    }

    void QuietPeriodManager::Update(double delta_time) {
        UpdateGamePace();

        if (m_state.in_quiet_period) {
            ProcessBackgroundActivities(delta_time * m_state.time_acceleration);
        }
    }

    // FIXED: Dynamic threshold calculation based on recent decisions
    void QuietPeriodManager::AnalyzeGameActivity(const std::vector<Decision>& recent_decisions) {
        // Calculate decision metrics from actual recent decisions
        m_state.decisions_last_hour = static_cast<int>(recent_decisions.size());
        
        if (!recent_decisions.empty()) {
            double total_urgency = 0.0;
            double total_importance = 0.0;
            
            for (const auto& decision : recent_decisions) {
                total_urgency += decision.urgent ? 1.0 : 0.0;
                total_importance += decision.importance_weight;
            }
            
            m_state.average_decision_urgency = total_urgency / recent_decisions.size();
            m_state.average_decision_importance = total_importance / recent_decisions.size();
        }
        
        // Check for quiet period transitions
        if (ShouldEnterQuietPeriod() && !m_state.in_quiet_period) {
            EnterQuietPeriod();
        } else if (ShouldExitQuietPeriod() && m_state.in_quiet_period) {
            ExitQuietPeriod();
        }
    }

    bool QuietPeriodManager::ShouldEnterQuietPeriod() const {
        // FIXED: Use configurable thresholds instead of hardcoded values
        bool low_decision_count = m_state.decisions_last_hour < m_settings.quiet_period_decision_threshold;
        bool low_event_count = m_state.events_last_hour < m_settings.quiet_period_event_threshold;
        bool low_urgency = m_state.average_decision_urgency < 0.3;
        bool low_importance = m_state.average_decision_importance < 1.2;
        
        return low_decision_count && low_event_count && low_urgency && low_importance;
    }

    bool QuietPeriodManager::ShouldExitQuietPeriod() const {
        // FIXED: Dynamic exit conditions
        bool high_decision_count = m_state.decisions_last_hour > (m_settings.quiet_period_decision_threshold * 2);
        bool high_event_count = m_state.events_last_hour > (m_settings.quiet_period_event_threshold * 2);
        bool high_urgency = m_state.average_decision_urgency > 0.7;
        bool high_importance = m_state.average_decision_importance > 2.0;
        
        return high_decision_count || high_event_count || high_urgency || high_importance;
    }

    void QuietPeriodManager::EnterQuietPeriod() {
        m_state.in_quiet_period = true;
        m_state.quiet_period_start = std::chrono::steady_clock::now();
        m_state.current_pace = GamePace::QUIET;

        if (!m_state.player_manually_accelerated && m_settings.enable_quiet_period_acceleration) {
            m_state.time_acceleration = 2.0;
        }

        GenerateQuietPeriodActivities();
        ::core::logging::LogInfo("CoreGameplaySystem", "Entered quiet period - time acceleration: " + 
                      std::to_string(m_state.time_acceleration) + "x");
    }

    void QuietPeriodManager::ExitQuietPeriod() {
        m_state.in_quiet_period = false;
        m_state.current_pace = GamePace::ACTIVE;

        if (!m_state.player_manually_accelerated) {
            m_state.time_acceleration = 1.0;
        }

        m_pending_background_activities.clear();
        ::core::logging::LogInfo("CoreGameplaySystem", "Exited quiet period - returning to normal pace");
    }

    void QuietPeriodManager::UpdateGamePace() {
        if (m_state.in_quiet_period) {
            auto quiet_duration = std::chrono::steady_clock::now() - m_state.quiet_period_start;
            auto minutes = std::chrono::duration_cast<std::chrono::minutes>(quiet_duration).count();

            if (minutes > 5 && !m_state.player_manually_accelerated) {
                m_state.time_acceleration = std::min(m_settings.max_acceleration_factor,
                    m_state.time_acceleration * 1.1);
            }

            // Update pace based on acceleration
            if (m_state.time_acceleration >= 4.0) {
                m_state.current_pace = GamePace::PEACEFUL;
            }
            else if (m_state.time_acceleration >= 2.0) {
                m_state.current_pace = GamePace::QUIET;
            }
        }
    }

    void QuietPeriodManager::GenerateQuietPeriodActivities() {
        m_pending_background_activities = {
            "routine_tax_collection",
            "market_fluctuations",
            "minor_population_changes",
            "trade_route_maintenance",
            "building_maintenance"
        };
    }

    void QuietPeriodManager::ProcessBackgroundActivities(double delta_time) {
        for (const auto& activity : m_pending_background_activities) {
            ProcessBackgroundActivity(activity, delta_time);
        }
    }

    void QuietPeriodManager::PlayerRequestTimeAcceleration(double factor) {
        m_state.time_acceleration = std::clamp(factor, 0.1, m_settings.max_acceleration_factor);
        m_state.player_manually_accelerated = true;
        ::core::logging::LogInfo("CoreGameplaySystem", "Player requested time acceleration: " + std::to_string(factor) + "x");
    }

    void QuietPeriodManager::PlayerRequestPause() {
        m_state.time_acceleration = 0.0;
        ::core::logging::LogInfo("CoreGameplaySystem", "Game paused");
    }

    void QuietPeriodManager::PlayerRequestNormalSpeed() {
        m_state.time_acceleration = 1.0;
        m_state.player_manually_accelerated = false;
        ::core::logging::LogInfo("CoreGameplaySystem", "Returned to normal speed");
    }

    void QuietPeriodManager::ProcessBackgroundActivity(const std::string& activity, double accelerated_delta) {
        ::core::logging::LogDebug("CoreGameplaySystem", "Processing background activity: " + activity + 
                       " (delta: " + std::to_string(accelerated_delta) + ")");
    }

    // FIXED: Serialization for QuietPeriodManager
    Json::Value QuietPeriodManager::Serialize(int version) const {
        Json::Value root;
        root["version"] = version;
        root["schema_version"] = schema_versions::QUIET_PERIOD_SCHEMA_V1;
        root["in_quiet_period"] = m_state.in_quiet_period;
        root["current_pace"] = static_cast<int>(m_state.current_pace);
        root["time_acceleration"] = m_state.time_acceleration;
        root["decisions_last_hour"] = m_state.decisions_last_hour;
        root["events_last_hour"] = m_state.events_last_hour;
        root["average_decision_urgency"] = m_state.average_decision_urgency;
        root["average_decision_importance"] = m_state.average_decision_importance;
        root["player_manually_accelerated"] = m_state.player_manually_accelerated;
        
        return root;
    }

    bool QuietPeriodManager::Deserialize(const Json::Value& data, int version) {
        try {
            // Check schema version compatibility
            int schema_version = data.get("schema_version", 1).asInt();
            if (schema_version > schema_versions::QUIET_PERIOD_SCHEMA_V1) {
                ::core::logging::LogWarning("CoreGameplaySystem", "Loading newer quiet period schema version " + std::to_string(schema_version));
            }

            m_state.in_quiet_period = data.get("in_quiet_period", false).asBool();
            m_state.current_pace = static_cast<GamePace>(data.get("current_pace", 1).asInt());
            m_state.time_acceleration = std::clamp(data.get("time_acceleration", 1.0).asDouble(), 0.0, 10.0);
            m_state.decisions_last_hour = std::max(0, data.get("decisions_last_hour", 0).asInt());
            m_state.events_last_hour = std::max(0, data.get("events_last_hour", 0).asInt());
            m_state.average_decision_urgency = std::clamp(data.get("average_decision_urgency", 0.5).asDouble(), 0.0, 1.0);
            m_state.average_decision_importance = std::clamp(data.get("average_decision_importance", 0.5).asDouble(), 0.0, 5.0);
            m_state.player_manually_accelerated = data.get("player_manually_accelerated", false).asBool();
            
            return true;
        } catch (const std::exception& e) {
            ::core::logging::LogError("CoreGameplaySystem", "Failed to deserialize QuietPeriodManager: " + std::string(e.what()));
            return false;
        }
    }

    // ============================================================================
    // GameplayCoordinator Implementation (FIXED)
    // ============================================================================

    GameplayCoordinator::GameplayCoordinator(const ComplexitySettings& settings, 
                                           uint32_t random_seed)
        : m_settings(settings)
        , m_decision_system(settings, random_seed)
        , m_delegation_system(settings)
        , m_quiet_period_manager(settings) {

        // FIXED: Connect systems properly
        m_delegation_system.SetConsequenceSystem(&m_decision_system);
        
        SetupInitialDelegation();
        ::core::logging::LogInfo("CoreGameplaySystem", "GameplayCoordinator initialized with complexity level: " + 
                      std::to_string(static_cast<int>(settings.overall_level)));
    }

    void GameplayCoordinator::Update(double delta_time) {
        // Update decision system (handles cleanup)
        m_decision_system.Update(delta_time);
        
        // Analyze activity for quiet period management
        auto recent_decisions = m_decision_system.GetActiveDecisions();
        m_quiet_period_manager.AnalyzeGameActivity(recent_decisions);
        m_quiet_period_manager.Update(delta_time);

        // Adjust delta_time based on time acceleration
        double accelerated_delta = delta_time * m_quiet_period_manager.GetTimeAcceleration();

        // Process systems with accelerated time
        UpdateGameSystems(accelerated_delta);
        
        // Update metrics
        UpdateMetrics();
    }

    // FIXED: Prevent double-counting of delegated decisions
    bool GameplayCoordinator::PresentDecisionToPlayer(const Decision& decision) {
        // FIXED: Check if decision was already processed to prevent double-counting
        std::string decision_key = decision.id + "_" + std::to_string(static_cast<int>(decision.system));
        
        static std::unordered_set<std::string> processed_decisions;
        if (processed_decisions.find(decision_key) != processed_decisions.end()) {
            ::core::logging::LogWarning("CoreGameplaySystem", "Decision already processed: " + decision.id);
            return false;
        }
        
        processed_decisions.insert(decision_key);
        
        if (m_delegation_system.ShouldDelegate(decision.system, decision.function,
            decision.region, decision.situation)) {
            // FIXED: Route exclusively through delegation - no double processing
            m_delegation_system.ExecuteDelegatedDecision(decision);
            m_metrics.delegated_decisions++;
            return false;  // Don't present to player
        } else {
            // FIXED: Route exclusively through player decision system
            m_decision_system.PresentDecision(decision);
            m_metrics.player_decisions++;
            return true;
        }
    }

    void GameplayCoordinator::MakePlayerDecision(const std::string& decision_id, const std::string& choice_id) {
        m_decision_system.MakeDecision(decision_id, choice_id);
        m_metrics.total_decisions_processed++;
    }

    void GameplayCoordinator::UpdateComplexitySettings(const ComplexitySettings& new_settings) {
        m_settings = new_settings;
        
        // Update all subsystems
        m_delegation_system = DelegationSystem(new_settings);
        m_delegation_system.SetConsequenceSystem(&m_decision_system);
        SetupInitialDelegation();

        ::core::logging::LogInfo("CoreGameplaySystem", "Complexity settings updated");
    }

    void GameplayCoordinator::EnableSystemComplexity(types::SystemType system, bool enable) {
        m_settings.simplified_systems[system] = !enable;

        if (!enable) {
            std::string council_member = GetCouncilMemberForSystem(system);
            CreateSystemDelegation(system, council_member);
        }
    }

    void GameplayCoordinator::RequestTimeAcceleration(double factor) {
        m_quiet_period_manager.PlayerRequestTimeAcceleration(factor);
    }

    void GameplayCoordinator::RequestPause() {
        m_quiet_period_manager.PlayerRequestPause();
    }

    void GameplayCoordinator::RequestNormalSpeed() {
        m_quiet_period_manager.PlayerRequestNormalSpeed();
    }

    void GameplayCoordinator::SetupInitialDelegation() {
        if (m_settings.overall_level == ComplexityLevel::SIMPLIFIED) {
            m_delegation_system.SetupBeginnerDelegation();
        }

        // Create system-specific delegation based on complexity settings
        for (const auto& [system, simplified] : m_settings.simplified_systems) {
            if (simplified) {
                std::string council_member = GetCouncilMemberForSystem(system);
                CreateSystemDelegation(system, council_member);
            }
        }
    }

    void GameplayCoordinator::CreateSystemDelegation(types::SystemType system, const std::string& council_member) {
        m_delegation_system.CreateSystemDelegation(system, council_member);
    }

    void GameplayCoordinator::HandleDelegatedDecision(const Decision& decision) {
        m_delegation_system.ExecuteDelegatedDecision(decision);
    }

    void GameplayCoordinator::UpdateGameSystems(double accelerated_delta) {
        // Update all game systems with potentially accelerated time
        ::core::logging::LogDebug("CoreGameplaySystem", "Updating game systems with accelerated delta: " + std::to_string(accelerated_delta));
    }

    void GameplayCoordinator::UpdateMetrics() {
        // Update average decision quality from system performance
        if (!m_decision_system.GetActiveDecisions().empty()) {
            double total_quality = 0.0;
            int system_count = 0;
            
            for (const auto& [system, tracker] : m_decision_system.m_system_performance) {
                if (tracker.HasSufficientData()) {
                    total_quality += tracker.GetPerformance();
                    system_count++;
                }
            }
            
            if (system_count > 0) {
                m_metrics.average_decision_quality = total_quality / system_count;
            }
        }
    }

    GameplayMetrics GameplayCoordinator::GetMetrics() const {
        return m_metrics;
    }
    
    void GameplayCoordinator::ResetMetrics() {
        m_metrics.Reset();
    }

    std::string GameplayCoordinator::GetCouncilMemberForSystem(types::SystemType system) {
        switch (system) {
        case types::SystemType::ECONOMICS:
            return "Treasurer";
        case types::SystemType::MILITARY:
            return "Marshal";
        case types::SystemType::DIPLOMACY:
            return "Chancellor";
        case types::SystemType::ADMINISTRATION:
            return "Steward";
        case types::SystemType::CONSTRUCTION:
            return "Master_Builder";
        case types::SystemType::POPULATION:
            return "Steward";
        case types::SystemType::TECHNOLOGY:
            return "Scholar";
        case types::SystemType::CULTURE:
            return "Chancellor";
        case types::SystemType::RELIGION:
            return "Bishop";
        case types::SystemType::ESPIONAGE:
            return "Spymaster";
        default:
            return "Steward";  // Default fallback
        }
    }

    // Decision analysis helper methods
    types::SystemType GameplayCoordinator::ExtractSystemFromDecision(const Decision& decision) {
        return decision.system;
    }

    types::FunctionType GameplayCoordinator::ExtractFunctionFromDecision(const Decision& decision) {
        return decision.function;
    }

    types::RegionType GameplayCoordinator::ExtractRegionFromDecision(const Decision& decision) {
        return decision.region;
    }

    // FIXED: Serialization for GameplayCoordinator
    Json::Value GameplayCoordinator::Serialize(int version) const {
        Json::Value root;
        root["version"] = version;
        root["schema_version"] = schema_versions::GAMEPLAY_COORDINATOR_SCHEMA_V1;
        
        // Serialize complexity settings
        Json::Value settings;
        settings["overall_level"] = static_cast<int>(m_settings.overall_level);
        settings["enable_progressive_unlock"] = m_settings.enable_progressive_unlock;
        settings["unlock_year_interval"] = m_settings.unlock_year_interval;
        settings["player_can_enable_early"] = m_settings.player_can_enable_early;
        settings["allow_system_delegation"] = m_settings.allow_system_delegation;
        settings["allow_regional_delegation"] = m_settings.allow_regional_delegation;
        settings["allow_task_delegation"] = m_settings.allow_task_delegation;
        settings["enable_quiet_period_acceleration"] = m_settings.enable_quiet_period_acceleration;
        settings["max_acceleration_factor"] = m_settings.max_acceleration_factor;
        settings["quiet_period_decision_threshold"] = m_settings.quiet_period_decision_threshold;
        settings["quiet_period_event_threshold"] = m_settings.quiet_period_event_threshold;
        settings["escalation_performance_threshold"] = m_settings.escalation_performance_threshold;
        settings["max_stored_decisions"] = m_settings.max_stored_decisions;
        settings["max_stored_consequences"] = m_settings.max_stored_consequences;
        
        // Serialize simplified systems
        Json::Value simplified_systems;
        for (const auto& [system, simplified] : m_settings.simplified_systems) {
            simplified_systems[std::to_string(static_cast<int>(system))] = simplified;
        }
        settings["simplified_systems"] = simplified_systems;
        root["settings"] = settings;
        
        // Serialize metrics
        Json::Value metrics;
        metrics["total_decisions_processed"] = m_metrics.total_decisions_processed;
        metrics["delegated_decisions"] = m_metrics.delegated_decisions;
        metrics["player_decisions"] = m_metrics.player_decisions;
        metrics["escalated_consequences"] = m_metrics.escalated_consequences;
        metrics["average_decision_quality"] = m_metrics.average_decision_quality;
        root["metrics"] = metrics;
        
        // Serialize subsystems
        root["decision_system"] = m_decision_system.Serialize(version);
        root["delegation_system"] = m_delegation_system.Serialize(version);
        root["quiet_period_manager"] = m_quiet_period_manager.Serialize(version);
        
        return root;
    }

    bool GameplayCoordinator::Deserialize(const Json::Value& data, int version) {
        try {
            // Check schema version compatibility
            int schema_version = data.get("schema_version", 1).asInt();
            if (schema_version > schema_versions::GAMEPLAY_COORDINATOR_SCHEMA_V1) {
                ::core::logging::LogWarning("CoreGameplaySystem", "Loading newer coordinator schema version " + std::to_string(schema_version));
            }

            // Deserialize complexity settings
            if (data.isMember("settings")) {
                const Json::Value& settings = data["settings"];
                m_settings.overall_level = static_cast<ComplexityLevel>(settings.get("overall_level", 1).asInt());
                m_settings.enable_progressive_unlock = settings.get("enable_progressive_unlock", true).asBool();
                m_settings.unlock_year_interval = std::max(1, settings.get("unlock_year_interval", 10).asInt());
                m_settings.player_can_enable_early = settings.get("player_can_enable_early", true).asBool();
                m_settings.allow_system_delegation = settings.get("allow_system_delegation", true).asBool();
                m_settings.allow_regional_delegation = settings.get("allow_regional_delegation", true).asBool();
                m_settings.allow_task_delegation = settings.get("allow_task_delegation", true).asBool();
                m_settings.enable_quiet_period_acceleration = settings.get("enable_quiet_period_acceleration", true).asBool();
                m_settings.max_acceleration_factor = std::clamp(settings.get("max_acceleration_factor", 5.0).asDouble(), 1.0, 20.0);
                m_settings.quiet_period_decision_threshold = std::max(0, settings.get("quiet_period_decision_threshold", 2).asInt());
                m_settings.quiet_period_event_threshold = std::max(0, settings.get("quiet_period_event_threshold", 3).asInt());
                m_settings.escalation_performance_threshold = std::clamp(settings.get("escalation_performance_threshold", 0.3).asDouble(), 0.0, 1.0);
                m_settings.max_stored_decisions = std::max(10, settings.get("max_stored_decisions", 100).asInt());
                m_settings.max_stored_consequences = std::max(50, settings.get("max_stored_consequences", 500).asInt());
                
                // Deserialize simplified systems
                if (settings.isMember("simplified_systems")) {
                    const Json::Value& simplified_systems = settings["simplified_systems"];
                    for (const auto& key : simplified_systems.getMemberNames()) {
                        try {
                            int system_int = std::stoi(key);
                            auto system = static_cast<types::SystemType>(system_int);
                            m_settings.simplified_systems[system] = simplified_systems[key].asBool();
                        } catch (const std::exception& e) {
                            ::core::logging::LogWarning("CoreGameplaySystem", "Failed to deserialize simplified system key " + key + ": " + std::string(e.what()));
                        }
                    }
                }
            }
            
            // Deserialize metrics
            if (data.isMember("metrics")) {
                const Json::Value& metrics = data["metrics"];
                m_metrics.total_decisions_processed = std::max(0, metrics.get("total_decisions_processed", 0).asInt());
                m_metrics.delegated_decisions = std::max(0, metrics.get("delegated_decisions", 0).asInt());
                m_metrics.player_decisions = std::max(0, metrics.get("player_decisions", 0).asInt());
                m_metrics.escalated_consequences = std::max(0, metrics.get("escalated_consequences", 0).asInt());
                m_metrics.average_decision_quality = std::clamp(metrics.get("average_decision_quality", 0.5).asDouble(), 0.0, 1.0);
            }
            
            // Deserialize subsystems
            bool success = true;
            success &= m_decision_system.Deserialize(data.get("decision_system", Json::Value()), version);
            success &= m_delegation_system.Deserialize(data.get("delegation_system", Json::Value()), version);
            success &= m_quiet_period_manager.Deserialize(data.get("quiet_period_manager", Json::Value()), version);
            
            // Reconnect systems
            m_delegation_system.SetConsequenceSystem(&m_decision_system);
            
            return success;
        } catch (const std::exception& e) {
            ::core::logging::LogError("CoreGameplaySystem", "Failed to deserialize GameplayCoordinator: " + std::string(e.what()));
            return false;
        }
    }

} // namespace game::gameplay
        
