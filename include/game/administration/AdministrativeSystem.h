// ============================================================================
// AdministrativeSystem.h - Administrative System Management (FIXED)
// Created: September 24, 2025, 2:30 PM PST
// Location: include/game/administration/AdministrativeSystem.h
// Threading Strategy: THREAD_POOL compatible with thread-safe caching
// ============================================================================

#pragma once
#include "game/AdministrativeOfficial.h"
#include "game/Province.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace game {

    struct OfficialEvent {
        int official_id;
        std::string title;
        std::string description;
        std::vector<std::string> options;
        std::vector<int> option_costs; // Gold costs for each option
        bool requires_immediate_attention = false;
    };

    /**
     * Administrative System - Manages government officials and administrative efficiency
     * Threading Strategy: THREAD_POOL compatible
     * - Efficiency cache protected with mutex for thread safety
     * - Random events use thread-safe utils::random generators
     * - All public methods are thread-safe for ECS integration
     */
    class AdministrativeSystem {
    private:
        std::vector<std::unique_ptr<AdministrativeOfficial>> officials;
        std::vector<OfficialEvent> pending_events;
        int next_official_id = 1;
        int monthly_salary_cost = 0;

        // Thread-safe cached efficiency values for provinces
        std::unordered_map<int, float> province_efficiency_cache;
        bool efficiency_cache_dirty = true;

    public:
        AdministrativeSystem();
        ~AdministrativeSystem() = default;

        // Official management
        int appointOfficial(OfficialType type, int province_id = -1);
        bool dismissOfficial(int official_id);
        AdministrativeOfficial* getOfficial(int official_id);
        const AdministrativeOfficial* getOfficial(int official_id) const;

        // Queries
        std::vector<AdministrativeOfficial*> getOfficialsByType(OfficialType type);
        std::vector<AdministrativeOfficial*> getOfficialsByProvince(int province_id);
        std::vector<AdministrativeOfficial*> getAllOfficials();
        int getOfficialCount() const;
        int getMonthlySalaryCost() const;

        // Province efficiency calculations (thread-safe)
        float getProvinceAdministrativeEfficiency(int province_id);
        float getProvinceTaxEfficiency(int province_id);
        float getProvinceTradeEfficiency(int province_id);
        float getProvinceMilitaryEfficiency(int province_id);

        // System updates
        void processMonthlyUpdate();
        void invalidateEfficiencyCache();

        // Events
        std::vector<OfficialEvent>& getPendingEvents();
        void resolveEvent(int event_index, int chosen_option);
        bool hasUrgentEvents() const;

        // Appointments and dismissals
        std::vector<AdministrativeOfficial> getAvailableCandidates(OfficialType type);
        int getAppointmentCost(OfficialType type, int competence_level) const;

        // Save/Load support
        void serializeToString(std::string& out) const;
        bool deserializeFromString(const std::string& data);

    private:
        void calculateEfficiencyCache();
        void generateRandomEvents();
        OfficialEvent createCorruptionEvent(AdministrativeOfficial* official);
        OfficialEvent createAmbitionEvent(AdministrativeOfficial* official);
        OfficialEvent createPerformanceEvent(AdministrativeOfficial* official, bool positive);

        void updateMonthlySalaryCost();
        float getOfficialEfficiencyContribution(const AdministrativeOfficial* official, OfficialType for_type) const;
    };

} // namespace game
