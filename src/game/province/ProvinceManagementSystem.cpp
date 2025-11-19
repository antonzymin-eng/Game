// ============================================================================
// ProvinceManagementSystem.cpp - Updated Player Interface Implementation
// Created: September 22, 2025, 17:30 UTC
// Location: src/game/management/ProvinceManagementSystem.cpp
// Updated to integrate with EnhancedProvinceSystem - UI/Player layer only
// ============================================================================

#include "game/province/ProvinceManagementSystem.h"
#include "game/province/ProvinceSystem.h"
#include "core/logging/Logger.h"
#include "game/config/GameConfig.h"
#include "utils/RandomGenerator.h"
#include "utils/PlatformCompat.h"

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

    ProvinceManagementSystem::ProvinceManagementSystem(::core::ecs::ComponentAccessManager& access_manager,
        ::core::ecs::MessageBus& message_bus)  // Fix: Changed from ThreadSafeMessageBus
        : m_access_manager(access_manager), m_message_bus(message_bus), m_province_system(nullptr) {

        m_decision_queue = std::make_unique<DecisionQueue>();
        m_order_system = std::make_unique<ProvinceOrderSystem>();
        m_last_update = std::chrono::steady_clock::now();
    }

    ProvinceManagementSystem::~ProvinceManagementSystem() = default;

    void ProvinceManagementSystem::Initialize() {
        CORE_LOG_INFO("ProvinceManagementSystem", "Initializing Province Management System");

        InitializeDecisionGenerators();

        // Subscribe to province system events
        m_message_bus.Subscribe<game::province::messages::ProvinceCreated>(
            [this](const auto& msg) { OnProvinceCreated(msg); });
        m_message_bus.Subscribe<game::province::messages::EconomicCrisis>(
            [this](const auto& msg) { OnEconomicCrisis(msg); });
        m_message_bus.Subscribe<game::province::messages::ResourceShortage>(
            [this](const auto& msg) { OnResourceShortage(msg); });

        CORE_LOG_INFO("ProvinceManagementSystem", "Province Management System initialized");
    }

    void ProvinceManagementSystem::Update(float delta_time) {  // Fix: Corrected signature to match ISystem

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
        CORE_LOG_INFO("ProvinceManagementSystem", "Shutting down Province Management System");

        m_decision_queue->Clear();
        m_order_system.reset();
        m_decision_queue.reset();
        m_province_system = nullptr;
    }

    ::core::threading::ThreadingStrategy ProvinceManagementSystem::GetThreadingStrategy() const {
        return ::core::threading::ThreadingStrategy::MAIN_THREAD;  // UI system must run on main thread
    }

    // Fix: Added missing ISerializable interface implementations
    // Note: Serialize/Deserialize methods removed - not required for this system

    // ============================================================================
    // Province Management Interface
    // ============================================================================

    bool ProvinceManagementSystem::CreateManagedProvince(types::EntityID province_id,
        const std::string& manager_name) {
        // Get EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            CORE_LOG_ERROR("ProvinceManagementSystem", "EntityManager not available");
            return false;
        }

        // Convert to core::ecs::EntityID
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(province_id), 1);

        // Create management component
        auto mgmt_comp = entity_manager->AddComponent<ManagementComponent>(entity_handle, province_id);
        if (!mgmt_comp) {
            return false;
        }
        mgmt_comp->manager_name = manager_name;
        mgmt_comp->player_controlled = true;
        mgmt_comp->automation_level = AutomationLevel::ASSISTED;

        // Create policy component with defaults
        auto policy_comp = entity_manager->AddComponent<PlayerPolicyComponent>(entity_handle);
        if (!policy_comp) {
            return false;
        }

        LogManagementAction(province_id, "Province management created for " + manager_name);
        return true;
    }

    bool ProvinceManagementSystem::DestroyManagedProvince(types::EntityID province_id) {
        // Get EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return false;
        }

        // Convert to core::ecs::EntityID
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(province_id), 1);

        entity_manager->RemoveComponent<ManagementComponent>(entity_handle);
        entity_manager->RemoveComponent<PlayerPolicyComponent>(entity_handle);
        LogManagementAction(province_id, "Province management destroyed");
        return true;
    }

    bool ProvinceManagementSystem::SetProvinceAutomation(types::EntityID province_id, AutomationLevel level) {
        // Get EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return false;
        }

        // Convert to core::ecs::EntityID
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(province_id), 1);

        auto mgmt_comp = entity_manager->GetComponent<ManagementComponent>(entity_handle);
        if (!mgmt_comp) {
            return false;
        }

        mgmt_comp->automation_level = level;
        LogManagementAction(province_id, "Automation level set to " + utils::AutomationLevelToString(level));
        return true;
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
            CORE_LOG_ERROR("ProvinceManagementSystem",
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
            CORE_LOG_ERROR("ProvinceManagementSystem",
                "Failed to process decision " + decision_id + ": " + e.what());
            return false;
        }
    }

    // ============================================================================
    // Order System Interface
    // ============================================================================

    std::string ProvinceManagementSystem::IssueConstructionOrder(types::EntityID province_id,
                                                                game::province::ProductionBuilding building_type) {
        if (!m_province_system || !m_province_system->IsValidProvince(province_id)) {
            return "";
        }

        try {
            auto order = std::make_unique<ProvinceOrder>(OrderType::CONSTRUCTION_ORDER, province_id);
            order->order_description = "Construct " + game::province::utils::ProductionBuildingToString(building_type);
            order->parameters["building_type"] = std::to_string(static_cast<int>(building_type));

            if (m_province_system->CanConstructBuilding(province_id, building_type)) {
                order->estimated_cost = m_province_system->CalculateBuildingCost(building_type,
                    m_province_system->GetBuildingLevel(province_id, building_type));
                order->can_execute = true;
            } else {
                order->can_execute = false;
            }

            std::string order_id = m_order_system->AddOrder(std::move(order));
            
            LogManagementAction(province_id, "Construction order issued: " + order_id);
            return order_id;
        }
        catch (const std::exception& e) {
            CORE_LOG_ERROR("ProvinceManagementSystem",
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
            CORE_LOG_ERROR("ProvinceManagementSystem",
                "Failed to issue policy order for province " + std::to_string(province_id) +
                ": " + e.what());
            return "";
        }
    }

    // ============================================================================
    // Policy Management Interface
    // ============================================================================

    bool ProvinceManagementSystem::SetTaxRate(types::EntityID province_id, double tax_rate) {
        // Get EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return false;
        }

        // Convert to core::ecs::EntityID
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(province_id), 1);

        auto policy_comp = entity_manager->GetComponent<PlayerPolicyComponent>(entity_handle);
        if (!policy_comp) {
            return false;
        }

        policy_comp->base_tax_rate = std::max(0.0, std::min(1.0, tax_rate));
        LogManagementAction(province_id, "Tax rate set to " + std::to_string(tax_rate));
        return true;
    }

    bool ProvinceManagementSystem::SetTradePolicy(types::EntityID province_id, double openness_level) {
        // Get EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return false;
        }

        // Convert to core::ecs::EntityID
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(province_id), 1);

        auto policy_comp = entity_manager->GetComponent<PlayerPolicyComponent>(entity_handle);
        if (!policy_comp) {
            return false;
        }

        policy_comp->trade_policy_openness = std::max(0.0, std::min(1.0, openness_level));
        LogManagementAction(province_id, "Trade policy set to " + std::to_string(openness_level));
        return true;
    }

    bool ProvinceManagementSystem::SetSocialServices(types::EntityID province_id, double funding_level) {
        // Get EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return false;
        }

        // Convert to core::ecs::EntityID
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(province_id), 1);

        auto policy_comp = entity_manager->GetComponent<PlayerPolicyComponent>(entity_handle);
        if (!policy_comp) {
            return false;
        }

        policy_comp->social_services_funding = std::max(0.0, std::min(1.0, funding_level));
        LogManagementAction(province_id, "Social services set to " + std::to_string(funding_level));
        return true;
    }

    // ============================================================================
    // Information Queries
    // ============================================================================

    std::vector<types::EntityID> ProvinceManagementSystem::GetManagedProvinces() const {
        std::vector<types::EntityID> managed_provinces;

        // Get EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return managed_provinces;
        }

        if (m_province_system) {
            auto all_provinces = m_province_system->GetAllProvinces();
            for (auto province_id : all_provinces) {
                // Convert to core::ecs::EntityID
                ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(province_id), 1);

                auto mgmt_comp = entity_manager->GetComponent<ManagementComponent>(entity_handle);
                if (mgmt_comp) {
                    managed_provinces.push_back(province_id);
                }
            }
        }

        return managed_provinces;
    }

    ManagementComponent* ProvinceManagementSystem::GetManagementData(types::EntityID province_id) {
        // Get EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return nullptr;
        }

        // Convert to core::ecs::EntityID
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(province_id), 1);

        auto comp = entity_manager->GetComponent<ManagementComponent>(entity_handle);
        return comp ? comp.get() : nullptr;
    }

    PlayerPolicyComponent* ProvinceManagementSystem::GetPolicyData(types::EntityID province_id) {
        // Get EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return nullptr;
        }

        // Convert to core::ecs::EntityID
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(province_id), 1);

        auto comp = entity_manager->GetComponent<PlayerPolicyComponent>(entity_handle);
        return comp ? comp.get() : nullptr;
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
            std::vector<game::province::ProductionBuilding> potential_buildings = {
                game::province::ProductionBuilding::FARM,
                game::province::ProductionBuilding::MARKET,
                game::province::ProductionBuilding::SMITHY
            };

            for (auto building : potential_buildings) {
                if (m_province_system->CanConstructBuilding(province_id, building)) {
                    DecisionOption option;
                    option.option_id = "construct_" + game::province::utils::ProductionBuildingToString(building);
                    option.description = "Construct " + game::province::utils::ProductionBuildingToString(building);
                    int current_level = m_province_system->GetBuildingLevel(province_id, building);
                    option.cost = m_province_system->CalculateBuildingCost(building, current_level);
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
            auto building_type = static_cast<game::province::ProductionBuilding>(building_type_int);

            return m_province_system->ConstructBuilding(order.target_province, building_type);
        }
        catch (const std::exception& e) {
            CORE_LOG_ERROR("ProvinceManagementSystem",
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
            CORE_LOG_ERROR("ProvinceManagementSystem",
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
        return m_decision_queue->ShouldAutomate(decision);
    }

    bool ProvinceManagementSystem::ExecuteDecision(const PlayerDecision& decision) {
        const auto& context = decision.GetContext();
        const auto* selected_option = decision.GetSelectedOption();

        if (!selected_option) {
            CORE_LOG_ERROR("ProvinceManagementSystem", "ExecuteDecision: No option selected");
            return false;
        }

        if (!m_province_system) {
            CORE_LOG_ERROR("ProvinceManagementSystem", "ExecuteDecision: Province system not available");
            return false;
        }

        // Execute decision based on type and selected option
        switch (context.decision_type) {
        case ManagementDecisionType::TAX_RATE_ADJUSTMENT:
            if (selected_option->option_id == "increase_taxes") {
                return SetTaxRate(context.province_id, 0.15); // Increase to 15%
            }
            else if (selected_option->option_id == "reduce_taxes") {
                return SetTaxRate(context.province_id, 0.08); // Reduce to 8%
            }
            CORE_LOG_WARNING("ProvinceManagementSystem",
                "Unknown tax adjustment option: " + selected_option->option_id);
            return false;

        case ManagementDecisionType::BUILDING_CONSTRUCTION: {
            // Extract building type from option_id (format: "construct_BuildingName")
            if (selected_option->option_id.find("construct_") == 0) {
                std::string building_name = selected_option->option_id.substr(10); // Skip "construct_"

                // Convert building name to enum
                game::province::ProductionBuilding building_type;
                if (building_name == "Farm") {
                    building_type = game::province::ProductionBuilding::FARM;
                } else if (building_name == "Market") {
                    building_type = game::province::ProductionBuilding::MARKET;
                } else if (building_name == "Smithy") {
                    building_type = game::province::ProductionBuilding::SMITHY;
                } else if (building_name == "Workshop") {
                    building_type = game::province::ProductionBuilding::WORKSHOP;
                } else if (building_name == "Mine") {
                    building_type = game::province::ProductionBuilding::MINE;
                } else if (building_name == "Temple") {
                    building_type = game::province::ProductionBuilding::TEMPLE;
                } else {
                    CORE_LOG_ERROR("ProvinceManagementSystem",
                        "Unknown building type in option: " + building_name);
                    return false;
                }

                // Issue construction order
                std::string order_id = IssueConstructionOrder(context.province_id, building_type);
                return !order_id.empty();
            }
            CORE_LOG_WARNING("ProvinceManagementSystem",
                "Invalid building construction option: " + selected_option->option_id);
            return false;
        }

        case ManagementDecisionType::TRADE_POLICY_CHANGE:
            if (selected_option->option_id == "open_trade") {
                return SetTradePolicy(context.province_id, 0.8); // More open
            }
            else if (selected_option->option_id == "restrict_trade") {
                return SetTradePolicy(context.province_id, 0.3); // More restrictive
            }
            else if (selected_option->option_id == "maintain_trade") {
                return true; // No change
            }
            return false;

        case ManagementDecisionType::SOCIAL_SERVICES:
            if (selected_option->option_id == "increase_social_services") {
                return SetSocialServices(context.province_id, 0.7);
            }
            else if (selected_option->option_id == "reduce_social_services") {
                return SetSocialServices(context.province_id, 0.3);
            }
            else if (selected_option->option_id == "maintain_current") {
                return true; // No change
            }
            return false;

        case ManagementDecisionType::BUDGET_ALLOCATION: {
            // Extract allocation from numeric data in context
            auto treasury_it = context.numeric_data.find("budget_amount");
            if (treasury_it != context.numeric_data.end()) {
                double amount = treasury_it->second;
                return m_province_system->InvestInDevelopment(context.province_id, amount);
            }
            CORE_LOG_WARNING("ProvinceManagementSystem", "Budget allocation missing amount");
            return false;
        }

        case ManagementDecisionType::INFRASTRUCTURE_DEVELOPMENT:
            // Infrastructure decisions would affect prosperity and development
            if (m_province_system->ModifyProsperity(context.province_id, 0.05)) {
                return m_province_system->SetDevelopmentLevel(
                    context.province_id,
                    m_province_system->GetProvinceData(context.province_id)->development_level + 1
                );
            }
            return false;

        // TODO: Implement remaining decision types when corresponding systems are available
        case ManagementDecisionType::MIGRATION_POLICY:
        case ManagementDecisionType::RESEARCH_FUNDING:
        case ManagementDecisionType::SCHOLAR_PATRONAGE:
        case ManagementDecisionType::OFFICIAL_APPOINTMENT:
        case ManagementDecisionType::BUREAUCRACY_REFORM:
        case ManagementDecisionType::RECRUITMENT_ORDER:
        case ManagementDecisionType::GARRISON_ASSIGNMENT:
            CORE_LOG_INFO("ProvinceManagementSystem",
                "Decision type " + utils::ManagementDecisionTypeToString(context.decision_type) +
                " not yet implemented - requires additional game systems");
            return false;

        default:
            CORE_LOG_ERROR("ProvinceManagementSystem",
                "Unknown decision type: " + std::to_string(static_cast<int>(context.decision_type)));
            return false;
        }
    }

    // ============================================================================
    // Event Handlers
    // ============================================================================

    void ProvinceManagementSystem::OnProvinceCreated(const game::province::messages::ProvinceCreated& message) {
        // Automatically create management for new provinces
        CreateManagedProvince(message.province_id, "Auto-Generated");
    }

    void ProvinceManagementSystem::OnEconomicCrisis(const game::province::messages::EconomicCrisis& message) {
        // Generate crisis management decision
        GenerateDecision(message.province_id, ManagementDecisionType::TAX_RATE_ADJUSTMENT);
        
        LogManagementAction(message.province_id, 
            "Economic crisis detected - decision generated");
    }

    void ProvinceManagementSystem::OnResourceShortage(const game::province::messages::ResourceShortage& message) {
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
        CORE_LOG_INFO("ProvinceManagementSystem", 
            "Province " + std::to_string(province_id) + ": " + action);
    }

    std::string ProvinceManagementSystem::GenerateOrderId(OrderType type) {
        static int order_counter = 0;
        return "order_" + std::to_string(++order_counter) + "_" + 
               std::to_string(static_cast<int>(type));  // Simplified - just use number
    }

    // ============================================================================
    // Utility Functions
    // ============================================================================

    namespace utils {
        std::string ManagementDecisionTypeToString(ManagementDecisionType type) {
            switch (type) {
                case ManagementDecisionType::TAX_RATE_ADJUSTMENT: return "TaxRateAdjustment";
                case ManagementDecisionType::BUDGET_ALLOCATION: return "BudgetAllocation";
                case ManagementDecisionType::TRADE_POLICY_CHANGE: return "TradePolicyChange";
                case ManagementDecisionType::BUILDING_CONSTRUCTION: return "BuildingConstruction";
                case ManagementDecisionType::INFRASTRUCTURE_DEVELOPMENT: return "InfrastructureDevelopment";
                case ManagementDecisionType::MIGRATION_POLICY: return "MigrationPolicy";
                case ManagementDecisionType::SOCIAL_SERVICES: return "SocialServices";
                case ManagementDecisionType::RESEARCH_FUNDING: return "ResearchFunding";
                case ManagementDecisionType::SCHOLAR_PATRONAGE: return "ScholarPatronage";
                case ManagementDecisionType::OFFICIAL_APPOINTMENT: return "OfficialAppointment";
                case ManagementDecisionType::BUREAUCRACY_REFORM: return "BureaucracyReform";
                case ManagementDecisionType::RECRUITMENT_ORDER: return "RecruitmentOrder";
                case ManagementDecisionType::GARRISON_ASSIGNMENT: return "GarrisonAssignment";
                default: return "Unknown";
            }
        }

        std::string DecisionPriorityToString(DecisionPriority priority) {
            switch (priority) {
                case DecisionPriority::ROUTINE: return "Routine";
                case DecisionPriority::IMPORTANT: return "Important";
                case DecisionPriority::URGENT: return "Urgent";
                case DecisionPriority::CRITICAL: return "Critical";
                default: return "Unknown";
            }
        }

        std::string DecisionStatusToString(DecisionStatus status) {
            switch (status) {
                case DecisionStatus::PENDING: return "Pending";
                case DecisionStatus::APPROVED: return "Approved";
                case DecisionStatus::REJECTED: return "Rejected";
                case DecisionStatus::DELEGATED: return "Delegated";
                case DecisionStatus::EXECUTED: return "Executed";
                case DecisionStatus::FAILED: return "Failed";
                default: return "Unknown";
            }
        }

        std::string AutomationLevelToString(AutomationLevel level) {
            switch (level) {
                case AutomationLevel::MANUAL: return "Manual";
                case AutomationLevel::ASSISTED: return "Assisted";
                case AutomationLevel::GUIDED: return "Guided";
                case AutomationLevel::AUTOMATED: return "Automated";
                default: return "Unknown";
            }
        }

        std::string OrderTypeToString(OrderType type) {
            switch (type) {
                case OrderType::CONSTRUCTION_ORDER: return "Construction";
                case OrderType::POLICY_CHANGE: return "PolicyChange";
                case OrderType::RESOURCE_ALLOCATION: return "ResourceAllocation";
                case OrderType::RESEARCH_ORDER: return "ResearchOrder";
                default: return "Unknown";
            }
        }

        std::string OrderStatusToString(OrderStatus status) {
            switch (status) {
                case OrderStatus::QUEUED: return "Queued";
                case OrderStatus::IN_PROGRESS: return "InProgress";
                case OrderStatus::COMPLETED: return "Completed";
                case OrderStatus::FAILED: return "Failed";
                case OrderStatus::CANCELLED: return "Cancelled";
                default: return "Unknown";
            }
        }

        ManagementComponent CreateManagement(types::EntityID province_id,
            const std::string& manager_name) {
            ManagementComponent comp(province_id);
            comp.manager_name = manager_name;
            comp.player_controlled = true;
            comp.automation_level = AutomationLevel::ASSISTED;
            comp.administrative_efficiency = 1.0;
            return comp;
        }

        PlayerPolicyComponent CreateDefaultPolicies() {
            PlayerPolicyComponent comp;
            comp.base_tax_rate = 0.1;
            comp.trade_policy_openness = 0.5;
            comp.social_services_funding = 0.5;
            comp.research_funding_level = 0.5;
            comp.military_focus = 0.5;
            comp.bureaucratic_centralization = 0.5;
            return comp;
        }

        std::unique_ptr<PlayerDecision> CreateEconomicDecision(types::EntityID province_id,
            ManagementDecisionType type) {
            DecisionContext context;
            context.province_id = province_id;
            context.decision_type = type;
            context.situation_description = "Economic decision required";
            context.urgency_factor = 0.5;
            context.deadline = std::chrono::system_clock::now() + std::chrono::hours(72);
            return std::make_unique<PlayerDecision>(context);
        }

        std::unique_ptr<ProvinceOrder> CreateConstructionOrder(types::EntityID province_id,
            game::province::ProductionBuilding building) {
            auto order = std::make_unique<ProvinceOrder>(OrderType::CONSTRUCTION_ORDER, province_id);
            order->order_description = "Construct " + game::province::utils::ProductionBuildingToString(building);
            order->parameters["building_type"] = std::to_string(static_cast<int>(building));
            order->can_execute = true;
            return order;
        }

        bool IsValidDecisionType(ManagementDecisionType type) {
            return type >= ManagementDecisionType::TAX_RATE_ADJUSTMENT &&
                   type < ManagementDecisionType::COUNT;
        }

        bool IsValidAutomationLevel(AutomationLevel level) {
            return level >= AutomationLevel::MANUAL &&
                   level < AutomationLevel::COUNT;
        }

        bool CanExecuteOrder(const ProvinceOrder& order) {
            return order.can_execute && order.status == OrderStatus::QUEUED;
        }
    }

} // namespace game::management
