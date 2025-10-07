// ============================================================================
// TechnologyUtils.cpp - Technology Management Integration Utilities
// Created: September 22, 2025, 18:15 UTC
// Location: src/game/technology/TechnologyUtils.cpp
// Provides integration utilities for ProvinceManagementSystem
// ============================================================================

#include "game/technology/TechnologySystem.h"
#include "game/management/ProvinceManagementSystem.h"
#include "core/logging/Logger.h"

namespace game::technology::integration {

    // ============================================================================
    // Management System Integration
    // ============================================================================

    /**
     * Generate technology research decisions for the ProvinceManagementSystem
     */
    management::DecisionContext GenerateTechnologyResearchDecision(types::EntityID province_id,
        const TechnologySystem& tech_system) {

        management::DecisionContext context;
        context.province_id = province_id;
        context.decision_type = management::ManagementDecisionType::RESEARCH_FUNDING;
        context.situation_description = "Technology research opportunities available";
        context.urgency_factor = 0.3; // Research is generally not urgent
        context.deadline = std::chrono::system_clock::now() + std::chrono::hours(336); // 2 weeks

        // Get available research options
        auto available_research = tech_system.GetAvailableResearch(province_id);
        
        if (available_research.empty()) {
            // No research available
            management::DecisionOption no_research;
            no_research.option_id = "no_research_available";
            no_research.description = "No new technologies available for research";
            no_research.cost = 0.0;
            no_research.benefit_estimate = 0.0;
            no_research.is_available = true;
            no_research.ai_recommendation = 1.0;
            context.available_options.push_back(no_research);
            return context;
        }

        // Add research options for top 3 available technologies
        int option_count = 0;
        for (auto tech_type : available_research) {
            if (option_count >= 3) break; // Limit to 3 options to avoid decision overload

            auto* definition = tech_system.GetTechnologyDefinition(tech_type);
            if (!definition) continue;

            management::DecisionOption option;
            option.option_id = "research_" + utils::TechnologyTypeToString(tech_type);
            option.description = "Begin research on " + definition->name;
            option.tooltip = definition->description;
            option.cost = definition->base_research_cost;
            
            // Estimate benefit based on technology effects
            double total_benefit = 0.0;
            for (const auto& [effect_type, magnitude] : definition->effects) {
                total_benefit += magnitude * 1000.0; // Convert percentage to gold value estimate
            }
            option.benefit_estimate = total_benefit;

            option.is_available = true;

            // AI recommendation based on category priority
            switch (definition->category) {
            case TechnologyCategory::AGRICULTURAL:
                option.ai_recommendation = 0.8; // High priority for economic growth
                break;
            case TechnologyCategory::MILITARY:
                option.ai_recommendation = 0.6; // Medium priority unless at war
                break;
            case TechnologyCategory::ADMINISTRATIVE:
                option.ai_recommendation = 0.7; // Good for governance
                break;
            default:
                option.ai_recommendation = 0.5; // Default medium priority
                break;
            }

            context.available_options.push_back(option);
            option_count++;
        }

        // Add "maintain current research" option
        management::DecisionOption maintain;
        maintain.option_id = "maintain_current_research";
        maintain.description = "Continue current research projects";
        maintain.cost = 0.0;
        maintain.benefit_estimate = 50.0; // Small benefit for consistency
        maintain.is_available = true;
        maintain.ai_recommendation = 0.4;
        context.available_options.push_back(maintain);

        return context;
    }

    /**
     * Generate scholar patronage decisions
     */
    management::DecisionContext GenerateScholarPatronageDecision(types::EntityID province_id,
        const TechnologySystem& tech_system) {

        management::DecisionContext context;
        context.province_id = province_id;
        context.decision_type = management::ManagementDecisionType::SCHOLAR_PATRONAGE;
        context.situation_description = "Scholar patronage investment opportunity";
        context.urgency_factor = 0.2; // Low urgency
        context.deadline = std::chrono::system_clock::now() + std::chrono::hours(720); // 1 month

        // Add patronage investment options
        std::vector<std::pair<double, std::string>> investment_levels = {
            {100.0, "Basic patronage - Support local scholars"},
            {250.0, "Enhanced patronage - Attract foreign scholars"},
            {500.0, "Major patronage - Establish research center"},
            {1000.0, "Royal patronage - Create centers of learning"}
        };

        for (const auto& [cost, description] : investment_levels) {
            management::DecisionOption option;
            option.option_id = "patronage_" + std::to_string(static_cast<int>(cost));
            option.description = description;
            option.cost = cost;
            option.benefit_estimate = cost * 0.2; // 20% return estimate through research benefits
            option.is_available = true;
            option.ai_recommendation = std::max(0.3, std::min(0.9, (1000.0 - cost) / 1000.0)); // Higher recommendation for lower cost
            context.available_options.push_back(option);
        }

        // Add "no investment" option
        management::DecisionOption no_investment;
        no_investment.option_id = "no_scholar_investment";
        no_investment.description = "Continue without additional scholar investment";
        no_investment.cost = 0.0;
        no_investment.benefit_estimate = 0.0;
        no_investment.is_available = true;
        no_investment.ai_recommendation = 0.5;
        context.available_options.push_back(no_investment);

        return context;
    }

    /**
     * Execute technology-related management decisions
     */
    bool ExecuteTechnologyDecision(types::EntityID province_id, const std::string& option_id,
        TechnologySystem& tech_system) {

        if (option_id.find("research_") == 0) {
            // Extract technology name from option_id
            std::string tech_name = option_id.substr(9); // Remove "research_" prefix
            TechnologyType tech_type = utils::StringToTechnologyType(tech_name);
            
            if (tech_type != TechnologyType::INVALID) {
                return tech_system.StartResearch(province_id, tech_type);
            }
        }
        else if (option_id.find("patronage_") == 0) {
            // Extract investment amount from option_id
            std::string amount_str = option_id.substr(10); // Remove "patronage_" prefix
            try {
                double investment = std::stod(amount_str);
                // Invest in academic research
                return tech_system.InvestInResearch(province_id, TechnologyCategory::ACADEMIC, investment);
            }
            catch (const std::exception&) {
                return false;
            }
        }
        else if (option_id == "maintain_current_research") {
            // No action needed - continue current research
            return true;
        }
        else if (option_id == "no_research_available" || option_id == "no_scholar_investment") {
            // No action options
            return true;
        }

        return false;
    }

    /**
     * Create technology research orders for the management system
     */
    std::unique_ptr<management::ProvinceOrder> CreateTechnologyResearchOrder(types::EntityID province_id,
        TechnologyType technology, double investment) {

        auto order = std::make_unique<management::ProvinceOrder>(management::OrderType::RESEARCH_ORDER, province_id);
        
        order->order_description = "Research " + utils::TechnologyTypeToString(technology);
        order->estimated_cost = investment;
        order->parameters["technology_type"] = std::to_string(static_cast<uint32_t>(technology));
        order->parameters["investment_amount"] = std::to_string(investment);
        order->can_execute = true; // Research orders are generally always executable

        return order;
    }

    /**
     * Get technology research recommendations for a province
     */
    std::vector<std::string> GetTechnologyRecommendations(types::EntityID province_id,
        const TechnologySystem& tech_system) {

        std::vector<std::string> recommendations;

        auto available_research = tech_system.GetAvailableResearch(province_id);
        auto discovered_tech = tech_system.GetDiscoveredTechnologies(province_id);

        // Analyze current technology state
        std::unordered_map<TechnologyCategory, int> discovered_by_category;
        for (auto tech : discovered_tech) {
            auto* definition = tech_system.GetTechnologyDefinition(tech);
            if (definition) {
                discovered_by_category[definition->category]++;
            }
        }

        // Recommend focusing on underdeveloped categories
        for (auto category = TechnologyCategory::AGRICULTURAL; 
             category < TechnologyCategory::COUNT; 
             category = static_cast<TechnologyCategory>(static_cast<int>(category) + 1)) {

            int count = discovered_by_category[category];
            if (count < 2) { // Less than 2 technologies in this category
                recommendations.push_back("Focus research on " + utils::TechnologyCategoryToString(category) + 
                    " technologies to improve province capabilities");
            }
        }

        // Recommend specific high-value technologies
        for (auto tech : available_research) {
            auto* definition = tech_system.GetTechnologyDefinition(tech);
            if (!definition) continue;

            // Check for high-impact technologies
            double total_impact = 0.0;
            for (const auto& [effect_type, magnitude] : definition->effects) {
                total_impact += magnitude;
            }

            if (total_impact >= 0.5) { // 50% or more total impact
                recommendations.push_back("Consider researching " + definition->name + 
                    " for significant economic and social benefits");
            }
        }

        // If no specific recommendations, give general advice
        if (recommendations.empty()) {
            recommendations.push_back("Continue steady research progress across all technology categories");
            recommendations.push_back("Invest in scholar patronage to improve research efficiency");
        }

        return recommendations;
    }

    /**
     * Calculate technology progress for management system display
     */
    std::unordered_map<std::string, double> GetTechnologyProgressReport(types::EntityID province_id,
        const TechnologySystem& tech_system) {

        std::unordered_map<std::string, double> progress_report;

        // Get research progress for all categories
        for (auto category = TechnologyCategory::AGRICULTURAL; 
             category < TechnologyCategory::COUNT; 
             category = static_cast<TechnologyCategory>(static_cast<int>(category) + 1)) {

            auto technologies_in_category = tech_system.GetTechnologiesInCategory(category);
            if (technologies_in_category.empty()) continue;

            double total_progress = 0.0;
            int technology_count = 0;

            for (auto tech : technologies_in_category) {
                double progress = tech_system.GetResearchProgress(province_id, tech);
                double implementation = tech_system.GetImplementationLevel(province_id, tech);
                
                // Combine research and implementation progress
                double combined_progress = std::max(progress, implementation);
                total_progress += combined_progress;
                technology_count++;
            }

            if (technology_count > 0) {
                double average_progress = total_progress / technology_count;
                progress_report[utils::TechnologyCategoryToString(category)] = average_progress;
            }
        }

        // Add overall technology level
        double overall_progress = 0.0;
        for (const auto& [category, progress] : progress_report) {
            overall_progress += progress;
        }
        if (!progress_report.empty()) {
            progress_report["Overall Technology Level"] = overall_progress / progress_report.size();
        }

        return progress_report;
    }

    /**
     * Generate technology crisis events for management system
     */
    management::DecisionContext GenerateTechnologyCrisisDecision(types::EntityID province_id,
        const std::string& crisis_type) {

        management::DecisionContext context;
        context.province_id = province_id;
        context.decision_type = management::ManagementDecisionType::RESEARCH_FUNDING;
        context.urgency_factor = 0.7; // Crises are more urgent

        if (crisis_type == "scholar_exodus") {
            context.situation_description = "Scholars are leaving due to lack of funding and support";
            context.deadline = std::chrono::system_clock::now() + std::chrono::hours(168); // 1 week

            // Emergency funding option
            management::DecisionOption emergency_funding;
            emergency_funding.option_id = "emergency_scholar_funding";
            emergency_funding.description = "Provide emergency funding to retain scholars";
            emergency_funding.cost = 500.0;
            emergency_funding.benefit_estimate = 300.0;
            emergency_funding.is_available = true;
            emergency_funding.ai_recommendation = 0.8;
            context.available_options.push_back(emergency_funding);

            // Let them leave option
            management::DecisionOption accept_exodus;
            accept_exodus.option_id = "accept_scholar_exodus";
            accept_exodus.description = "Accept the scholar exodus and rebuild later";
            accept_exodus.cost = 0.0;
            accept_exodus.benefit_estimate = -200.0; // Negative impact
            accept_exodus.is_available = true;
            accept_exodus.ai_recommendation = 0.2;
            context.available_options.push_back(accept_exodus);
        }
        else if (crisis_type == "research_stagnation") {
            context.situation_description = "Research progress has stagnated - new approaches needed";
            context.deadline = std::chrono::system_clock::now() + std::chrono::hours(720); // 1 month

            // Foreign scholar invitation
            management::DecisionOption invite_scholars;
            invite_scholars.option_id = "invite_foreign_scholars";
            invite_scholars.description = "Invite foreign scholars to bring new ideas";
            invite_scholars.cost = 300.0;
            invite_scholars.benefit_estimate = 400.0;
            invite_scholars.is_available = true;
            invite_scholars.ai_recommendation = 0.7;
            context.available_options.push_back(invite_scholars);

            // Research reform option
            management::DecisionOption reform_research;
            reform_research.option_id = "reform_research_methods";
            reform_research.description = "Reform research methods and institutions";
            reform_research.cost = 200.0;
            reform_research.benefit_estimate = 250.0;
            reform_research.is_available = true;
            reform_research.ai_recommendation = 0.6;
            context.available_options.push_back(reform_research);

            // Continue as normal
            management::DecisionOption continue_normal;
            continue_normal.option_id = "continue_research_normally";
            continue_normal.description = "Continue current research approach";
            continue_normal.cost = 0.0;
            continue_normal.benefit_estimate = 0.0;
            continue_normal.is_available = true;
            continue_normal.ai_recommendation = 0.3;
            context.available_options.push_back(continue_normal);
        }

        return context;
    }

    /**
     * Handle technology breakthrough events for management system
     */
    void HandleTechnologyBreakthrough(types::EntityID province_id, TechnologyType technology,
        management::ProvinceManagementSystem& management_system) {

        // Generate celebration decision
        auto celebration_context = management::DecisionContext{};
        celebration_context.province_id = province_id;
        celebration_context.decision_type = management::ManagementDecisionType::SOCIAL_SERVICES;
        celebration_context.situation_description = "Major technology breakthrough: " + 
            utils::TechnologyTypeToString(technology) + " - celebrate achievement?";
        celebration_context.urgency_factor = 0.1; // Low urgency
        celebration_context.deadline = std::chrono::system_clock::now() + std::chrono::hours(168);

        // Celebration options
        management::DecisionOption grand_celebration;
        grand_celebration.option_id = "grand_celebration";
        grand_celebration.description = "Hold grand celebration to honor the achievement";
        grand_celebration.cost = 200.0;
        grand_celebration.benefit_estimate = 150.0; // Morale boost
        grand_celebration.is_available = true;
        grand_celebration.ai_recommendation = 0.6;
        celebration_context.available_options.push_back(grand_celebration);

        management::DecisionOption modest_recognition;
        modest_recognition.option_id = "modest_recognition";
        modest_recognition.description = "Provide modest recognition to researchers";
        modest_recognition.cost = 50.0;
        modest_recognition.benefit_estimate = 75.0;
        modest_recognition.is_available = true;
        modest_recognition.ai_recommendation = 0.8;
        celebration_context.available_options.push_back(modest_recognition);

        management::DecisionOption no_celebration;
        no_celebration.option_id = "no_celebration";
        no_celebration.description = "Focus on practical implementation rather than celebration";
        no_celebration.cost = 0.0;
        no_celebration.benefit_estimate = 25.0; // Small efficiency gain
        no_celebration.is_available = true;
        no_celebration.ai_recommendation = 0.4;
        celebration_context.available_options.push_back(no_celebration);

        // Add decision to management system
        auto decision = std::make_unique<management::PlayerDecision>(celebration_context);
        management_system.GetDecisionQueue()->AddDecision(std::move(decision));
    }

    /**
     * Update technology effects in other systems
     */
    void UpdateTechnologyEffects(types::EntityID province_id, TechnologyType technology,
        double implementation_level) {

        // This function would integrate with other systems to apply technology effects
        // For now, it's a placeholder for future integration

        core::logging::LogInfo("TechnologyIntegration", 
            "Applied technology effects for " + utils::TechnologyTypeToString(technology) + 
            " at " + std::to_string(static_cast<int>(implementation_level * 100)) + "% implementation");

        // Example integrations:
        // - Update agricultural productivity in province system
        // - Update military unit effectiveness in military system
        // - Update administrative efficiency in administrative system
        // - Update population growth rates in population system
    }

    /**
     * Check for technology-based building unlocks
     */
    std::vector<std::string> GetUnlockedBuildings(types::EntityID province_id,
        const TechnologySystem& tech_system) {

        std::vector<std::string> unlocked_buildings;
        auto implemented_tech = tech_system.GetImplementedTechnologies(province_id);

        for (auto tech : implemented_tech) {
            auto* definition = tech_system.GetTechnologyDefinition(tech);
            if (!definition) continue;

            // Map technologies to building unlocks
            switch (tech) {
            case TechnologyType::UNIVERSITY_SYSTEM:
                unlocked_buildings.push_back("university");
                break;
            case TechnologyType::BLAST_FURNACE:
                unlocked_buildings.push_back("advanced_smithy");
                break;
            case TechnologyType::WINDMILL:
                unlocked_buildings.push_back("windmill");
                break;
            case TechnologyType::WATERMILL:
                unlocked_buildings.push_back("watermill");
                break;
            case TechnologyType::PRINTING_PRESS:
                unlocked_buildings.push_back("printing_house");
                break;
            case TechnologyType::STAR_FORTRESS:
                unlocked_buildings.push_back("star_fortress");
                break;
            default:
                // No building unlock for this technology
                break;
            }
        }

        return unlocked_buildings;
    }

    /**
     * Calculate technology impact on province economy
     */
    double CalculateTechnologyEconomicImpact(types::EntityID province_id,
        const TechnologySystem& tech_system) {

        double total_impact = 0.0;
        auto implemented_tech = tech_system.GetImplementedTechnologies(province_id);

        for (auto tech : implemented_tech) {
            auto* definition = tech_system.GetTechnologyDefinition(tech);
            if (!definition) continue;

            double implementation_level = tech_system.GetImplementationLevel(province_id, tech);

            // Calculate economic impact from technology effects
            for (const auto& [effect_type, magnitude] : definition->effects) {
                if (effect_type == "agricultural_productivity" ||
                    effect_type == "craft_productivity" ||
                    effect_type == "trade_efficiency") {
                    total_impact += magnitude * implementation_level;
                }
            }
        }

        return total_impact;
    }

    /**
     * Generate technology research strategy recommendations
     */
    std::vector<std::pair<TechnologyCategory, double>> GetResearchStrategyRecommendations(
        types::EntityID province_id, const TechnologySystem& tech_system) {

        std::vector<std::pair<TechnologyCategory, double>> recommendations;

        // Analyze current technology gaps
        std::unordered_map<TechnologyCategory, int> category_counts;
        auto discovered_tech = tech_system.GetDiscoveredTechnologies(province_id);

        for (auto tech : discovered_tech) {
            auto* definition = tech_system.GetTechnologyDefinition(tech);
            if (definition) {
                category_counts[definition->category]++;
            }
        }

        // Recommend investment based on gaps and priorities
        for (auto category = TechnologyCategory::AGRICULTURAL; 
             category < TechnologyCategory::COUNT; 
             category = static_cast<TechnologyCategory>(static_cast<int>(category) + 1)) {

            int current_count = category_counts[category];
            double recommended_investment = 0.0;

            // Base recommendation on category importance and current development
            switch (category) {
            case TechnologyCategory::AGRICULTURAL:
                recommended_investment = std::max(200.0, 500.0 - (current_count * 100.0));
                break;
            case TechnologyCategory::MILITARY:
                recommended_investment = std::max(150.0, 400.0 - (current_count * 80.0));
                break;
            case TechnologyCategory::ADMINISTRATIVE:
                recommended_investment = std::max(100.0, 300.0 - (current_count * 75.0));
                break;
            case TechnologyCategory::CRAFT:
                recommended_investment = std::max(100.0, 350.0 - (current_count * 70.0));
                break;
            case TechnologyCategory::ACADEMIC:
                recommended_investment = std::max(75.0, 250.0 - (current_count * 60.0));
                break;
            case TechnologyCategory::NAVAL:
                recommended_investment = std::max(50.0, 200.0 - (current_count * 50.0));
                break;
            default:
                recommended_investment = 100.0;
                break;
            }

            recommendations.emplace_back(category, recommended_investment);
        }

        // Sort by recommended investment (highest first)
        std::sort(recommendations.begin(), recommendations.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        return recommendations;
    }

    /**
     * Create technology research milestone tracking
     */
    std::unordered_map<std::string, std::vector<std::string>> GetTechnologyMilestones(
        types::EntityID province_id, const TechnologySystem& tech_system) {

        std::unordered_map<std::string, std::vector<std::string>> milestones;

        auto discovered_tech = tech_system.GetDiscoveredTechnologies(province_id);
        auto available_research = tech_system.GetAvailableResearch(province_id);

        // Past achievements
        std::vector<std::string> achievements;
        for (auto tech : discovered_tech) {
            achievements.push_back("Discovered " + utils::TechnologyTypeToString(tech));
        }
        milestones["Past Achievements"] = achievements;

        // Current research
        std::vector<std::string> current_research;
        for (auto tech : available_research) {
            double progress = tech_system.GetResearchProgress(province_id, tech);
            if (progress > 0.0) {
                current_research.push_back(utils::TechnologyTypeToString(tech) + 
                    " (" + std::to_string(static_cast<int>(progress * 100)) + "% complete)");
            }
        }
        milestones["Current Research"] = current_research;

        // Future opportunities
        std::vector<std::string> future_opportunities;
        int opportunity_count = 0;
        for (auto tech : available_research) {
            if (opportunity_count >= 5) break; // Limit to 5 opportunities
            double progress = tech_system.GetResearchProgress(province_id, tech);
            if (progress == 0.0) { // Not yet started
                future_opportunities.push_back("Available: " + utils::TechnologyTypeToString(tech));
                opportunity_count++;
            }
        }
        milestones["Future Opportunities"] = future_opportunities;

        return milestones;
    }

} // namespace game::technology::integration
