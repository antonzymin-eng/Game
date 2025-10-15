// ============================================================================
// ProvinceManagementUtils.cpp - Management System Utilities Implementation
// Created: September 22, 2025, 17:45 UTC
// Location: src/game/management/ProvinceManagementUtils.cpp
// Utility functions for the updated Province Management System
// ============================================================================

#include "game/province/ProvinceManagementSystem.h"
#include <unordered_map>

namespace game::management::utils {

    // ============================================================================
    // String Conversion Utilities Implementation
    // ============================================================================

    std::string ManagementDecisionTypeToString(ManagementDecisionType type) {
        static const std::unordered_map<ManagementDecisionType, std::string> type_names = {
            {ManagementDecisionType::TAX_RATE_ADJUSTMENT, "Tax Rate Adjustment"},
            {ManagementDecisionType::BUDGET_ALLOCATION, "Budget Allocation"},
            {ManagementDecisionType::TRADE_POLICY_CHANGE, "Trade Policy Change"},
            {ManagementDecisionType::BUILDING_CONSTRUCTION, "Building Construction"},
            {ManagementDecisionType::INFRASTRUCTURE_DEVELOPMENT, "Infrastructure Development"},
            {ManagementDecisionType::MIGRATION_POLICY, "Migration Policy"},
            {ManagementDecisionType::SOCIAL_SERVICES, "Social Services"},
            {ManagementDecisionType::RESEARCH_FUNDING, "Research Funding"},
            {ManagementDecisionType::SCHOLAR_PATRONAGE, "Scholar Patronage"},
            {ManagementDecisionType::OFFICIAL_APPOINTMENT, "Official Appointment"},
            {ManagementDecisionType::BUREAUCRACY_REFORM, "Bureaucracy Reform"},
            {ManagementDecisionType::RECRUITMENT_ORDER, "Recruitment Order"},
            {ManagementDecisionType::GARRISON_ASSIGNMENT, "Garrison Assignment"},
            {ManagementDecisionType::INVALID, "Invalid"}
        };

        auto it = type_names.find(type);
        return (it != type_names.end()) ? it->second : "Unknown";
    }

    std::string DecisionPriorityToString(DecisionPriority priority) {
        static const std::unordered_map<DecisionPriority, std::string> priority_names = {
            {DecisionPriority::ROUTINE, "Routine"},
            {DecisionPriority::IMPORTANT, "Important"},
            {DecisionPriority::URGENT, "Urgent"},
            {DecisionPriority::CRITICAL, "Critical"}
        };

        auto it = priority_names.find(priority);
        return (it != priority_names.end()) ? it->second : "Unknown";
    }

    std::string DecisionStatusToString(DecisionStatus status) {
        static const std::unordered_map<DecisionStatus, std::string> status_names = {
            {DecisionStatus::PENDING, "Pending"},
            {DecisionStatus::APPROVED, "Approved"},
            {DecisionStatus::REJECTED, "Rejected"},
            {DecisionStatus::DELEGATED, "Delegated"},
            {DecisionStatus::EXECUTED, "Executed"},
            {DecisionStatus::FAILED, "Failed"}
        };

        auto it = status_names.find(status);
        return (it != status_names.end()) ? it->second : "Unknown";
    }

    std::string AutomationLevelToString(AutomationLevel level) {
        static const std::unordered_map<AutomationLevel, std::string> level_names = {
            {AutomationLevel::MANUAL, "Manual"},
            {AutomationLevel::ASSISTED, "Assisted"},
            {AutomationLevel::GUIDED, "Guided"},
            {AutomationLevel::AUTOMATED, "Automated"}
        };

        auto it = level_names.find(level);
        return (it != level_names.end()) ? it->second : "Unknown";
    }

    std::string OrderTypeToString(OrderType type) {
        static const std::unordered_map<OrderType, std::string> type_names = {
            {OrderType::CONSTRUCTION_ORDER, "Construction Order"},
            {OrderType::POLICY_CHANGE, "Policy Change"},
            {OrderType::RESOURCE_ALLOCATION, "Resource Allocation"},
            {OrderType::RESEARCH_ORDER, "Research Order"}
        };

        auto it = type_names.find(type);
        return (it != type_names.end()) ? it->second : "Unknown";
    }

    std::string OrderStatusToString(OrderStatus status) {
        static const std::unordered_map<OrderStatus, std::string> status_names = {
            {OrderStatus::QUEUED, "Queued"},
            {OrderStatus::IN_PROGRESS, "In Progress"},
            {OrderStatus::COMPLETED, "Completed"},
            {OrderStatus::FAILED, "Failed"},
            {OrderStatus::CANCELLED, "Cancelled"}
        };

        auto it = status_names.find(status);
        return (it != status_names.end()) ? it->second : "Unknown";
    }

    // ============================================================================
    // Factory Methods Implementation
    // ============================================================================

    ManagementComponent CreateManagement(types::EntityID province_id, const std::string& manager_name) {
        ManagementComponent mgmt(province_id);
        mgmt.manager_name = manager_name;
        mgmt.player_controlled = true;
        mgmt.automation_level = AutomationLevel::ASSISTED;
        mgmt.decisions_pending = 0;
        mgmt.decisions_completed = 0;
        mgmt.administrative_efficiency = 1.0;
        return mgmt;
    }

    PlayerPolicyComponent CreateDefaultPolicies() {
        PlayerPolicyComponent policy;
        policy.base_tax_rate = 0.1;              // 10% tax rate
        policy.trade_policy_openness = 0.5;      // Balanced trade policy
        policy.social_services_funding = 0.5;    // Moderate social services
        policy.research_funding_level = 0.5;     // Moderate research funding
        policy.military_focus = 0.5;             // Balanced military focus
        policy.bureaucratic_centralization = 0.5; // Balanced centralization
        return policy;
    }

    std::unique_ptr<PlayerDecision> CreateEconomicDecision(types::EntityID province_id,
                                                          ManagementDecisionType type) {
        DecisionContext context;
        context.province_id = province_id;
        context.decision_type = type;
        context.situation_description = "Economic decision required";
        context.urgency_factor = 0.5;
        context.deadline = std::chrono::system_clock::now() + std::chrono::hours(72);

        // Add default economic options
        DecisionOption maintain_option;
        maintain_option.option_id = "maintain_current";
        maintain_option.description = "Maintain current economic policies";
        maintain_option.cost = 0.0;
        maintain_option.benefit_estimate = 0.0;
        maintain_option.is_available = true;
        maintain_option.ai_recommendation = 0.5;
        context.available_options.push_back(maintain_option);

        return std::make_unique<PlayerDecision>(context);
    }

    std::unique_ptr<ProvinceOrder> CreateConstructionOrder(types::EntityID province_id,
                                                          province::ProductionBuilding building) {
        auto order = std::make_unique<ProvinceOrder>(OrderType::CONSTRUCTION_ORDER, province_id);
        order->order_description = "Construct " + province::utils::ProductionBuildingToString(building);
        order->parameters["building_type"] = std::to_string(static_cast<int>(building));
        order->estimated_cost = 200.0; // Default estimate
        order->can_execute = false; // Will be validated by system
        return order;
    }

    // ============================================================================
    // Validation Utilities Implementation
    // ============================================================================

    bool IsValidDecisionType(ManagementDecisionType type) {
        return type != ManagementDecisionType::INVALID && 
               type < ManagementDecisionType::COUNT;
    }

    bool IsValidAutomationLevel(AutomationLevel level) {
        return level < AutomationLevel::COUNT;
    }

    bool CanExecuteOrder(const ProvinceOrder& order) {
        // Basic validation - order must have valid target and description
        if (order.target_province == types::EntityID{0}) {
            return false;
        }
        
        if (order.order_description.empty()) {
            return false;
        }
        
        // Type-specific validation
        switch (order.order_type) {
        case OrderType::CONSTRUCTION_ORDER:
            return order.parameters.find("building_type") != order.parameters.end();
            
        case OrderType::POLICY_CHANGE:
            return order.parameters.find("policy_name") != order.parameters.end() &&
                   order.parameters.find("new_value") != order.parameters.end();
            
        case OrderType::RESOURCE_ALLOCATION:
            return order.estimated_cost >= 0.0;
            
        case OrderType::RESEARCH_ORDER:
            return order.parameters.find("research_type") != order.parameters.end();
            
        default:
            return false;
        }
    }

    // ============================================================================
    // Decision Analysis Utilities
    // ============================================================================

    double CalculateDecisionUrgency(const DecisionContext& context) {
        double urgency = context.urgency_factor;
        
        // Increase urgency based on deadline proximity
        auto now = std::chrono::system_clock::now();
        auto time_remaining = std::chrono::duration_cast<std::chrono::hours>(context.deadline - now).count();
        
        if (time_remaining <= 24) {
            urgency = std::max(urgency, 0.8); // Very urgent if less than 24 hours
        }
        else if (time_remaining <= 72) {
            urgency = std::max(urgency, 0.6); // Urgent if less than 3 days
        }
        
        return std::clamp(urgency, 0.0, 1.0);
    }

    std::string GetDecisionRecommendation(const DecisionContext& context) {
        if (context.available_options.empty()) {
            return "No options available";
        }
        
        // Find option with highest AI recommendation
        const DecisionOption* best_option = nullptr;
        double best_score = -1.0;
        
        for (const auto& option : context.available_options) {
            if (option.is_available && option.ai_recommendation > best_score) {
                best_score = option.ai_recommendation;
                best_option = &option;
            }
        }
        
        if (best_option) {
            return "Recommended: " + best_option->description + 
                   " (Confidence: " + std::to_string(static_cast<int>(best_score * 100)) + "%)";
        }
        
        return "No clear recommendation available";
    }

    double EstimateDecisionImpact(const DecisionContext& context, const std::string& option_id) {
        for (const auto& option : context.available_options) {
            if (option.option_id == option_id) {
                // Simple impact calculation based on benefit vs cost
                if (option.cost > 0.0) {
                    return option.benefit_estimate / option.cost;
                }
                return option.benefit_estimate;
            }
        }
        return 0.0;
    }

    // ============================================================================
    // Management Analysis Utilities
    // ============================================================================

    double CalculateManagementEfficiency(const ManagementComponent& management) {
        double efficiency = management.administrative_efficiency;
        
        // Reduce efficiency if too many pending decisions
        if (management.decisions_pending > 5) {
            efficiency *= 0.8;
        }
        else if (management.decisions_pending > 10) {
            efficiency *= 0.6;
        }
        
        // Improve efficiency based on completion rate
        if (management.decisions_completed > 0) {
            double completion_ratio = static_cast<double>(management.decisions_completed) /
                                    (management.decisions_completed + management.decisions_pending);
            efficiency *= (0.5 + completion_ratio * 0.5);
        }
        
        return std::clamp(efficiency, 0.1, 1.0);
    }

    std::vector<std::string> IdentifyManagementIssues(const ManagementComponent& management) {
        std::vector<std::string> issues;
        
        if (management.decisions_pending > 10) {
            issues.push_back("High number of pending decisions");
        }
        
        if (management.administrative_efficiency < 0.5) {
            issues.push_back("Low administrative efficiency");
        }
        
        if (management.automation_level == AutomationLevel::MANUAL && management.decisions_pending > 5) {
            issues.push_back("Manual management with high decision load");
        }
        
        return issues;
    }

    double CalculateGovernanceScore(const ManagementComponent& management, 
                                   const PlayerPolicyComponent& policies) {
        double score = 0.0;
        
        // Management efficiency component (40%)
        score += CalculateManagementEfficiency(management) * 0.4;
        
        // Policy balance component (30%)
        double policy_balance = 1.0 - std::abs(0.5 - policies.base_tax_rate) * 2.0;
        policy_balance += 1.0 - std::abs(0.5 - policies.social_services_funding) * 2.0;
        policy_balance += 1.0 - std::abs(0.5 - policies.trade_policy_openness) * 2.0;
        score += (policy_balance / 3.0) * 0.3;
        
        // Responsiveness component (30%)
        double responsiveness = (management.decisions_completed > 0) ? 
            static_cast<double>(management.decisions_completed) / 
            (management.decisions_completed + management.decisions_pending) : 0.5;
        score += responsiveness * 0.3;
        
        return std::clamp(score, 0.0, 1.0);
    }

    std::unordered_map<std::string, double> GetKPIDashboard(const ManagementComponent& management,
                                                           const PlayerPolicyComponent& policies) {
        std::unordered_map<std::string, double> kpis;
        
        kpis["management_efficiency"] = CalculateManagementEfficiency(management);
        kpis["decisions_pending"] = management.decisions_pending;
        kpis["decisions_completed"] = management.decisions_completed;
        kpis["governance_score"] = CalculateGovernanceScore(management, policies);
        kpis["tax_rate"] = policies.base_tax_rate;
        kpis["social_services"] = policies.social_services_funding;
        kpis["trade_openness"] = policies.trade_policy_openness;
        
        // Decision completion rate
        if (management.decisions_completed > 0) {
            kpis["completion_rate"] = static_cast<double>(management.decisions_completed) /
                                     (management.decisions_completed + management.decisions_pending);
        } else {
            kpis["completion_rate"] = 0.0;
        }
        
        return kpis;
    }

    // ============================================================================
    // Order Management Utilities
    // ============================================================================

    std::string GenerateOrderDescription(OrderType type, const std::unordered_map<std::string, std::string>& parameters) {
        switch (type) {
        case OrderType::CONSTRUCTION_ORDER: {
            auto building_param = parameters.find("building_type");
            if (building_param != parameters.end()) {
                int building_type = std::stoi(building_param->second);
                return "Construct " + province::utils::ProductionBuildingToString(
                    static_cast<province::ProductionBuilding>(building_type));
            }
            return "Construction Order";
        }
        
        case OrderType::POLICY_CHANGE: {
            auto policy_param = parameters.find("policy_name");
            auto value_param = parameters.find("new_value");
            if (policy_param != parameters.end() && value_param != parameters.end()) {
                return "Change " + policy_param->second + " to " + value_param->second;
            }
            return "Policy Change Order";
        }
        
        case OrderType::RESOURCE_ALLOCATION: {
            auto resource_param = parameters.find("resource_type");
            auto amount_param = parameters.find("amount");
            if (resource_param != parameters.end() && amount_param != parameters.end()) {
                return "Allocate " + amount_param->second + " " + resource_param->second;
            }
            return "Resource Allocation Order";
        }
        
        case OrderType::RESEARCH_ORDER: {
            auto research_param = parameters.find("research_type");
            if (research_param != parameters.end()) {
                return "Research " + research_param->second;
            }
            return "Research Order";
        }
        
        default:
            return "Unknown Order";
        }
    }

    double EstimateOrderExecutionTime(const ProvinceOrder& order) {
        switch (order.order_type) {
        case OrderType::CONSTRUCTION_ORDER:
            return 30.0; // 30 days for construction
            
        case OrderType::POLICY_CHANGE:
            return 1.0; // 1 day for policy changes
            
        case OrderType::RESOURCE_ALLOCATION:
            return 7.0; // 1 week for resource allocation
            
        case OrderType::RESEARCH_ORDER:
            return 90.0; // 3 months for research
            
        default:
            return 7.0; // Default 1 week
        }
    }

    bool ValidateOrderParameters(OrderType type, const std::unordered_map<std::string, std::string>& parameters) {
        switch (type) {
        case OrderType::CONSTRUCTION_ORDER:
            return parameters.find("building_type") != parameters.end();
            
        case OrderType::POLICY_CHANGE:
            return parameters.find("policy_name") != parameters.end() &&
                   parameters.find("new_value") != parameters.end();
            
        case OrderType::RESOURCE_ALLOCATION:
            return parameters.find("resource_type") != parameters.end() &&
                   parameters.find("amount") != parameters.end();
            
        case OrderType::RESEARCH_ORDER:
            return parameters.find("research_type") != parameters.end();
            
        default:
            return false;
        }
    }

} // namespace game::management::utils
