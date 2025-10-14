// ============================================================================
// TechnologyUtils.cpp - Technology Management Integration Utilities
// Created: September 22, 2025, 18:15 UTC
// Location: src/game/technology/TechnologyUtils.cpp
// Provides integration utilities for ProvinceManagementSystem
// ============================================================================

#include "game/technology/TechnologySystem.h"
#include "game/technology/TechnologyComponents.h"
#include "core/types/game_types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>

namespace game::technology::integration {
    
    using game::technology::TechnologyType;
    using game::technology::TechnologyCategory;
    
    // Simplified management system types for integration
    enum class ManagementDecisionType : uint8_t {
        INVALID = 0,
        RESEARCH_FUNDING = 1,
        SCHOLAR_PATRONAGE = 2
    };
    
    struct DecisionOption {
        std::string option_id;
        std::string description;
        std::string tooltip;
        double cost = 0.0;
        double benefit_estimate = 0.0;
        std::vector<std::string> requirements;
        bool is_available = true;
        double ai_recommendation = 0.0;
    };

    struct DecisionContext {
        game::types::EntityID province_id{ 0 };
        ManagementDecisionType decision_type = ManagementDecisionType::INVALID;
        std::string situation_description;
        std::vector<DecisionOption> available_options;
        double urgency_factor = 0.0;
        std::chrono::system_clock::time_point deadline;
        std::unordered_map<std::string, double> numeric_data;
    };

    // ============================================================================
    // Utility Functions
    // ============================================================================

    /**
     * Convert TechnologyType enum to string
     */
    std::string TechnologyTypeToString(TechnologyType tech_type) {
        switch (tech_type) {
            case TechnologyType::THREE_FIELD_SYSTEM: return "Three Field System";
            case TechnologyType::HEAVY_PLOW: return "Heavy Plow";
            case TechnologyType::HORSE_COLLAR: return "Horse Collar";
            case TechnologyType::WINDMILL: return "Windmill";
            case TechnologyType::WATERMILL: return "Watermill";
            case TechnologyType::CHAINMAIL_ARMOR: return "Chainmail Armor";
            case TechnologyType::PLATE_ARMOR: return "Plate Armor";
            case TechnologyType::CROSSBOW: return "Crossbow";
            case TechnologyType::LONGBOW: return "Longbow";
            case TechnologyType::GUNPOWDER: return "Gunpowder";
            default: return "Unknown Technology";
        }
    }

    /**
     * Convert string to TechnologyType enum
     */
    TechnologyType StringToTechnologyType(const std::string& tech_name) {
        if (tech_name == "Three Field System") return TechnologyType::THREE_FIELD_SYSTEM;
        if (tech_name == "Heavy Plow") return TechnologyType::HEAVY_PLOW;
        if (tech_name == "Horse Collar") return TechnologyType::HORSE_COLLAR;
        if (tech_name == "Windmill") return TechnologyType::WINDMILL;
        if (tech_name == "Watermill") return TechnologyType::WATERMILL;
        if (tech_name == "Chainmail Armor") return TechnologyType::CHAINMAIL_ARMOR;
        if (tech_name == "Plate Armor") return TechnologyType::PLATE_ARMOR;
        if (tech_name == "Crossbow") return TechnologyType::CROSSBOW;
        if (tech_name == "Longbow") return TechnologyType::LONGBOW;
        if (tech_name == "Gunpowder") return TechnologyType::GUNPOWDER;
        return TechnologyType::THREE_FIELD_SYSTEM; // Default fallback
    }

    /**
     * Convert TechnologyCategory enum to string
     */
    std::string TechnologyCategoryToString(TechnologyCategory category) {
        switch (category) {
            case TechnologyCategory::AGRICULTURAL: return "Agricultural";
            case TechnologyCategory::MILITARY: return "Military";
            case TechnologyCategory::CRAFT: return "Craft";
            case TechnologyCategory::ADMINISTRATIVE: return "Administrative";
            case TechnologyCategory::ACADEMIC: return "Academic";
            case TechnologyCategory::NAVAL: return "Naval";
            default: return "Unknown Category";
        }
    }

    // ============================================================================
    // Management System Integration
    // ============================================================================

    /**
     * Generate technology research decisions for the ProvinceManagementSystem
     */
    DecisionContext GenerateTechnologyResearchDecision(types::EntityID province_id,
        const TechnologySystem& tech_system) {

        DecisionContext context;
        context.province_id = province_id;
        context.decision_type = ManagementDecisionType::RESEARCH_FUNDING;
        context.situation_description = "Technology research opportunities available";
        context.urgency_factor = 0.3; // Research is generally not urgent
        context.deadline = std::chrono::system_clock::now() + std::chrono::hours(336); // 2 weeks

        // Check if province has research component
        auto* research_component = tech_system.GetResearchComponent(province_id);
        if (!research_component) {
            // No research capability - suggest initialization
            DecisionOption init_research;
            init_research.option_id = "initialize_research";
            init_research.description = "Establish research infrastructure";
            init_research.cost = 200.0;
            init_research.benefit_estimate = 300.0;
            init_research.is_available = true;
            init_research.ai_recommendation = 0.8;
            context.available_options.push_back(init_research);
            return context;
        }

        // Create predefined research options based on medieval tech tree
        std::vector<std::pair<TechnologyType, std::pair<double, double>>> research_options = {
            {TechnologyType::THREE_FIELD_SYSTEM, {150.0, 400.0}},    // {cost, benefit}
            {TechnologyType::HEAVY_PLOW, {200.0, 350.0}},
            {TechnologyType::WINDMILL, {300.0, 500.0}},
            {TechnologyType::WATERMILL, {250.0, 450.0}},
            {TechnologyType::CROSSBOW, {180.0, 300.0}}
        };

        // Add research options
        for (const auto& [tech_type, cost_benefit] : research_options) {
            DecisionOption option;
            option.option_id = "research_" + TechnologyTypeToString(tech_type);
            option.description = "Begin research on " + TechnologyTypeToString(tech_type);
            option.cost = cost_benefit.first;
            option.benefit_estimate = cost_benefit.second;
            option.is_available = true;
            option.ai_recommendation = 0.6; // Default moderate recommendation
            context.available_options.push_back(option);
        }

        // Add "maintain current research" option
        DecisionOption maintain;
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
    DecisionContext GenerateScholarPatronageDecision(types::EntityID province_id,
        const TechnologySystem& tech_system) {

        DecisionContext context;
        context.province_id = province_id;
        context.decision_type = ManagementDecisionType::SCHOLAR_PATRONAGE;
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
            DecisionOption option;
            option.option_id = "patronage_" + std::to_string(static_cast<int>(cost));
            option.description = "Invest in scholar training and equipment";
            option.cost = cost;
            option.benefit_estimate = cost * 0.2; // 20% return estimate through research benefits
            option.is_available = true;
            option.ai_recommendation = std::max(0.3, std::min(0.9, (1000.0 - cost) / 1000.0)); // Higher recommendation for lower cost
            context.available_options.push_back(option);
        }

        // Add no investment option
        DecisionOption no_investment;
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

        if (option_id == "initialize_research") {
            // Initialize technology components for the province
            return tech_system.InitializeTechnologyComponents(province_id);
        }
        else if (option_id.find("research_") == 0) {
            // Extract technology name from option_id
            std::string tech_name = option_id.substr(9); // Remove "research_" prefix
            TechnologyType tech_type = StringToTechnologyType(tech_name);
            
            // Create research component if it doesn't exist
            if (!tech_system.GetResearchComponent(province_id)) {
                return tech_system.CreateResearchComponent(province_id);
            }
            return true;
        }
        else if (option_id.find("patronage_") == 0) {
            // Create or enhance innovation component
            if (!tech_system.GetInnovationComponent(province_id)) {
                return tech_system.CreateInnovationComponent(province_id);
            }
            return true; // Already exists, consider it successful
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
     * Create simple research order information (without management system dependency)
     */
    std::string CreateTechnologyResearchOrderInfo(types::EntityID province_id,
        TechnologyType technology, double investment) {
        
        return "Research order for " + TechnologyTypeToString(technology) + 
               " in province " + std::to_string(province_id) + 
               " with budget " + std::to_string(investment);
    }

    /**
     * Get technology research recommendations for a province
     */
    std::vector<std::string> GetTechnologyRecommendations(types::EntityID province_id,
        const TechnologySystem& tech_system) {

        std::vector<std::string> recommendations;

        // Check what technology components exist
        auto* research_component = tech_system.GetResearchComponent(province_id);
        auto* innovation_component = tech_system.GetInnovationComponent(province_id);
        auto* knowledge_component = tech_system.GetKnowledgeComponent(province_id);

        if (!research_component) {
            recommendations.push_back("Establish research infrastructure to begin technological advancement");
            return recommendations;
        }

        if (!innovation_component) {
            recommendations.push_back("Create innovation programs to boost research effectiveness");
        }

        if (!knowledge_component) {
            recommendations.push_back("Develop knowledge preservation systems to retain discoveries");
        }

        // General medieval technology recommendations
        recommendations.push_back("Focus on agricultural technologies to improve food production");
        recommendations.push_back("Develop military technologies to enhance defense capabilities");
        recommendations.push_back("Invest in craft technologies to boost economic output");
        
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

        // Check component status as proxy for progress
        auto* research_component = tech_system.GetResearchComponent(province_id);
        auto* innovation_component = tech_system.GetInnovationComponent(province_id);
        auto* knowledge_component = tech_system.GetKnowledgeComponent(province_id);
        auto* events_component = tech_system.GetTechnologyEventsComponent(province_id);

        // Calculate progress based on component existence and assumed activity
        progress_report["Research Infrastructure"] = research_component ? 0.8 : 0.0;
        progress_report["Innovation Programs"] = innovation_component ? 0.6 : 0.0;
        progress_report["Knowledge Systems"] = knowledge_component ? 0.7 : 0.0;
        progress_report["Technology Events"] = events_component ? 0.5 : 0.0;

        // Calculate overall progress
        double total_progress = 0.0;
        int component_count = 0;
        for (const auto& [category, progress] : progress_report) {
            total_progress += progress;
            component_count++;
        }

        if (component_count > 0) {
            progress_report["Overall Technology Level"] = total_progress / component_count;
        }

        return progress_report;
    }

    /**
     * Generate technology crisis events for management system
     */
    DecisionContext GenerateTechnologyCrisisDecision(types::EntityID province_id,
        const std::string& crisis_type) {

        DecisionContext context;
        context.province_id = province_id;
        context.decision_type = ManagementDecisionType::RESEARCH_FUNDING;
        context.urgency_factor = 0.7; // Crises are more urgent

        if (crisis_type == "scholar_exodus") {
            context.situation_description = "Scholars are leaving due to lack of funding and support";
            context.deadline = std::chrono::system_clock::now() + std::chrono::hours(168); // 1 week

            // Emergency funding option
            DecisionOption emergency_funding;
            emergency_funding.option_id = "emergency_scholar_funding";
            emergency_funding.description = "Provide emergency funding to retain scholars";
            emergency_funding.cost = 500.0;
            emergency_funding.benefit_estimate = 300.0;
            emergency_funding.is_available = true;
            emergency_funding.ai_recommendation = 0.8;
            context.available_options.push_back(emergency_funding);

            // Let them leave option
            DecisionOption accept_exodus;
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
            DecisionOption invite_scholars;
            invite_scholars.option_id = "invite_foreign_scholars";
            invite_scholars.description = "Invite foreign scholars to bring new ideas";
            invite_scholars.cost = 300.0;
            invite_scholars.benefit_estimate = 400.0;
            invite_scholars.is_available = true;
            invite_scholars.ai_recommendation = 0.7;
            context.available_options.push_back(invite_scholars);

            // Research reform option
            DecisionOption reform_research;
            reform_research.option_id = "reform_research_methods";
            reform_research.description = "Reform research methods and institutions";
            reform_research.cost = 200.0;
            reform_research.benefit_estimate = 250.0;
            reform_research.is_available = true;
            reform_research.ai_recommendation = 0.6;
            context.available_options.push_back(reform_research);

            // Continue as normal
            DecisionOption continue_normal;
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
     * Handle technology breakthrough events (simplified version)
     */
    void HandleTechnologyBreakthrough(types::EntityID province_id, TechnologyType technology) {
        // Simple logging of breakthrough without management system dependency
        // In a full implementation, this would integrate with the management system
        // TODO: Integrate with actual decision system when available
        
        // Log the technology breakthrough
        std::string log_message = "Technology breakthrough in province " + 
            std::to_string(province_id) + ": " + TechnologyTypeToString(technology);
        
        // In a real implementation, this would notify other systems
        // For now, we just acknowledge the breakthrough
    }

    /**
     * Update technology effects in other systems
     */
    void UpdateTechnologyEffects(types::EntityID province_id, TechnologyType technology,
        double implementation_level) {

        // This function would integrate with other systems to apply technology effects
        // For now, it's a placeholder for future integration

        // Log technology effect application (simplified without core::logging dependency)
        std::string effect_message = "Applied technology effects for " + TechnologyTypeToString(technology) + 
            " at " + std::to_string(static_cast<int>(implementation_level * 100)) + "% implementation";

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

        // Check if technology components exist to determine available buildings
        auto* research_component = tech_system.GetResearchComponent(province_id);
        auto* innovation_component = tech_system.GetInnovationComponent(province_id);
        auto* knowledge_component = tech_system.GetKnowledgeComponent(province_id);

        // Basic buildings available with research component
        if (research_component) {
            unlocked_buildings.push_back("basic_workshop");
            unlocked_buildings.push_back("scribal_school");
        }

        // Advanced buildings with innovation
        if (innovation_component) {
            unlocked_buildings.push_back("windmill");
            unlocked_buildings.push_back("watermill");
            unlocked_buildings.push_back("advanced_smithy");
        }

        // High-tier buildings with knowledge systems
        if (knowledge_component) {
            unlocked_buildings.push_back("library");
            unlocked_buildings.push_back("university");
        }

        return unlocked_buildings;
    }

    /**
     * Calculate technology impact on province economy
     */
    double CalculateTechnologyEconomicImpact(types::EntityID province_id,
        const TechnologySystem& tech_system) {

        double total_impact = 0.0;

        // Calculate impact based on technology components
        auto* research_component = tech_system.GetResearchComponent(province_id);
        auto* innovation_component = tech_system.GetInnovationComponent(province_id);
        auto* knowledge_component = tech_system.GetKnowledgeComponent(province_id);

        // Each component provides economic benefits
        if (research_component) {
            total_impact += 0.15; // 15% base economic boost from research
        }

        if (innovation_component) {
            total_impact += 0.20; // 20% boost from innovation
        }

        if (knowledge_component) {
            total_impact += 0.10; // 10% boost from knowledge preservation
        }

        return total_impact;
    }

    /**
     * Generate technology research strategy recommendations
     */
    std::vector<std::pair<TechnologyCategory, double>> GetResearchStrategyRecommendations(
        types::EntityID province_id, const TechnologySystem& tech_system) {

        std::vector<std::pair<TechnologyCategory, double>> recommendations;

        // Check technology component status
        auto* research_component = tech_system.GetResearchComponent(province_id);
        auto* innovation_component = tech_system.GetInnovationComponent(province_id);
        auto* knowledge_component = tech_system.GetKnowledgeComponent(province_id);

        // Base recommendations on medieval priorities and component status
        double agricultural_investment = research_component ? 300.0 : 500.0;
        double military_investment = innovation_component ? 250.0 : 400.0;
        double craft_investment = knowledge_component ? 200.0 : 350.0;
        double administrative_investment = 200.0;
        double academic_investment = 150.0;
        double naval_investment = 100.0;

        recommendations.emplace_back(TechnologyCategory::AGRICULTURAL, agricultural_investment);
        recommendations.emplace_back(TechnologyCategory::MILITARY, military_investment);
        recommendations.emplace_back(TechnologyCategory::CRAFT, craft_investment);
        recommendations.emplace_back(TechnologyCategory::ADMINISTRATIVE, administrative_investment);
        recommendations.emplace_back(TechnologyCategory::ACADEMIC, academic_investment);
        recommendations.emplace_back(TechnologyCategory::NAVAL, naval_investment);

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

        // Check technology component status for milestones
        auto* research_component = tech_system.GetResearchComponent(province_id);
        auto* innovation_component = tech_system.GetInnovationComponent(province_id);
        auto* knowledge_component = tech_system.GetKnowledgeComponent(province_id);
        auto* events_component = tech_system.GetTechnologyEventsComponent(province_id);

        // Past achievements based on components
        std::vector<std::string> achievements;
        if (research_component) {
            achievements.push_back("Established research infrastructure");
        }
        if (innovation_component) {
            achievements.push_back("Developed innovation programs");
        }
        if (knowledge_component) {
            achievements.push_back("Built knowledge preservation systems");
        }
        if (events_component) {
            achievements.push_back("Activated technology event tracking");
        }
        milestones["Past Achievements"] = achievements;

        // Current research status
        std::vector<std::string> current_research;
        if (research_component && innovation_component) {
            current_research.push_back("Advanced research in progress");
        } else if (research_component) {
            current_research.push_back("Basic research in progress");
        }
        milestones["Current Research"] = current_research;

        // Future opportunities
        std::vector<std::string> future_opportunities;
        if (!research_component) {
            future_opportunities.push_back("Available: Establish research infrastructure");
        }
        if (!innovation_component) {
            future_opportunities.push_back("Available: Develop innovation programs");
        }
        if (!knowledge_component) {
            future_opportunities.push_back("Available: Build knowledge systems");
        }
        future_opportunities.push_back("Available: " + TechnologyTypeToString(TechnologyType::THREE_FIELD_SYSTEM));
        future_opportunities.push_back("Available: " + TechnologyTypeToString(TechnologyType::WINDMILL));
        milestones["Future Opportunities"] = future_opportunities;

        return milestones;
    }

} // namespace game::technology::integration
