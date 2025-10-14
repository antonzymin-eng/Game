// ============================================================================
// Date/Time Created: September 27, 2025 - 4:10 PM PST
// Intended Folder Location: include/game/EconomicSystem.h
// FIXED: Added ISerializable, GetThreadingStrategy, removed circular dependency
// ============================================================================

#pragma once

#include "core/ECS/ISerializable.h"
#include "core/ECS/ComponentAccessManager.h"
#include "game/economy/EconomicComponents.h"
#include "core/types/game_types.h"

#include <jsoncpp/json/json.h>
#include <vector>
#include <string>

namespace game {

    // Forward declarations to avoid circular dependencies
    class AdministrativeSystem;
    struct Province;

    struct TradeRoute {
        int from_province;
        int to_province;
        float efficiency;
        int base_value;

        TradeRoute(int from, int to, float eff, int value)
            : from_province(from), to_province(to), efficiency(eff), base_value(value) {
        }
    };

    struct RandomEvent {
        enum Type {
            GOOD_HARVEST,
            BAD_HARVEST,
            MERCHANT_CARAVAN,
            BANDIT_RAID,
            PLAGUE_OUTBREAK,
            MARKET_BOOM
        };

        Type type;
        int affected_province;
        int duration_months;
        float effect_magnitude;
        std::string description;
    };

    class EconomicSystem : public game::core::ISerializable {
    private:
        ::core::ecs::ComponentAccessManager m_access_manager;
        AdministrativeSystem* admin_system = nullptr;
        bool m_initialized = false;

    public:
        EconomicSystem() = default;
        ~EconomicSystem() = default;

        // Threading strategy method - simplified to avoid circular dependencies
        int GetThreadingStrategy() const {
            return 1; // THREAD_POOL equivalent
        }

        // FIXED: ISerializable interface
        Json::Value Serialize(int version) const override;
        bool Deserialize(const Json::Value& data, int version) override;
        std::string GetSystemName() const override { return "EconomicSystem"; }

        // System lifecycle
        void Initialize(AdministrativeSystem* administrative_system);
        void Shutdown();
        
        // ECS-based economic management
        void CreateEconomicComponents(game::types::EntityID entity_id);
        void ProcessMonthlyUpdate(game::types::EntityID entity_id);
        
        // Trade route management (ECS)
        void AddTradeRoute(game::types::EntityID from_entity, game::types::EntityID to_entity, 
                          float efficiency, int base_value);
        void RemoveTradeRoute(game::types::EntityID from_entity, game::types::EntityID to_entity);
        std::vector<economy::TradeRoute> GetTradeRoutesForEntity(game::types::EntityID entity_id) const;
        
        // Treasury management (ECS)
        bool SpendMoney(game::types::EntityID entity_id, int amount);
        void AddMoney(game::types::EntityID entity_id, int amount);
        int GetTreasury(game::types::EntityID entity_id) const;
        int GetMonthlyIncome(game::types::EntityID entity_id) const;
        int GetMonthlyExpenses(game::types::EntityID entity_id) const;
        int GetNetIncome(game::types::EntityID entity_id) const;
        
        // Economic events (ECS)
        void ProcessRandomEvents(game::types::EntityID entity_id);
        std::vector<economy::EconomicEvent> GetActiveEvents(game::types::EntityID entity_id) const;

        // Aggregate calculations (ECS)
        int GetTotalTaxIncome(const std::vector<game::types::EntityID>& entities) const;
        int GetTotalTradeIncome(const std::vector<game::types::EntityID>& entities) const;
        float GetAverageEconomicGrowth(const std::vector<game::types::EntityID>& entities) const;

        // DEPRECATED: Legacy province-based methods
        void processMonthlyUpdate(std::vector<Province>& provinces);
        void serializeToString(std::string& out) const;
        bool deserializeFromString(const std::string& data);

    private:
        // ECS-based internal methods
        void CalculateMonthlyTotals(game::types::EntityID entity_id);
        void ProcessEntityEconomy(game::types::EntityID entity_id);
        void ProcessTradeRoutes(game::types::EntityID entity_id);
        void GenerateRandomEvent(game::types::EntityID entity_id);
        void ApplyEventEffects(game::types::EntityID entity_id, const economy::EconomicEvent& event);
        void UpdateEventDurations();
    };

} // namespace game
