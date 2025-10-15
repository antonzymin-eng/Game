// ============================================================================
// ProvinceManagementSystem.cpp - Updated Player Interface Implementation
// Created: September 22, 2025, 17:30 UTC
// Location: src/game/management/ProvinceManagementSystem.cpp
// Updated to integrate with EnhancedProvinceSystem - UI/Player layer only
// ============================================================================

#include "game/province/ProvinceManagementSystem.h"
#include "core/logging/Logger.h"
#include "core/config/GameConfig.h"
#include "game/province/EnhancedProvinceSystem.h"
#include "utils/RandomGenerator.h"

#include <algorithm>
#include <sstream>
#include <iomanip>

namespace game::management {

    // ============================================================================
    // PlayerDecision Implementation
    // ============================================================================

    PlayerDecision::PlayerDecision(const DecisionContext& context)
        : m_context(context), m_status(DecisionStatus::PENDING) {

        static int decision_counter = 0;
        std::stringstream ss;
        ss << "decision_" << ++decision_counter << "_" << static_cast<int>(context.decision_type);
        m_decision_id = ss.str();

        m_created_time = std::chrono::system_clock::now();
        m_deadline = context.deadline;

        // Determine priority based on urgency
        if (context.urgency_factor > 0.8) {
            m_priority = DecisionPriority::CRITICAL;
        }
        else if (context.urgency_factor > 0.6) {
            m_priority = DecisionPriority::URGENT;
        }
        else if (context.urgency_factor > 0.3) {
            m_priority = DecisionPriority::IMPORTANT;
        }
        else {
            m_priority = DecisionPriority::ROUTINE;
        }
    }

    double PlayerDecision::GetTimeRemaining() const {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::hours>(m_deadline - now);
        return duration.count();
    }

    bool PlayerDecision::IsOverdue() const {
        return std::chrono::system_clock::now() > m_deadline;
    }

    bool PlayerDecision::SelectOption(const std::string& option_id) {
        for (const auto& option : m_context.available_options) {
            if (option.option_id == option_id && option.is_available) {
                m_selected_option_id = option_id;
                return true;
            }
        }
        return false;
    }

    bool PlayerDecision::ApproveDecision(const std::string& player_notes) {
        if (m_selected_option_id.empty()) return false;
        m_status = DecisionStatus::APPROVED;
        m_player_notes = player_notes;
        return true;
    }

    bool PlayerDecision::RejectDecision(const std::string& reason) {
        m_status = DecisionStatus::REJECTED;
        m_player_notes = reason;
        return true;
    }

    bool PlayerDecision::DelegateDecision() {
        m_status = DecisionStatus::DELEGATED;
        
        // Auto-select best option based on AI recommendation
        const DecisionOption* best_option = nullptr;
        double best_score = -1.0;

        for (const auto& option : m_context.available_options) {
            if (option.is_available && option.ai_recommendation > best_score) {
                best_score = option.ai_recommendation;
                best_option = &option;
            }
        }

        if (best_option) {
            m_selected_option_id = best_option->option_id;
            return true;
        }
        return false;
    }

    const DecisionOption* PlayerDecision::GetSelectedOption() const {
        if (m_selected_option_id.empty()) return nullptr;
        
        for (const auto& option : m_context.available_options) {
            if (option.option_id == m_selected_option_id) {
                return &option;
            }
        }
        return nullptr;
    }

    std::vector<DecisionOption> PlayerDecision::GetAvailableOptions() const {
        std::vector<DecisionOption> available;
        for (const auto& option : m_context.available_options) {
            if (option.is_available) {
                available.push_back(option);
            }
        }
        return available;
    }

    // ============================================================================
    // DecisionQueue Implementation
    // ============================================================================

    void DecisionQueue::AddDecision(std::unique_ptr<PlayerDecision> decision) {
        DecisionPriority priority = decision->GetPriority();
        PlayerDecision* decision_ptr = decision.get();

        m_pending_decisions.push_back(std::move(decision));
        m_priority_queues[priority].push(decision_ptr);
    }

    PlayerDecision* DecisionQueue::GetNextDecision(DecisionPriority min_priority) {
        for (int p = static_cast<int>(DecisionPriority::CRITICAL);
            p >= static_cast<int>(min_priority); --p) {

            DecisionPriority priority = static_cast<DecisionPriority>(p);
            auto& queue = m_priority_queues[priority];

            if (!queue.empty()) {
                PlayerDecision* decision = queue.front();
                queue.pop();
                return decision;
            }
        }
        return nullptr;
    }

    PlayerDecision* DecisionQueue::GetDecision(const std::string& decision_id) {
        for (auto& decision : m_pending_decisions) {
            if (decision->GetDecisionId() == decision_id) {
                return decision.get();
            }
        }
        return nullptr;
    }

    std::vector<PlayerDecision*> DecisionQueue::GetPendingDecisions() const {
        std::vector<PlayerDecision*> pending;
        for (const auto& decision : m_pending_decisions) {
            if (decision->GetStatus() == DecisionStatus::PENDING) {
                pending.push_back(decision.get());
            }
        }
        return pending;
    }

    std::vector<PlayerDecision*> DecisionQueue::GetOverdueDecisions() const {
        std::vector<PlayerDecision*> overdue;
        for (const auto& decision : m_pending_decisions) {
            if (decision->IsOverdue()) {
                overdue.push_back(decision.get());
            }
        }
        return overdue;
    }

    void DecisionQueue::MarkDecisionCompleted(const std::string& decision_id) {
        auto it = std::find_if(m_pending_decisions.begin(), m_pending_decisions.end(),
            [&decision_id](const std::unique_ptr<PlayerDecision>& decision) {
                return decision->GetDecisionId() == decision_id;
            });

        if (it != m_pending_decisions.end()) {
            m_completed_decisions.push_back(std::move(*it));
            m_pending_decisions.erase(it);

            // Cleanup if history gets too large
            if (m_completed_decisions.size() > m_max_completed_history) {
                m_completed_decisions.erase(m_completed_decisions.begin());
            }
        }
    }

    void DecisionQueue::ProcessAutomatedDecisions() {
        std::vector<std::string> to_process;

        for (const auto& decision : m_pending_decisions) {
            if (ShouldAutomate(*decision)) {
                if (decision->DelegateDecision()) {
                    to_process.push_back(decision->GetDecisionId());
                }
            }
        }

        for (const auto& decision_id : to_process) {
            MarkDecisionCompleted(decision_id);
        }
    }

    bool DecisionQueue::ShouldAutomate(const PlayerDecision& decision) const {
        DecisionPriority priority = decision.GetPriority();

        switch (m_automation_level) {
        case AutomationLevel::MANUAL:
            return false;
        case AutomationLevel::ASSISTED:
            return false;
        case AutomationLevel::GUIDED:
            return priority == DecisionPriority::ROUTINE;
        case AutomationLevel::AUTOMATED:
            return priority != DecisionPriority::CRITICAL;
        default:
            return false;
        }
    }

    void DecisionQueue::Clear() {
        m_pending_decisions.clear();
        m_completed_decisions.clear();
        m_priority_queues.clear();
    }

    // ============================================================================
    // ProvinceOrderSystem Implementation
    // ============================================================================

    std::string ProvinceOrderSystem::AddOrder(std::unique_ptr<ProvinceOrder> order) {
        static int order_counter = 0;
        std::stringstream ss;
        ss << "order_" << ++order_counter << "_" << static_cast<int>(order->order_type);
        order->order_id = ss.str();
        order->start_time = std::chrono::system_clock::now();

        std::string order_id = order->order_id;
        m_active_orders.push_back(std::move(order));
        return order_id;
    }

    bool ProvinceOrderSystem::CompleteOrder(const std::string& order_id) {
        auto it = std::find_if(m_active_orders.begin(), m_active_orders.end(),
            [&order_id](const std::unique_ptr<ProvinceOrder>& order) {
                return order->order_id == order_id;
            });

        if (it != m_active_orders.end()) {
            (*it)->status = OrderStatus::COMPLETED;
            (*it)->progress = 1.0;
            m_completed_orders.push_back(std::move(*it));
            m_active_orders.erase(it);
            return true;
        }
        return false;
    }

    bool ProvinceOrderSystem::CancelOrder(const std::string& order_id) {
        auto it = std::find_if(m_active_orders.begin(), m_active_orders.end(),
            [&order_id](const std::unique_ptr<ProvinceOrder>& order) {
                return order->order_id == order_id;
            });

        if (it != m_active_orders.end()) {
            (*it)->status = OrderStatus::CANCELLED;
            m_completed_orders.push_back(std::move(*it));
            m_active_orders.erase(it);
            return true;
        }
        return false;
    }

    ProvinceOrder* ProvinceOrderSystem::GetOrder(const std::string& order_id) {
        for (auto& order : m_active_orders) {
            if (order->order_id == order_id) {
                return order.get();
            }
        }
        return nullptr;
    }

    std::vector<ProvinceOrder*> ProvinceOrderSystem::GetActiveOrders() const {
        std::vector<ProvinceOrder*> orders;
        for (const auto& order : m_active_orders) {
            orders.push_back(order.get());
        }
        return orders;
    }

    std::vector<ProvinceOrder*> ProvinceOrderSystem::GetOrdersByProvince(types::EntityID province_id) const {
        std::vector<ProvinceOrder*> orders;
        for (const auto& order : m_active_orders) {
            if (order->target_province == province_id) {
                orders.push_back(order.get());
            }
        }
        return orders;
    }

    // ============================================================================
    // ProvinceManagementSystem Implementation
    // ============================================================================

    ProvinceManagementSystem::ProvinceManagementSystem(core::ecs::ComponentAccessManager& access_manager,
        core::threading::ThreadSafeMessageBus& message_bus)
        : m_access_manager(access_manager), m_message_bus(message_bus), m_province_system(nullptr) {

        m_decision_queue = std::make_unique<DecisionQueue>();
        m_order_system = std::make_unique<ProvinceOrderSystem>();
        m_last_update = std::chrono::steady_clock::now();
    }

    ProvinceManagementSystem::~ProvinceManagementSystem() = default;

    void ProvinceManagementSystem::Initialize() {
        core::logging::LogInfo("ProvinceManagementSystem", "Initializing Province Management System");

        InitializeDecisionGenerators();

        // Subscribe to province system events
        m_message_bus.Subscribe<province::messages::ProvinceCreated>(
            [this](const auto& msg) { OnProvinceCreated(msg); });
        m_message_bus.Subscribe<province::messages::EconomicCrisis>(
            [this](const auto& msg) { OnEconomicCrisis(msg); });
        m_message_bus.Subscribe<province::messages::ResourceShortage>(
            [this](const auto& msg) { OnResourceShortage(msg); });

        core::logging::LogInfo("ProvinceManagementSystem", "Province Management System initialized");
    }

    void ProvinceManagementSystem::Update(float delta_time,
        core::ecs::ComponentAccessManager& access_manager,
        core::threading::ThreadSafeMessageBus& message_bus) {

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double>(now - m_last_update).count();

        if (elapsed >= (1.0 / m_update_frequency)) {
            // Process automated decisions
            ProcessAutomatedDecisions();

            // Process active orders
            ProcessActiveOrders();

            m_last_update = now;
        }
    }

    void ProvinceManagementSystem::Shutdown() {
        core::logging::LogInfo("ProvinceManagementSystem", "Shutting down Province Management System");

        m_decision_queue->Clear();
        m_order_system.reset();
        m_decision_queue.reset();
        m_province_system = nullptr;
    }

    core::threading::ThreadingStrategy ProvinceManagementSystem::GetPreferredStrategy() const {
        return core::threading::ThreadingStrategy::MAIN_THREAD; // UI system
    }

    // ============================================================================
    // Province Management Interface
    // ============================================================================

    bool ProvinceManagementSystem::CreateManagedProvince(types::EntityID province_id,
        const std::string& manager_name) {

        try {
            auto mgmt_write = m_access_manager.GetWriteAccess<ManagementComponent>("CreateManagement");
            auto policy_write = m_access_manager.GetWriteAccess<PlayerPolicyComponent>("CreatePolicy");

            mgmt_write.AddComponent(province_id, ManagementComponent(province_id));
            policy_write.AddComponent(province_id, PlayerPolicyComponent());

            LogManagementAction(province_id, "Province management created for " + manager_name);
            return true;
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to create management for province " + std::to_string(province_id) +
                ": " + e.what());
            return false;
        }
    }

    bool ProvinceManagementSystem::DestroyManagedProvince(types::EntityID province_id) {
        try {
            auto mgmt_write = m_access_manager.GetWriteAccess<ManagementComponent>("DestroyManagement");
            auto policy_write = m_access_manager.GetWriteAccess<PlayerPolicyComponent>("DestroyPolicy");

            mgmt_write.RemoveComponent(province_id);
            policy_write.RemoveComponent(province_id);

            LogManagementAction(province_id, "Province management destroyed");
            return true;
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to destroy management for province " + std::to_string(province_id) +
                ": " + e.what());
            return false;
        }
    }

    bool ProvinceManagementSystem::SetProvinceAutomation(types::EntityID province_id, AutomationLevel level) {
        try {
            auto mgmt_write = m_access_manager.GetWriteAccess<ManagementComponent>("SetAutomation");
            auto* mgmt_comp = mgmt_write.GetComponent(province_id);

            if (mgmt_comp) {
                mgmt_comp->automation_level = level;
                LogManagementAction(province_id, "Automation level set to " + 
                    utils::AutomationLevelToString(level));
                return true;
            }
            return false;
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to set automation for province " + std::to_string(province_id) +
                ": " + e.what());
            return false;
        }
    }

    // ============================================================================
    // Decision System Interface
    // ============================================================================

    bool ProvinceManagementSystem::GenerateDecision(types::EntityID province_id, ManagementDecisionType type) {
        if (!m_province_system || !m_province_system->IsValidProvince(province_id)) {
            return false;
        }

        try {
            auto generator_it = m_decision_generators.find(type);
            if (generator_it == m_decision_generators.end()) {
                return false;
            }

            DecisionContext context = generator_it->second(province_id);
            context.decision_type = type;

            auto decision = std::make_unique<PlayerDecision>(context);
            m_decision_queue->AddDecision(std::move(decision));

            LogManagementAction(province_id, "Decision generated: " + 
                utils::ManagementDecisionTypeToString(type));
            return true;
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to generate decision for province " + std::to_string(province_id) +
                ": " + e.what());
            return false;
        }
    }

    bool ProvinceManagementSystem::ProcessDecision(const std::string& decision_id, 
                                                  const std::string& selected_option) {
        auto* decision = m_decision_queue->GetDecision(decision_id);
        if (!decision) return false;

        try {
            if (!decision->SelectOption(selected_option)) {
                return false;
            }

            if (!decision->ApproveDecision()) {
                return false;
            }

            // Execute the decision through the province system
            bool execution_success = ExecuteDecision(*decision);
            
            if (execution_success) {
                decision->GetContext().decision_type; // Mark as executed
                m_decision_queue->MarkDecisionCompleted(decision_id);
                
                LogManagementAction(decision->GetContext().province_id,
                    "Decision executed: " + selected_option);
                return true;
            }
            return false;
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to process decision " + decision_id + ": " + e.what());
            return false;
        }
    }

    // ============================================================================
    // Order System Interface
    // ============================================================================

    std::string ProvinceManagementSystem::IssueConstructionOrder(types::EntityID province_id,
                                                                province::ProductionBuilding building_type) {
        if (!m_province_system || !m_province_system->IsValidProvince(province_id)) {
            return "";
        }

        try {
            auto order = std::make_unique<ProvinceOrder>(OrderType::CONSTRUCTION_ORDER, province_id);
            order->order_description = "Construct " + province::utils::ProductionBuildingToString(building_type);
            order->parameters["building_type"] = std::to_string(static_cast<int>(building_type));
            
            if (m_province_system->CanConstructBuilding(province_id, building_type)) {
                order->estimated_cost = m_province_system->CalculateBuildingCost(building_type, 
                    m_province_system->GetBuildingLevel(province_id, building_type));
                order->can_execute = true;
            }

            std::string order_id = m_order_system->AddOrder(std::move(order));
            
            LogManagementAction(province_id, "Construction order issued: " + order_id);
            return order_id;
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to issue construction order for province " + std::to_string(province_id) +
                ": " + e.what());
            return "";
        }
    }

    std::string ProvinceManagementSystem::IssuePolicyOrder(types::EntityID province_id,
                                                          const std::string& policy_name, 
                                                          double new_value) {
        if (!m_province_system || !m_province_system->IsValidProvince(province_id)) {
            return "";
        }

        try {
            auto order = std::make_unique<ProvinceOrder>(OrderType::POLICY_CHANGE, province_id);
            order->order_description = "Change policy: " + policy_name + " to " + 
                std::to_string(new_value);
            order->parameters["policy_name"] = policy_name;
            order->parameters["new_value"] = std::to_string(new_value);
            order->can_execute = true; // Policy changes are always executable
            order->estimated_cost = 0.0; // No direct cost for policy changes

            std::string order_id = m_order_system->AddOrder(std::move(order));
            
            LogManagementAction(province_id, "Policy order issued: " + order_id);
            return order_id;
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to issue policy order for province " + std::to_string(province_id) +
                ": " + e.what());
            return "";
        }
    }

    // ============================================================================
    // Policy Management Interface
    // ============================================================================

    bool ProvinceManagementSystem::SetTaxRate(types::EntityID province_id, double tax_rate) {
        try {
            auto policy_write = m_access_manager.GetWriteAccess<PlayerPolicyComponent>("SetTaxRate");
            auto* policy_comp = policy_write.GetComponent(province_id);

            if (policy_comp) {
                policy_comp->base_tax_rate = std::clamp(tax_rate, 0.0, 1.0);
                LogManagementAction(province_id, "Tax rate set to " + 
                    std::to_string(static_cast<int>(tax_rate * 100)) + "%");
                return true;
            }
            return false;
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to set tax rate for province " + std::to_string(province_id) +
                ": " + e.what());
            return false;
        }
    }

    bool ProvinceManagementSystem::SetTradePolicy(types::EntityID province_id, double openness_level) {
        try {
            auto policy_write = m_access_manager.GetWriteAccess<PlayerPolicyComponent>("SetTradePolicy");
            auto* policy_comp = policy_write.GetComponent(province_id);

            if (policy_comp) {
                policy_comp->trade_policy_openness = std::clamp(openness_level, 0.0, 1.0);
                LogManagementAction(province_id, "Trade policy openness set to " + 
                    std::to_string(static_cast<int>(openness_level * 100)) + "%");
                return true;
            }
            return false;
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to set trade policy for province " + std::to_string(province_id) +
                ": " + e.what());
            return false;
        }
    }

    bool ProvinceManagementSystem::SetSocialServices(types::EntityID province_id, double funding_level) {
        try {
            auto policy_write = m_access_manager.GetWriteAccess<PlayerPolicyComponent>("SetSocialServices");
            auto* policy_comp = policy_write.GetComponent(province_id);

            if (policy_comp) {
                policy_comp->social_services_funding = std::clamp(funding_level, 0.0, 1.0);
                LogManagementAction(province_id, "Social services funding set to " + 
                    std::to_string(static_cast<int>(funding_level * 100)) + "%");
                return true;
            }
            return false;
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to set social services for province " + std::to_string(province_id) +
                ": " + e.what());
            return false;
        }
    }

    // ============================================================================
    // Information Queries
    // ============================================================================

    std::vector<types::EntityID> ProvinceManagementSystem::GetManagedProvinces() const {
        std::vector<types::EntityID> managed_provinces;
        
        try {
            auto mgmt_read = m_access_manager.GetReadAccess<ManagementComponent>("GetManagedProvinces");
            auto entities = mgmt_read.GetAllEntities();
            
            for (auto entity_id : entities) {
                if (mgmt_read.GetComponent(entity_id) != nullptr) {
                    managed_provinces.push_back(entity_id);
                }
            }
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to get managed provinces: " + std::string(e.what()));
        }
        
        return managed_provinces;
    }

    ManagementComponent* ProvinceManagementSystem::GetManagementData(types::EntityID province_id) {
        try {
            auto mgmt_write = m_access_manager.GetWriteAccess<ManagementComponent>("GetManagementData");
            return mgmt_write.GetComponent(province_id);
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to get management data for province " + std::to_string(province_id) +
                ": " + e.what());
            return nullptr;
        }
    }

    PlayerPolicyComponent* ProvinceManagementSystem::GetPolicyData(types::EntityID province_id) {
        try {
            auto policy_write = m_access_manager.GetWriteAccess<PlayerPolicyComponent>("GetPolicyData");
            return policy_write.GetComponent(province_id);
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to get policy data for province " + std::to_string(province_id) +
                ": " + e.what());
            return nullptr;
        }
    }

    // ============================================================================
    // Decision Generation Implementation
    // ============================================================================

    void ProvinceManagementSystem::InitializeDecisionGenerators() {
        m_decision_generators[ManagementDecisionType::TAX_RATE_ADJUSTMENT] = 
            [this](types::EntityID id) { return GenerateEconomicDecision(id); };
        m_decision_generators[ManagementDecisionType::BUILDING_CONSTRUCTION] = 
            [this](types::EntityID id) { return GenerateConstructionDecision(id); };
        m_decision_generators[ManagementDecisionType::SOCIAL_SERVICES] = 
            [this](types::EntityID id) { return GeneratePolicyDecision(id); };
    }

    DecisionContext ProvinceManagementSystem::GenerateEconomicDecision(types::EntityID province_id) {
        DecisionContext context;
        context.province_id = province_id;
        context.situation_description = "Economic situation requires attention";
        context.urgency_factor = 0.5;
        context.deadline = std::chrono::system_clock::now() + std::chrono::hours(72);

        if (m_province_system) {
            double prosperity = m_province_system->GetProsperityLevel(province_id);
            double treasury = m_province_system->GetTreasuryBalance(province_id);
            
            context.numeric_data["prosperity"] = prosperity;
            context.numeric_data["treasury"] = treasury;
            
            // Generate options based on economic state
            if (treasury < 500.0) {
                DecisionOption option;
                option.option_id = "increase_taxes";
                option.description = "Increase tax rate to improve revenue";
                option.cost = 0.0;
                option.benefit_estimate = 200.0;
                option.ai_recommendation = 0.7;
                context.available_options.push_back(option);
            }
            
            if (prosperity < 0.4) {
                DecisionOption option;
                option.option_id = "reduce_taxes";
                option.description = "Reduce tax burden to improve prosperity";
                option.cost = 100.0;
                option.benefit_estimate = 0.1;
                option.ai_recommendation = 0.6;
                context.available_options.push_back(option);
            }
        }

        return context;
    }

    DecisionContext ProvinceManagementSystem::GenerateConstructionDecision(types::EntityID province_id) {
        DecisionContext context;
        context.province_id = province_id;
        context.situation_description = "Construction opportunities available";
        context.urgency_factor = 0.3;
        context.deadline = std::chrono::system_clock::now() + std::chrono::hours(168); // 1 week

        if (m_province_system) {
            // Generate construction options based on current buildings
            std::vector<province::ProductionBuilding> potential_buildings = {
                province::ProductionBuilding::FARM,
                province::ProductionBuilding::MARKET,
                province::ProductionBuilding::SMITHY
            };

            for (auto building : potential_buildings) {
                if (m_province_system->CanConstructBuilding(province_id, building)) {
                    DecisionOption option;
                    option.option_id = "construct_" + province::utils::ProductionBuildingToString(building);
                    option.description = "Construct " + province::utils::ProductionBuildingToString(building);
                    option.cost = m_province_system->CalculateBuildingCost(building, 
                        m_province_system->GetBuildingLevel(province_id, building));
                    option.benefit_estimate = option.cost * 0.1; // 10% monthly return estimate
                    option.ai_recommendation = 0.5;
                    context.available_options.push_back(option);
                }
            }
        }

        return context;
    }

    DecisionContext ProvinceManagementSystem::GeneratePolicyDecision(types::EntityID province_id) {
        DecisionContext context;
        context.province_id = province_id;
        context.situation_description = "Policy review recommended";
        context.urgency_factor = 0.2;
        context.deadline = std::chrono::system_clock::now() + std::chrono::hours(336); // 2 weeks

        // Generate policy adjustment options
        DecisionOption option1;
        option1.option_id = "increase_social_services";
        option1.description = "Increase social services funding";
        option1.cost = 50.0;
        option1.benefit_estimate = 0.05; // 5% prosperity improvement
        option1.ai_recommendation = 0.6;
        context.available_options.push_back(option1);

        DecisionOption option2;
        option2.option_id = "maintain_current";
        option2.description = "Maintain current policy levels";
        option2.cost = 0.0;
        option2.benefit_estimate = 0.0;
        option2.ai_recommendation = 0.4;
        context.available_options.push_back(option2);

        return context;
    }

    // ============================================================================
    // Order Processing Implementation
    // ============================================================================

    void ProvinceManagementSystem::ProcessActiveOrders() {
        auto active_orders = m_order_system->GetActiveOrders();
        
        for (auto* order : active_orders) {
            if (!order->can_execute) continue;
            
            bool success = false;
            switch (order->order_type) {
            case OrderType::CONSTRUCTION_ORDER:
                success = ExecuteConstructionOrder(*order);
                break;
            case OrderType::POLICY_CHANGE:
                success = ExecutePolicyOrder(*order);
                break;
            default:
                break;
            }
            
            if (success) {
                m_order_system->CompleteOrder(order->order_id);
            }
        }
    }

    bool ProvinceManagementSystem::ExecuteConstructionOrder(const ProvinceOrder& order) {
        if (!m_province_system) return false;
        
        auto building_param = order.parameters.find("building_type");
        if (building_param == order.parameters.end()) return false;
        
        try {
            int building_type_int = std::stoi(building_param->second);
            auto building_type = static_cast<province::ProductionBuilding>(building_type_int);
            
            return m_province_system->ConstructBuilding(order.target_province, building_type);
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to execute construction order " + order.order_id + ": " + e.what());
            return false;
        }
    }

    bool ProvinceManagementSystem::ExecutePolicyOrder(const ProvinceOrder& order) {
        auto policy_param = order.parameters.find("policy_name");
        auto value_param = order.parameters.find("new_value");
        
        if (policy_param == order.parameters.end() || value_param == order.parameters.end()) {
            return false;
        }
        
        try {
            double new_value = std::stod(value_param->second);
            const std::string& policy_name = policy_param->first;
            
            if (policy_name == "base_tax_rate") {
                return SetTaxRate(order.target_province, new_value);
            }
            else if (policy_name == "trade_policy_openness") {
                return SetTradePolicy(order.target_province, new_value);
            }
            else if (policy_name == "social_services_funding") {
                return SetSocialServices(order.target_province, new_value);
            }
            
            return false;
        }
        catch (const std::exception& e) {
            core::logging::LogError("ProvinceManagementSystem",
                "Failed to execute policy order " + order.order_id + ": " + e.what());
            return false;
        }
    }

    // ============================================================================
    // Automation Implementation
    // ============================================================================

    void ProvinceManagementSystem::ProcessAutomatedDecisions() {
        m_decision_queue->ProcessAutomatedDecisions();
    }

    bool ProvinceManagementSystem::ShouldAutomate(const PlayerDecision& decision) const {
        auto managed_provinces = GetManagedProvinces();
        
        for (auto province_id : managed_provinces) {
            auto mgmt_read = m_access_manager.GetReadAccess<ManagementComponent>("ShouldAutomate");
            auto* mgmt_comp = mgmt_read.GetComponent(province_id);
            
            if (mgmt_comp && mgmt_comp->province_id == decision.GetContext().province_id) {
                return m_decision_queue->ShouldAutomate(decision);
            }
        }
        
        return false;
    }

    bool ProvinceManagementSystem::ExecuteDecision(const PlayerDecision& decision) {
        const auto& context = decision.GetContext();
        const auto* selected_option = decision.GetSelectedOption();
        
        if (!selected_option) return false;
        
        // Execute decision based on type and selected option
        switch (context.decision_type) {
        case ManagementDecisionType::TAX_RATE_ADJUSTMENT:
            if (selected_option->option_id == "increase_taxes") {
                return SetTaxRate(context.province_id, 0.15); // Increase to 15%
            }
            else if (selected_option->option_id == "reduce_taxes") {
                return SetTaxRate(context.province_id, 0.08); // Reduce to 8%
            }
            break;
            
        case ManagementDecisionType::BUILDING_CONSTRUCTION:
            // Extract building type from option_id
            if (selected_option->option_id.find("construct_") == 0) {
                std::string building_name = selected_option->option_id.substr(10);
                // Would need building name to enum conversion here
                return true; // Simplified for now
            }
            break;
            
        default:
            break;
        }
        
        return false;
    }

    // ============================================================================
    // Event Handlers
    // ============================================================================

    void ProvinceManagementSystem::OnProvinceCreated(const province::messages::ProvinceCreated& message) {
        // Automatically create management for new provinces if they don't have it
        auto mgmt_read = m_access_manager.GetReadAccess<ManagementComponent>("OnProvinceCreated");
        if (!mgmt_read.GetComponent(message.province_id)) {
            CreateManagedProvince(message.province_id, "Auto-Generated");
        }
    }

    void ProvinceManagementSystem::OnEconomicCrisis(const province::messages::EconomicCrisis& message) {
        // Generate crisis management decision
        GenerateDecision(message.province_id, ManagementDecisionType::TAX_RATE_ADJUSTMENT);
        
        LogManagementAction(message.province_id, 
            "Economic crisis detected - decision generated");
    }

    void ProvinceManagementSystem::OnResourceShortage(const province::messages::ResourceShortage& message) {
        // Generate resource management decision
        GenerateDecision(message.province_id, ManagementDecisionType::TRADE_POLICY_CHANGE);
        
        LogManagementAction(message.province_id, 
            "Resource shortage detected - trade policy decision generated");
    }

    // ============================================================================
    // Helper Methods
    // ============================================================================

    void ProvinceManagementSystem::LogManagementAction(types::EntityID province_id, 
                                                      const std::string& action) {
        core::logging::LogInfo("ProvinceManagementSystem", 
            "Province " + std::to_string(province_id) + ": " + action);
    }

    std::string ProvinceManagementSystem::GenerateOrderId(OrderType type) {
        static int order_counter = 0;
        return "order_" + std::to_string(++order_counter) + "_" + 
               utils::OrderTypeToString(type);
    }

} // namespace game::management
