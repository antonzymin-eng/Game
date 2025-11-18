// ============================================================================
// MilitaryOrders.h - Military Orders and Command System
// Created: November 18, 2025
// Description: Order system for military units with priority queues,
//              execution tracking, and command hierarchy
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "core/types/game_types.h"
#include "map/ProvinceRenderComponent.h"
#include <vector>
#include <string>
#include <queue>

namespace game::military {

    // ========================================================================
    // OrderType - Types of military orders
    // ========================================================================
    enum class OrderType {
        NONE = 0,           // No orders (idle)
        MOVE,               // Move to location
        ATTACK,             // Attack enemy army
        DEFEND,             // Defend location/province
        SIEGE,              // Siege fortification
        RETREAT,            // Retreat to safety
        PATROL,             // Patrol route
        GARRISON,           // Garrison in province
        RAID,               // Raid enemy territory
        FOLLOW,             // Follow another army
        SUPPORT,            // Support friendly army in battle
        AMBUSH,             // Set up ambush
        SCOUT,              // Scout area
        RESUPPLY,           // Move to resupply point
        DISBAND,            // Disband army
        COUNT
    };

    // ========================================================================
    // OrderPriority - Execution priority levels
    // ========================================================================
    enum class OrderPriority {
        LOW = 0,
        NORMAL,
        HIGH,
        URGENT,
        EMERGENCY,
        COUNT
    };

    // ========================================================================
    // OrderStatus - Order execution status
    // ========================================================================
    enum class OrderStatus {
        PENDING = 0,        // Not yet started
        IN_TRANSIT,         // Command being sent (delay system)
        RECEIVED,           // Command received by unit
        EXECUTING,          // Currently executing
        COMPLETED,          // Successfully completed
        FAILED,             // Failed to execute
        CANCELLED,          // Cancelled by player/commander
        INTERRUPTED,        // Interrupted by events (e.g., attacked)
        COUNT
    };

    // ========================================================================
    // MilitaryOrder - Individual military order
    // ========================================================================
    struct MilitaryOrder {
        OrderType type = OrderType::NONE;
        OrderPriority priority = OrderPriority::NORMAL;
        OrderStatus status = OrderStatus::PENDING;

        // Order targets
        game::types::EntityID target_province = 0;      // Target location
        game::types::EntityID target_army = 0;          // Target army (for attack/follow)
        game::types::EntityID target_fortification = 0; // Target fortification (for siege)
        map::Vector2 target_position;                   // Precise position target

        // Order metadata
        game::types::EntityID issuer_id = 0;           // Who issued the order
        float issue_time = 0.0f;                       // Game time when issued
        float receive_time = 0.0f;                     // Game time when received
        float start_time = 0.0f;                       // Game time when execution started
        float completion_time = 0.0f;                  // Game time when completed

        // Order parameters
        bool allow_engagement = true;                  // Allow combat during movement
        bool force_march = false;                      // Forced march (faster, more attrition)
        float patrol_radius = 0.0f;                    // Patrol area radius
        std::vector<game::types::EntityID> patrol_route; // Patrol waypoints

        // Execution data
        float progress = 0.0f;                         // 0.0 to 1.0
        std::string failure_reason;                    // Why order failed
        std::string notes;                             // Additional notes

        MilitaryOrder() = default;
        MilitaryOrder(OrderType order_type, OrderPriority order_priority = OrderPriority::NORMAL)
            : type(order_type), priority(order_priority) {}

        // Check if order is active (being executed or pending)
        bool IsActive() const {
            return status == OrderStatus::PENDING ||
                   status == OrderStatus::IN_TRANSIT ||
                   status == OrderStatus::RECEIVED ||
                   status == OrderStatus::EXECUTING;
        }

        // Check if order is complete (success or failure)
        bool IsComplete() const {
            return status == OrderStatus::COMPLETED ||
                   status == OrderStatus::FAILED ||
                   status == OrderStatus::CANCELLED;
        }

        // Get order type name
        const char* GetTypeName() const;

        // Get order status name
        const char* GetStatusName() const;
    };

    // ========================================================================
    // OrderQueue - Priority queue for military orders
    // ========================================================================
    class OrderQueue {
    public:
        OrderQueue() = default;

        // Add order to queue
        void AddOrder(const MilitaryOrder& order);

        // Get current order being executed
        MilitaryOrder* GetCurrentOrder();
        const MilitaryOrder* GetCurrentOrder() const;

        // Get next pending order
        MilitaryOrder* GetNextOrder();

        // Remove current order (when completed)
        void CompleteCurrentOrder();

        // Cancel all orders
        void CancelAllOrders();

        // Cancel specific order type
        void CancelOrdersOfType(OrderType type);

        // Get all orders
        const std::vector<MilitaryOrder>& GetAllOrders() const { return orders_; }
        std::vector<MilitaryOrder>& GetAllOrders() { return orders_; }

        // Check if queue is empty
        bool IsEmpty() const { return orders_.empty(); }

        // Get queue size
        size_t GetSize() const { return orders_.size(); }

        // Clear completed orders
        void ClearCompleted();

    private:
        std::vector<MilitaryOrder> orders_;
        size_t current_order_index_ = 0;

        // Sort orders by priority
        void SortByPriority();
    };

    // ========================================================================
    // CommandHierarchy - Chain of command for order propagation
    // ========================================================================
    struct CommandHierarchy {
        game::types::EntityID supreme_commander = 0;   // Top commander
        game::types::EntityID theater_commander = 0;   // Regional commander
        game::types::EntityID field_commander = 0;     // Direct commander

        // Command effectiveness
        float command_efficiency = 1.0f;               // How well orders are executed
        float communication_speed = 1.0f;              // Command delay multiplier

        CommandHierarchy() = default;

        // Check if unit has a commander
        bool HasCommander() const {
            return field_commander != 0 ||
                   theater_commander != 0 ||
                   supreme_commander != 0;
        }

        // Get highest-ranking commander
        game::types::EntityID GetHighestCommander() const {
            if (field_commander != 0) return field_commander;
            if (theater_commander != 0) return theater_commander;
            return supreme_commander;
        }
    };

    // ========================================================================
    // MilitaryOrdersComponent - ECS Component for army orders
    // ========================================================================
    struct MilitaryOrdersComponent : public game::core::Component<MilitaryOrdersComponent> {
        OrderQueue order_queue;
        CommandHierarchy chain_of_command;

        // Order execution state
        bool autonomous_orders = false;        // AI controls orders
        bool accept_new_orders = true;         // Can receive new orders

        // Order history
        std::vector<MilitaryOrder> order_history;
        size_t max_history_size = 100;

        MilitaryOrdersComponent() = default;

        // Issue new order
        void IssueOrder(const MilitaryOrder& order);

        // Update order execution
        void UpdateOrders(float delta_time, float current_game_time);

        // Cancel current order
        void CancelCurrentOrder();

        // Add completed order to history
        void ArchiveOrder(const MilitaryOrder& order);

        std::string GetComponentTypeName() const override {
            return "MilitaryOrdersComponent";
        }
    };

    // ========================================================================
    // Helper Functions
    // ========================================================================

    // Get order type name
    inline const char* OrderTypeToString(OrderType type) {
        switch (type) {
            case OrderType::NONE: return "None";
            case OrderType::MOVE: return "Move";
            case OrderType::ATTACK: return "Attack";
            case OrderType::DEFEND: return "Defend";
            case OrderType::SIEGE: return "Siege";
            case OrderType::RETREAT: return "Retreat";
            case OrderType::PATROL: return "Patrol";
            case OrderType::GARRISON: return "Garrison";
            case OrderType::RAID: return "Raid";
            case OrderType::FOLLOW: return "Follow";
            case OrderType::SUPPORT: return "Support";
            case OrderType::AMBUSH: return "Ambush";
            case OrderType::SCOUT: return "Scout";
            case OrderType::RESUPPLY: return "Resupply";
            case OrderType::DISBAND: return "Disband";
            default: return "Unknown";
        }
    }

    // Get order status name
    inline const char* OrderStatusToString(OrderStatus status) {
        switch (status) {
            case OrderStatus::PENDING: return "Pending";
            case OrderStatus::IN_TRANSIT: return "In Transit";
            case OrderStatus::RECEIVED: return "Received";
            case OrderStatus::EXECUTING: return "Executing";
            case OrderStatus::COMPLETED: return "Completed";
            case OrderStatus::FAILED: return "Failed";
            case OrderStatus::CANCELLED: return "Cancelled";
            case OrderStatus::INTERRUPTED: return "Interrupted";
            default: return "Unknown";
        }
    }

    // Get order priority name
    inline const char* OrderPriorityToString(OrderPriority priority) {
        switch (priority) {
            case OrderPriority::LOW: return "Low";
            case OrderPriority::NORMAL: return "Normal";
            case OrderPriority::HIGH: return "High";
            case OrderPriority::URGENT: return "Urgent";
            case OrderPriority::EMERGENCY: return "Emergency";
            default: return "Unknown";
        }
    }

    inline const char* MilitaryOrder::GetTypeName() const {
        return OrderTypeToString(type);
    }

    inline const char* MilitaryOrder::GetStatusName() const {
        return OrderStatusToString(status);
    }

} // namespace game::military
