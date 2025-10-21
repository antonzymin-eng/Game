// ============================================================================
// ScenarioSystem.h - Configuration-Based Gameplay Scenario System
// Created: October 15, 2025
// Demonstrates Phase 1 ECS systems working together in meaningful gameplay
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include "utils/PlatformCompat.h"

// Core ECS
#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"

// Phase 1 Systems - Forward declarations for cross-system interactions
// (Using void* for demo to avoid header dependencies)

namespace game::scenario {

    // ============================================================================
    // Scenario Event Types
    // ============================================================================

    enum class EventType {
        ECONOMIC_SHOCK,
        POPULATION_UNREST,
        MILITARY_BUDGET_CUT,
        ADMINISTRATIVE_RESPONSE,
        RECOVERY_BEGINS,
        TECHNOLOGY_BREAKTHROUGH,
        MILITARY_ENHANCEMENT,
        DIPLOMATIC_TENSION,
        ECONOMIC_BOOST,
        POPULATION_PRIDE,
        ADMINISTRATIVE_ADAPTATION,
        UNKNOWN
    };

    struct ScenarioEffect {
        std::string parameter;
        float value;
        std::string operation; // "multiply", "add", "set"
    };

    struct ScenarioEvent {
        EventType type;
        std::string target_system;
        std::vector<ScenarioEffect> effects;
        std::string message;
        bool executed = false;
    };

    struct ScenarioTrigger {
        std::string condition; // e.g., "day >= 7"
        std::vector<ScenarioEvent> events;
    };

    struct ScenarioData {
        std::string id;
        std::string name;
        std::string description;
        int duration_days;
        std::vector<ScenarioTrigger> triggers;
        std::vector<std::string> completion_messages;
        
        // Runtime state
        int current_day = 0;
        bool is_active = false;
        bool is_completed = false;
    };

    // ============================================================================
    // Scenario System
    // ============================================================================

    class ScenarioSystem {
    private:
        // ECS Infrastructure
        ::core::ecs::ComponentAccessManager* component_manager;
        ::core::ecs::MessageBus* message_bus;
        
        // Phase 1 System References (for cross-system effects) - Optional for demo
        void* population_system;
        void* economic_system;
        void* military_system;
        void* technology_system;
        void* diplomacy_system;
        void* admin_system;
        
        // Scenario Management
        std::vector<ScenarioData> loaded_scenarios;
        ScenarioData* active_scenario;
        
        // Event Processing
        std::vector<std::string> recent_messages;
        std::function<void(const std::string&)> message_callback;
        
    public:
        ScenarioSystem(
            ::core::ecs::ComponentAccessManager* comp_mgr,
            ::core::ecs::MessageBus* msg_bus
        );
        
        ~ScenarioSystem() = default;
        
        // System References (call after system initialization) - Optional
        void RegisterSystems(
            void* pop_sys = nullptr,
            void* econ_sys = nullptr,
            void* mil_sys = nullptr,
            void* tech_sys = nullptr,
            void* dip_sys = nullptr,
            void* admin_sys = nullptr
        );
        
        // Scenario Management
        bool LoadScenario(const std::string& filename);
        bool StartScenario(const std::string& scenario_id);
        void StopCurrentScenario();
        
        // System Update
        void Update(float delta_time);
        void AdvanceDay(); // Call when game day advances
        
        // Information Access
        const ScenarioData* GetActiveScenario() const { return active_scenario; }
        const std::vector<std::string>& GetRecentMessages() const { return recent_messages; }
        std::vector<std::string> GetAvailableScenarios() const;
        
        // Message Handling
        void SetMessageCallback(std::function<void(const std::string&)> callback) {
            message_callback = callback;
        }
        
    private:
        // Internal Processing
        EventType ParseEventType(const std::string& type_str);
        bool EvaluateCondition(const std::string& condition);
        void ExecuteEvent(ScenarioEvent& event);
        void ApplyEffect(const std::string& target_system, const ScenarioEffect& effect);
        
        // System Effect Helpers
        void ApplyPopulationEffect(const ScenarioEffect& effect);
        void ApplyEconomicEffect(const ScenarioEffect& effect);
        void ApplyMilitaryEffect(const ScenarioEffect& effect);
        void ApplyTechnologyEffect(const ScenarioEffect& effect);
        void ApplyDiplomacyEffect(const ScenarioEffect& effect);
        void ApplyAdministrativeEffect(const ScenarioEffect& effect);
        
        // Utility
        void SendMessage(const std::string& message);
        void LogEvent(const std::string& event_description);
    };

} // namespace game::scenario