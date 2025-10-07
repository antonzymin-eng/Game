// ============================================================================
// AdministrativeSystem.cpp - Administrative System Implementation (FIXED)
// Created: September 24, 2025, 2:25 PM PST
// Location: src/game/administration/AdministrativeSystem.cpp
// Threading Strategy: THREAD_POOL compatible with mutex-protected cache
// ============================================================================

#include "game/administration/AdministrativeSystem.h"
#include "utils/RandomGenerator.h"
#include <algorithm>
#include <sstream>
#include <mutex>

namespace game {

    // Thread-safe efficiency cache protection
    static std::mutex efficiency_cache_mutex;

    AdministrativeSystem::AdministrativeSystem() : next_official_id(1), monthly_salary_cost(0) {
        // Start with a basic court advisor
        appointOfficial(OfficialType::COURT_ADVISOR, -1);
    }

    int AdministrativeSystem::appointOfficial(OfficialType type, int province_id) {
        auto official = std::make_unique<AdministrativeOfficial>(
            next_official_id++,
            AdministrativeOfficial::generateRandomName(),
            type,
            province_id
        );

        official->is_appointed = true;
        official->satisfaction = 0.7f; // Start happy with appointment

        int official_id = official->id;
        officials.push_back(std::move(official));

        updateMonthlySalaryCost();
        invalidateEfficiencyCache();

        return official_id;
    }

    bool AdministrativeSystem::dismissOfficial(int official_id) {
        auto it = std::find_if(officials.begin(), officials.end(),
            [official_id](const std::unique_ptr<AdministrativeOfficial>& official) {
                return official->id == official_id;
            });

        if (it != officials.end()) {
            // Check if dismissal will cause issues
            AdministrativeOfficial* official = it->get();
            if (official->loyalty > 60 && official->months_in_position > 6) {
                // Generate dismissal event - loyal long-serving officials don't go quietly
                OfficialEvent event;
                event.official_id = official_id;
                event.title = "Dismissal of " + official->name;
                event.description = official->name + " has served loyally and does not wish to be dismissed. Other officials are watching.";
                event.options = { "Dismiss anyway (-10 stability)", "Offer generous severance (+50 gold cost)", "Keep in position" };
                event.option_costs = { 0, 50, 0 };
                pending_events.push_back(event);
            }

            officials.erase(it);
            updateMonthlySalaryCost();
            invalidateEfficiencyCache();
            return true;
        }
        return false;
    }

    AdministrativeOfficial* AdministrativeSystem::getOfficial(int official_id) {
        auto it = std::find_if(officials.begin(), officials.end(),
            [official_id](const std::unique_ptr<AdministrativeOfficial>& official) {
                return official->id == official_id;
            });

        return (it != officials.end()) ? it->get() : nullptr;
    }

    const AdministrativeOfficial* AdministrativeSystem::getOfficial(int official_id) const {
        auto it = std::find_if(officials.begin(), officials.end(),
            [official_id](const std::unique_ptr<AdministrativeOfficial>& official) {
                return official->id == official_id;
            });

        return (it != officials.end()) ? it->get() : nullptr;
    }

    std::vector<AdministrativeOfficial*> AdministrativeSystem::getOfficialsByType(OfficialType type) {
        std::vector<AdministrativeOfficial*> result;
        for (auto& official : officials) {
            if (official->type == type) {
                result.push_back(official.get());
            }
        }
        return result;
    }

    std::vector<AdministrativeOfficial*> AdministrativeSystem::getOfficialsByProvince(int province_id) {
        std::vector<AdministrativeOfficial*> result;
        for (auto& official : officials) {
            if (official->province_id == province_id) {
                result.push_back(official.get());
            }
        }
        return result;
    }

    std::vector<AdministrativeOfficial*> AdministrativeSystem::getAllOfficials() {
        std::vector<AdministrativeOfficial*> result;
        for (auto& official : officials) {
            result.push_back(official.get());
        }
        return result;
    }

    int AdministrativeSystem::getOfficialCount() const {
        return static_cast<int>(officials.size());
    }

    int AdministrativeSystem::getMonthlySalaryCost() const {
        return monthly_salary_cost;
    }

    float AdministrativeSystem::getProvinceAdministrativeEfficiency(int province_id) {
        std::lock_guard<std::mutex> lock(efficiency_cache_mutex);
        
        if (efficiency_cache_dirty) {
            calculateEfficiencyCache();
        }

        auto it = province_efficiency_cache.find(province_id);
        if (it != province_efficiency_cache.end()) {
            return it->second;
        }

        return 0.5f; // Base efficiency when no officials assigned
    }

    float AdministrativeSystem::getProvinceTaxEfficiency(int province_id) {
        float base_efficiency = getProvinceAdministrativeEfficiency(province_id);

        // Tax collectors provide specific bonus
        auto tax_collectors = getOfficialsByProvince(province_id);
        float tax_bonus = 0.0f;

        for (auto* official : tax_collectors) {
            if (official->type == OfficialType::TAX_COLLECTOR) {
                tax_bonus += getOfficialEfficiencyContribution(official, OfficialType::TAX_COLLECTOR) * 0.3f;
            }
        }

        return std::min(1.0f, base_efficiency + tax_bonus);
    }

    float AdministrativeSystem::getProvinceTradeEfficiency(int province_id) {
        float base_efficiency = getProvinceAdministrativeEfficiency(province_id);

        // Trade ministers provide specific bonus
        auto trade_officials = getOfficialsByProvince(province_id);
        float trade_bonus = 0.0f;

        for (auto* official : trade_officials) {
            if (official->type == OfficialType::TRADE_MINISTER) {
                trade_bonus += getOfficialEfficiencyContribution(official, OfficialType::TRADE_MINISTER) * 0.25f;
            }
        }

        return std::min(1.0f, base_efficiency + trade_bonus);
    }

    float AdministrativeSystem::getProvinceMilitaryEfficiency(int province_id) {
        float base_efficiency = getProvinceAdministrativeEfficiency(province_id);

        // Military governors provide specific bonus
        auto military_officials = getOfficialsByProvince(province_id);
        float military_bonus = 0.0f;

        for (auto* official : military_officials) {
            if (official->type == OfficialType::MILITARY_GOVERNOR) {
                military_bonus += getOfficialEfficiencyContribution(official, OfficialType::MILITARY_GOVERNOR) * 0.4f;
            }
        }

        return std::min(1.0f, base_efficiency + military_bonus);
    }

    void AdministrativeSystem::processMonthlyUpdate() {
        // Update all officials
        for (auto& official : officials) {
            official->processMonthlyUpdate();
        }

        // Generate random events
        generateRandomEvents();

        // Update costs
        updateMonthlySalaryCost();

        // Mark efficiency cache as dirty
        invalidateEfficiencyCache();
    }

    void AdministrativeSystem::invalidateEfficiencyCache() {
        std::lock_guard<std::mutex> lock(efficiency_cache_mutex);
        efficiency_cache_dirty = true;
    }

    std::vector<OfficialEvent>& AdministrativeSystem::getPendingEvents() {
        return pending_events;
    }

    void AdministrativeSystem::resolveEvent(int event_index, int chosen_option) {
        if (event_index < 0 || event_index >= static_cast<int>(pending_events.size())) {
            return;
        }

        OfficialEvent& event = pending_events[event_index];
        AdministrativeOfficial* official = getOfficial(event.official_id);

        if (official) {
            // FIXED: Replace fragile string matching with more robust checks
            // Check for corruption event type
            if (event.title.substr(0, 10) == "Corruption" || 
                event.description.find("embezzling") != std::string::npos ||
                event.description.find("bribes") != std::string::npos) {
                
                if (chosen_option == 0) { // Investigate
                    official->corruption_suspicion += 20;
                    official->adjustSatisfaction(-0.2f);
                }
                else if (chosen_option == 1) { // Ignore
                    official->corruption_suspicion -= 10;
                    official->adjustSatisfaction(0.1f);
                }
                else if (chosen_option == 2) { // Dismiss
                    dismissOfficial(official->id);
                    official = nullptr; // Official no longer exists
                }
            }
            // Check for ambition event type
            else if (event.title.substr(0, 8) == "Ambitious" || 
                     event.description.find("promotion") != std::string::npos ||
                     event.description.find("opportunities elsewhere") != std::string::npos) {
                
                if (chosen_option == 0) { // Promise promotion
                    official->adjustSatisfaction(0.3f);
                }
                else if (chosen_option == 1) { // Refuse
                    official->adjustSatisfaction(-0.2f);
                }
            }
            // Check for performance event type
            else if (event.title.substr(0, 11) == "Exceptional" || 
                     event.title.substr(0, 11) == "Performance") {
                
                if (chosen_option == 0) { // Reward/Training
                    official->adjustSatisfaction(0.2f);
                }
                else if (chosen_option == 1) { // Acknowledge/Warning
                    official->adjustSatisfaction(0.1f);
                }
            }
        }

        // Remove the resolved event
        pending_events.erase(pending_events.begin() + event_index);
    }

    bool AdministrativeSystem::hasUrgentEvents() const {
        for (const auto& event : pending_events) {
            if (event.requires_immediate_attention) {
                return true;
            }
        }
        return false;
    }

    std::vector<AdministrativeOfficial> AdministrativeSystem::getAvailableCandidates(OfficialType type) {
        std::vector<AdministrativeOfficial> candidates;

        // Generate 3-5 random candidates using proper random generation
        int num_candidates = utils::random::Int(3, 5);
        for (int i = 0; i < num_candidates; ++i) {
            candidates.push_back(AdministrativeOfficial::generateRandomOfficial(0, type, -1));
        }

        return candidates;
    }

    int AdministrativeSystem::getAppointmentCost(OfficialType type, int competence_level) const {
        int base_cost = 50;

        // Type modifiers
        switch (type) {
        case OfficialType::COURT_ADVISOR: base_cost = 100; break;
        case OfficialType::MILITARY_GOVERNOR: base_cost = 80; break;
        case OfficialType::TRADE_MINISTER: base_cost = 70; break;
        case OfficialType::TAX_COLLECTOR: base_cost = 60; break;
        }

        // Competence modifier
        base_cost += (competence_level - 50) * 2;

        return std::max(25, base_cost);
    }

    void AdministrativeSystem::calculateEfficiencyCache() {
        // Note: This function should be called while holding efficiency_cache_mutex
        province_efficiency_cache.clear();

        // Calculate base efficiency for each province
        std::unordered_map<int, float> province_totals;
        std::unordered_map<int, int> province_counts;

        for (auto& official : officials) {
            if (official->province_id >= 0) { // Skip court officials
                float contribution = getOfficialEfficiencyContribution(official.get(), official->type);
                province_totals[official->province_id] += contribution;
                province_counts[official->province_id]++;
            }
        }

        // Apply court advisor bonuses empire-wide
        float court_bonus = 0.0f;
        for (auto& official : officials) {
            if (official->type == OfficialType::COURT_ADVISOR) {
                court_bonus += getOfficialEfficiencyContribution(official.get(), OfficialType::COURT_ADVISOR) * 0.1f;
            }
        }

        // Calculate final efficiency for each province
        for (auto& pair : province_totals) {
            int province_id = pair.first;
            float total_contribution = pair.second;
            int official_count = province_counts[province_id];

            // Average contribution with diminishing returns
            float base_efficiency = total_contribution / std::max(1, official_count);
            float efficiency_with_bonus = std::min(1.0f, base_efficiency + court_bonus);

            // FIXED: Apply diminishing returns without going negative
            if (official_count > 1) {
                float diminishing_factor = 1.0f - (official_count - 1) * 0.1f;
                efficiency_with_bonus *= std::max(0.3f, diminishing_factor); // Minimum 30% efficiency
            }

            province_efficiency_cache[province_id] = std::max(0.3f, efficiency_with_bonus);
        }

        efficiency_cache_dirty = false;
    }

    void AdministrativeSystem::generateRandomEvents() {
        // Limit to 3 pending events at once
        if (pending_events.size() >= 3) return;

        // Check for official-specific events
        for (auto& official : officials) {
            if (official->has_pending_event) {
                if (official->isCorrupt() && utils::random::Percentage(30)) {
                    pending_events.push_back(createCorruptionEvent(official.get()));
                }
                else if (official->hasTrait(OfficialTrait::AMBITIOUS) && utils::random::Percentage(25)) {
                    pending_events.push_back(createAmbitionEvent(official.get()));
                }
                else if (utils::random::Percentage(15)) {
                    bool positive = official->satisfaction > 0.6f || official->getEffectiveCompetence() > 70;
                    pending_events.push_back(createPerformanceEvent(official.get(), positive));
                }

                official->has_pending_event = false;
                break; // Only one event per month
            }
        }
    }

    OfficialEvent AdministrativeSystem::createCorruptionEvent(AdministrativeOfficial* official) {
        OfficialEvent event;
        event.official_id = official->id;
        event.title = "Corruption Suspicions: " + official->name;
        event.description = "Reports suggest " + official->name + " may be embezzling funds or accepting bribes. What should be done?";
        event.options = { "Launch investigation (-20 gold)", "Ignore the rumors", "Dismiss immediately" };
        event.option_costs = { 20, 0, 0 };
        event.requires_immediate_attention = official->corruption_suspicion > 80;
        return event;
    }

    OfficialEvent AdministrativeSystem::createAmbitionEvent(AdministrativeOfficial* official) {
        OfficialEvent event;
        event.official_id = official->id;
        event.title = "Ambitious Demands: " + official->name;
        event.description = official->name + " seeks a promotion or additional responsibilities. They hint at seeking opportunities elsewhere if denied.";
        event.options = { "Promise future promotion", "Refuse their demands", "Offer immediate raise (+10 gold/month)" };
        event.option_costs = { 0, 0, 0 }; // Raise cost handled separately
        return event;
    }

    OfficialEvent AdministrativeSystem::createPerformanceEvent(AdministrativeOfficial* official, bool positive) {
        OfficialEvent event;
        event.official_id = official->id;

        if (positive) {
            event.title = "Exceptional Service: " + official->name;
            event.description = official->name + " has demonstrated exceptional competence and dedication. Their efforts have been noticed by other officials and the populace.";
            event.options = { "Reward with bonus (+30 gold)", "Public acknowledgment", "No special recognition" };
            event.option_costs = { 30, 0, 0 };
        }
        else {
            event.title = "Performance Issues: " + official->name;
            event.description = official->name + " has been struggling with their duties. Their performance is affecting administrative efficiency.";
            event.options = { "Provide additional training (+15 gold)", "Issue formal warning", "Reduce their responsibilities" };
            event.option_costs = { 15, 0, 0 };
        }

        return event;
    }

    void AdministrativeSystem::updateMonthlySalaryCost() {
        monthly_salary_cost = 0;
        for (auto& official : officials) {
            monthly_salary_cost += official->getMonthlyUpkeepCost();
        }
    }

    float AdministrativeSystem::getOfficialEfficiencyContribution(const AdministrativeOfficial* official, OfficialType for_type) const {
        if (!official) return 0.0f;

        float base_contribution = official->getEffectiveCompetence() / 100.0f;
        float loyalty_modifier = official->getLoyaltyModifier();

        // Type matching bonus
        if (official->type == for_type) {
            base_contribution *= 1.2f;
        }

        // Loyalty affects contribution
        base_contribution *= (0.5f + (loyalty_modifier * 0.5f));

        return std::max(0.0f, std::min(1.0f, base_contribution));
    }

    void AdministrativeSystem::serializeToString(std::string& out) const {
        std::ostringstream oss;

        // Save basic info
        oss << "ADMIN_SYSTEM_V1\n";
        oss << "next_id:" << next_official_id << "\n";
        oss << "official_count:" << officials.size() << "\n";

        // Save each official
        for (const auto& official : officials) {
            oss << "OFFICIAL_START\n";
            oss << "id:" << official->id << "\n";
            oss << "name:" << official->name << "\n";
            oss << "type:" << static_cast<int>(official->type) << "\n";
            oss << "province_id:" << official->province_id << "\n";
            oss << "competence:" << official->competence << "\n";
            oss << "loyalty:" << official->loyalty << "\n";
            oss << "age:" << official->age << "\n";
            oss << "months_in_position:" << official->months_in_position << "\n";
            oss << "salary_cost:" << official->salary_cost << "\n";
            oss << "satisfaction:" << official->satisfaction << "\n";
            oss << "is_appointed:" << (official->is_appointed ? 1 : 0) << "\n";
            oss << "corruption_suspicion:" << official->corruption_suspicion << "\n";
            oss << "trait_count:" << official->traits.size() << "\n";

            for (auto trait : official->traits) {
                oss << "trait:" << static_cast<int>(trait) << "\n";
            }

            oss << "OFFICIAL_END\n";
        }

        out = oss.str();
    }

    bool AdministrativeSystem::deserializeFromString(const std::string& data) {
        std::istringstream iss(data);
        std::string line;

        if (!std::getline(iss, line) || line != "ADMIN_SYSTEM_V1") {
            return false;
        }

        officials.clear();
        pending_events.clear();

        // Read basic info
        while (std::getline(iss, line)) {
            // FIXED: Replace C++20 starts_with() with C++17 compatible substr()
            if (line.substr(0, 8) == "next_id:") {
                next_official_id = std::stoi(line.substr(8));
            }
            else if (line.substr(0, 15) == "official_count:") {
                // Just for validation, we'll count as we read
            }
            else if (line == "OFFICIAL_START") {
                // Read an official
                auto official = std::make_unique<AdministrativeOfficial>();

                while (std::getline(iss, line) && line != "OFFICIAL_END") {
                    if (line.substr(0, 3) == "id:") {
                        official->id = std::stoi(line.substr(3));
                    }
                    else if (line.substr(0, 5) == "name:") {
                        official->name = line.substr(5);
                    }
                    else if (line.substr(0, 5) == "type:") {
                        official->type = static_cast<OfficialType>(std::stoi(line.substr(5)));
                    }
                    else if (line.substr(0, 12) == "province_id:") {
                        official->province_id = std::stoi(line.substr(12));
                    }
                    else if (line.substr(0, 11) == "competence:") {
                        official->competence = std::stoi(line.substr(11));
                    }
                    else if (line.substr(0, 8) == "loyalty:") {
                        official->loyalty = std::stoi(line.substr(8));
                    }
                    else if (line.substr(0, 4) == "age:") {
                        official->age = std::stoi(line.substr(4));
                    }
                    else if (line.substr(0, 19) == "months_in_position:") {
                        official->months_in_position = std::stoi(line.substr(19));
                    }
                    else if (line.substr(0, 12) == "salary_cost:") {
                        official->salary_cost = std::stoi(line.substr(12));
                    }
                    else if (line.substr(0, 13) == "satisfaction:") {
                        official->satisfaction = std::stof(line.substr(13));
                    }
                    else if (line.substr(0, 13) == "is_appointed:") {
                        official->is_appointed = (std::stoi(line.substr(13)) == 1);
                    }
                    else if (line.substr(0, 21) == "corruption_suspicion:") {
                        official->corruption_suspicion = std::stoi(line.substr(21));
                    }
                    else if (line.substr(0, 12) == "trait_count:") {
                        int trait_count = std::stoi(line.substr(12));
                        official->traits.clear();
                        for (int i = 0; i < trait_count; ++i) {
                            if (std::getline(iss, line) && line.substr(0, 6) == "trait:") {
                                OfficialTrait trait = static_cast<OfficialTrait>(std::stoi(line.substr(6)));
                                official->traits.push_back(trait);
                            }
                        }
                    }
                }

                officials.push_back(std::move(official));
            }
        }

        updateMonthlySalaryCost();
        invalidateEfficiencyCache();

        return true;
    }

} // namespace game
