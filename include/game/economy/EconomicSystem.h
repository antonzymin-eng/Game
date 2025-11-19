// ============================================================================
// Date/Time Created: September 27, 2025 - 4:10 PM PST
// Strategic Rebuild: October 21, 2025 - Following PopulationSystem pattern
// Location: include/game/economy/EconomicSystem.h
// ============================================================================

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include "core/ECS/ISystem.h"
#include "core/threading/ThreadedSystemManager.h"
#include "game/economy/EconomicComponents.h"
#include "core/types/game_types.h"
#include "utils/PlatformCompat.h"

#include <vector>
#include <string>
#include <memory>

namespace game::economy {

    // Forward declaration
    struct EconomicSystemConfig;

    // ============================================================================
    // Economic System Configuration
    // ============================================================================

    struct EconomicSystemConfig {
        // Update frequencies
        double monthly_update_interval = 30.0; // 30 days in-game

        // Economic parameters
        double base_tax_rate = 0.10;
        double trade_efficiency = 0.85;
        double inflation_rate = 0.02;

        // Treasury limits
        int min_treasury = 0;
        int starting_treasury = 1000;

        // Random event chances
        double event_chance_per_month = 0.15;
        double good_event_weight = 0.4;
        double bad_event_weight = 0.6;
    };

    // ============================================================================
    // EconomicSystem - Strategic Rebuild Following PopulationSystem Pattern
    // ============================================================================

    class EconomicSystem : public game::core::ISystem {
    public:
        explicit EconomicSystem(::core::ecs::ComponentAccessManager& access_manager,
                               ::core::threading::ThreadSafeMessageBus& message_bus);
        
        virtual ~EconomicSystem() = default;

        // ISystem interface
        void Initialize() override;
        void Update(float delta_time) override;
        void Shutdown() override;
        
        // Threading configuration
        ::core::threading::ThreadingStrategy GetThreadingStrategy() const override;
        std::string GetThreadingRationale() const;

        // Serialization interface
        std::string GetSystemName() const override;
        Json::Value Serialize(int version) const override;
        bool Deserialize(const Json::Value& data, int version) override;

        // Economic management interface
        void CreateEconomicComponents(game::types::EntityID entity_id);
        void ProcessMonthlyUpdate(game::types::EntityID entity_id);
        
        // Treasury management
        bool SpendMoney(game::types::EntityID entity_id, int amount);
        void AddMoney(game::types::EntityID entity_id, int amount);
        int GetTreasury(game::types::EntityID entity_id) const;
        int GetMonthlyIncome(game::types::EntityID entity_id) const;
        int GetMonthlyExpenses(game::types::EntityID entity_id) const;
        int GetNetIncome(game::types::EntityID entity_id) const;
        
        // Trade route management
        void AddTradeRoute(game::types::EntityID from_entity, game::types::EntityID to_entity,
                          double efficiency, int base_value);
        void RemoveTradeRoute(game::types::EntityID from_entity, game::types::EntityID to_entity);
        std::vector<TradeRoute> GetTradeRoutesForEntity(game::types::EntityID entity_id) const;
        
        // Economic events
        void ProcessRandomEvents(game::types::EntityID entity_id);
        std::vector<EconomicEvent> GetActiveEvents(game::types::EntityID entity_id) const;

        // Configuration access
        const EconomicSystemConfig& GetConfiguration() const;

    private:
        // Core dependencies
        ::core::ecs::ComponentAccessManager& m_access_manager;
        ::core::threading::ThreadSafeMessageBus& m_message_bus;

        // System state
        bool m_initialized = false;
        EconomicSystemConfig m_config;

        // Timing
        float m_accumulated_time = 0.0f;
        float m_monthly_timer = 0.0f;

        // System initialization
        void LoadConfiguration();
        void SubscribeToEvents();

        // Update processing
        void ProcessRegularUpdates(float delta_time);
        void ProcessMonthlyUpdates(float delta_time);

        // Internal methods
        void CalculateMonthlyTotals(game::types::EntityID entity_id);
        void ProcessEntityEconomy(game::types::EntityID entity_id);
        void ProcessTradeRoutes(game::types::EntityID entity_id);
        void GenerateRandomEvent(game::types::EntityID entity_id);
        void ApplyEventEffects(game::types::EntityID entity_id, const EconomicEvent& event);
    };

} // namespace game::economy
