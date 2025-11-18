// ============================================================================
// MessageBus.cpp - Type-safe event communication implementation
// Location: src/core/ECS/MessageBus.cpp
// ============================================================================

#include "core/ECS/MessageBus.h"
#include "core/logging/Logger.h"

#include <string>

namespace core::ecs {

    // ============================================================================
    // MessageBus Implementation (Non-template methods only)
    // ============================================================================

    void MessageBus::ProcessQueuedMessages() {
        // FIXED: Use atomic flag for lock-free check
        bool expected = false;
        if (!m_processing.compare_exchange_strong(expected, true)) {
            CORE_TRACE_MESSAGE_BUS("reentry_guard", "queue", "already processing");
            return;
        }

        CORE_TRACE_MESSAGE_BUS("process_start", "queue", "starting");

        while (true) {
            // Extract message from queue (minimal lock time)
            std::unique_ptr<IMessage> message;
            MessagePriority priority;
            {
                std::unique_lock lock(m_queue_mutex);
                if (m_message_queue.empty()) {
                    break;
                }
                // FIXED: Use priority_queue interface (top/pop instead of front/pop)
                auto& prioritized = const_cast<PrioritizedMessage&>(m_message_queue.top());
                message = std::move(prioritized.message);
                priority = prioritized.priority;
                m_message_queue.pop();
            }

            // Process message outside queue lock to allow concurrent publishing
            const char* priority_str = (priority == MessagePriority::CRITICAL) ? "CRITICAL" :
                                      (priority == MessagePriority::HIGH) ? "HIGH" :
                                      (priority == MessagePriority::NORMAL) ? "NORMAL" : "LOW";
            CORE_TRACE_MESSAGE_BUS("dispatch", message->GetTypeIndex().name(),
                                   std::string("priority=") + priority_str);
            PublishImmediate(*message);
        }

        m_processing.store(false);
        CORE_TRACE_MESSAGE_BUS("process_end", "queue", "completed");
    }

    void MessageBus::Clear() {
        {
            std::unique_lock lock(m_handlers_mutex);
            m_handlers.clear();
        }

        {
            std::unique_lock lock(m_queue_mutex);
            // Clear priority queue by creating a new empty one
            m_message_queue = std::priority_queue<PrioritizedMessage>();
        }
    }

    size_t MessageBus::GetHandlerCount() const {
        std::shared_lock lock(m_handlers_mutex);
        size_t total = 0;
        for (const auto& [type_index, handlers] : m_handlers) {
            total += handlers.size();
        }
        return total;
    }

    size_t MessageBus::GetQueuedMessageCount() const {
        std::lock_guard lock(m_queue_mutex);
        return m_message_queue.size();
    }

    void MessageBus::PublishImmediate(const IMessage& message) {
        auto type_index = message.GetTypeIndex();

        // Use shared lock for reading handlers (allows concurrent reads)
        std::shared_lock lock(m_handlers_mutex);
        auto it = m_handlers.find(type_index);

        if (it != m_handlers.end()) {
            CORE_TRACE_MESSAGE_BUS("publish_immediate", message.GetTypeIndex().name(),
                                   "handlers=" + std::to_string(it->second.size()));
            for (const auto& handler : it->second) {
                handler->HandleMessage(message);
            }
        }
    }

} // namespace core::ecs