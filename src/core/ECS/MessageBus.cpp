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
        // Check if already processing (prevent recursive processing)
        {
            std::unique_lock lock(m_processing_mutex);
            if (m_processing) {
                CORE_TRACE_MESSAGE_BUS("reentry_guard", "queue", "already processing");
                return;
            }
            m_processing = true;
        }

        CORE_TRACE_MESSAGE_BUS("process_start", "queue", "starting");

        while (true) {
            // Extract message from queue (minimal lock time)
            std::unique_ptr<IMessage> message;
            {
                std::unique_lock lock(m_queue_mutex);
                if (m_message_queue.empty()) {
                    break;
                }
                message = std::move(m_message_queue.front());
                m_message_queue.pop();
            }

            // Process message outside queue lock to allow concurrent publishing
            CORE_TRACE_MESSAGE_BUS("dispatch", message->GetTypeIndex().name(), "queued message");
            PublishImmediate(*message);
        }

        {
            std::unique_lock lock(m_processing_mutex);
            m_processing = false;
        }
        CORE_TRACE_MESSAGE_BUS("process_end", "queue", "completed");
    }

    void MessageBus::Clear() {
        {
            std::unique_lock lock(m_handlers_mutex);
            m_handlers.clear();
        }

        {
            std::unique_lock lock(m_queue_mutex);
            // Clear message queue
            while (!m_message_queue.empty()) {
                m_message_queue.pop();
            }
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
        std::unique_lock lock(m_queue_mutex);
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