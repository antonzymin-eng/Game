// ============================================================================
// MilitaryOrders.cpp - Military Orders System Implementation
// Created: November 18, 2025
// ============================================================================

#include "game/military/MilitaryOrders.h"
#include "core/logging/Logger.h"
#include <algorithm>

namespace game::military {

    // ========================================================================
    // OrderQueue Implementation
    // ========================================================================

    void OrderQueue::AddOrder(const MilitaryOrder& order) {
        orders_.push_back(order);
        SortByPriority();
    }

    MilitaryOrder* OrderQueue::GetCurrentOrder() {
        if (orders_.empty() || current_order_index_ >= orders_.size()) {
            return nullptr;
        }

        // Find first active order
        for (size_t i = current_order_index_; i < orders_.size(); ++i) {
            if (orders_[i].IsActive()) {
                current_order_index_ = i;
                return &orders_[i];
            }
        }

        return nullptr;
    }

    const MilitaryOrder* OrderQueue::GetCurrentOrder() const {
        if (orders_.empty() || current_order_index_ >= orders_.size()) {
            return nullptr;
        }

        for (size_t i = current_order_index_; i < orders_.size(); ++i) {
            if (orders_[i].IsActive()) {
                return &orders_[i];
            }
        }

        return nullptr;
    }

    MilitaryOrder* OrderQueue::GetNextOrder() {
        if (orders_.empty()) return nullptr;

        // Find next pending order
        for (size_t i = current_order_index_ + 1; i < orders_.size(); ++i) {
            if (orders_[i].status == OrderStatus::PENDING) {
                return &orders_[i];
            }
        }

        return nullptr;
    }

    void OrderQueue::CompleteCurrentOrder() {
        if (auto* order = GetCurrentOrder()) {
            order->status = OrderStatus::COMPLETED;
            current_order_index_++;
        }
    }

    void OrderQueue::CancelAllOrders() {
        for (auto& order : orders_) {
            if (order.IsActive()) {
                order.status = OrderStatus::CANCELLED;
            }
        }
        current_order_index_ = 0;
    }

    void OrderQueue::CancelOrdersOfType(OrderType type) {
        for (auto& order : orders_) {
            if (order.type == type && order.IsActive()) {
                order.status = OrderStatus::CANCELLED;
            }
        }
    }

    void OrderQueue::ClearCompleted() {
        orders_.erase(
            std::remove_if(orders_.begin(), orders_.end(),
                [](const MilitaryOrder& order) { return order.IsComplete(); }),
            orders_.end()
        );

        // Reset index if needed
        if (current_order_index_ >= orders_.size()) {
            current_order_index_ = 0;
        }
    }

    void OrderQueue::SortByPriority() {
        // Sort by priority (higher priority first) and then by issue time
        std::sort(orders_.begin(), orders_.end(),
            [](const MilitaryOrder& a, const MilitaryOrder& b) {
                if (a.priority != b.priority) {
                    return static_cast<int>(a.priority) > static_cast<int>(b.priority);
                }
                return a.issue_time < b.issue_time;
            }
        );

        current_order_index_ = 0;
    }

    // ========================================================================
    // MilitaryOrdersComponent Implementation
    // ========================================================================

    void MilitaryOrdersComponent::IssueOrder(const MilitaryOrder& order) {
        if (!accept_new_orders) {
            CORE_STREAM_WARN("MilitaryOrders") << "Cannot accept new orders";
            return;
        }

        order_queue.AddOrder(order);

        CORE_STREAM_INFO("MilitaryOrders") << "Issued order: "
                                           << order.GetTypeName()
                                           << " (priority: " << order.GetStatusName() << ")";
    }

    void MilitaryOrdersComponent::UpdateOrders(float delta_time, float current_game_time) {
        auto* current_order = order_queue.GetCurrentOrder();
        if (!current_order) return;

        // Update order status based on game state
        switch (current_order->status) {
            case OrderStatus::PENDING:
                // Transition to IN_TRANSIT (handled by command delay system)
                current_order->status = OrderStatus::IN_TRANSIT;
                break;

            case OrderStatus::IN_TRANSIT:
                // Check if command has arrived (handled by command delay system)
                // For now, just transition to RECEIVED
                if (current_game_time >= current_order->receive_time) {
                    current_order->status = OrderStatus::RECEIVED;
                }
                break;

            case OrderStatus::RECEIVED:
                // Start executing
                current_order->status = OrderStatus::EXECUTING;
                current_order->start_time = current_game_time;
                break;

            case OrderStatus::EXECUTING:
                // Update progress (implementation depends on order type)
                // This is a placeholder - actual execution logic should be in military system
                current_order->progress += delta_time * 0.01f; // Slow progress

                if (current_order->progress >= 1.0f) {
                    current_order->status = OrderStatus::COMPLETED;
                    current_order->completion_time = current_game_time;
                    ArchiveOrder(*current_order);
                    order_queue.CompleteCurrentOrder();
                }
                break;

            default:
                break;
        }
    }

    void MilitaryOrdersComponent::CancelCurrentOrder() {
        if (auto* order = order_queue.GetCurrentOrder()) {
            order->status = OrderStatus::CANCELLED;
            ArchiveOrder(*order);
            order_queue.CompleteCurrentOrder();
        }
    }

    void MilitaryOrdersComponent::ArchiveOrder(const MilitaryOrder& order) {
        order_history.push_back(order);

        // Limit history size
        if (order_history.size() > max_history_size) {
            order_history.erase(order_history.begin());
        }
    }

} // namespace game::military
