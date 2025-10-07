// ============================================================================
// Date/Time Created: September 27, 2025 - 4:10 PM PST
// Intended Folder Location: include/game/EconomicSystem.h
// FIXED: Added ISerializable, GetThreadingStrategy, removed circular dependency
// ============================================================================

#pragma once
#include "game/Province.h"
#include "../core/save/ISerializable.h"
#include "../core/threading/ThreadingStrategy.h"
#include <vector>
#include <string>

namespace game {

    // Forward declaration to avoid circular dependency
    class AdministrativeSystem;

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

    class EconomicSystem : public core::save::ISerializable {
    private:
        std::vector<TradeRoute> trade_routes;
        std::vector<RandomEvent> active_events;
        AdministrativeSystem* admin_system = nullptr;

        int national_treasury = 1000;
        int monthly_income = 0;
        int monthly_expenses = 0;

    public:
        EconomicSystem() = default;
        ~EconomicSystem() = default;

        // FIXED: Added GetThreadingStrategy
        core::threading::ThreadingStrategy GetThreadingStrategy() const {
            return core::threading::ThreadingStrategy::THREAD_POOL;
        }

        // FIXED: ISerializable interface
        Json::Value Serialize(int version) const override;
        bool Deserialize(const Json::Value& data, int version) override;
        std::string GetSystemName() const override { return "EconomicSystem"; }

        void initialize(AdministrativeSystem* administrative_system);
        void processMonthlyUpdate(std::vector<Province>& provinces);

        void addTradeRoute(int from_province, int to_province, float efficiency, int base_value);
        void removeTradeRoute(int from_province, int to_province);
        std::vector<TradeRoute> getTradeRoutesForProvince(int province_id) const;
        int calculateTradeIncome(const Province& province) const;

        bool spendMoney(int amount);
        void addMoney(int amount);
        int getTreasury() const { return national_treasury; }
        int getMonthlyIncome() const { return monthly_income; }
        int getMonthlyExpenses() const { return monthly_expenses; }
        int getNetIncome() const { return monthly_income - monthly_expenses; }

        const std::vector<RandomEvent>& getActiveEvents() const { return active_events; }
        void processRandomEvents(std::vector<Province>& provinces);

        int getTotalTaxIncome(const std::vector<Province>& provinces) const;
        int getTotalTradeIncome(const std::vector<Province>& provinces) const;
        int getTotalPopulation(const std::vector<Province>& provinces) const;
        float getAverageStability(const std::vector<Province>& provinces) const;

        // DEPRECATED: Use Serialize/Deserialize instead
        void serializeToString(std::string& out) const;
        bool deserializeFromString(const std::string& data);

    private:
        void calculateMonthlyTotals(const std::vector<Province>& provinces);
        void processProvinceEconomy(Province& province);
        void processTradeRoutes(std::vector<Province>& provinces);
        void generateRandomEvent(const std::vector<Province>& provinces);
        void applyEventEffects(Province& province, const RandomEvent& event);
        void updateEventDurations();
    };

} // namespace game
