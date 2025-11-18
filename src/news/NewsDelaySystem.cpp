// ============================================================================
// NewsDelaySystem.cpp - News Delay System Implementation
// Created: November 18, 2025
// ============================================================================

#include "game/news/NewsDelaySystem.h"
#include "core/logging/Logger.h"
#include <algorithm>

namespace game::news {

    // ========================================================================
    // NewsDelaySystem Implementation
    // ========================================================================

    NewsDelaySystem::NewsDelaySystem() {
        CORE_STREAM_INFO("NewsDelay") << "NewsDelaySystem initialized";
    }

    void NewsDelaySystem::Initialize(
        ::core::ecs::EntityManager& entity_manager,
        military::CommandDelayCalculator* delay_calculator
    ) {
        entity_manager_ = &entity_manager;
        delay_calculator_ = delay_calculator;

        CORE_STREAM_INFO("NewsDelay") << "NewsDelaySystem: Initialized with entity manager";
    }

    void NewsDelaySystem::SendNewsToPlayer(
        game::types::EntityID player_entity,
        const NewsMessage& message_template,
        float current_game_time
    ) {
        auto* inbox = GetInbox(player_entity);
        if (!inbox) {
            CORE_STREAM_WARN("NewsDelay") << "No inbox component for player " << player_entity;
            return;
        }

        // Create mutable copy
        NewsMessage message = message_template;
        message.send_time = current_game_time;
        message.event_time = message_template.event_time > 0.0f ?
                            message_template.event_time : current_game_time;

        // Calculate delay based on player location and priority
        float delay = CalculateNewsDelay(
            player_entity,
            message.origin_position,
            message.priority,
            current_game_time
        );

        message.total_delay = delay;
        message.arrival_time = current_game_time + delay;

        // Determine communication type based on priority
        message.comm_type = GetCommTypeForPriority(message.priority);

        // Add to pending queue
        inbox->AddPendingMessage(message);

        CORE_STREAM_INFO("NewsDelay") << "Sent news to player " << player_entity
                                      << ": " << message.title
                                      << " (delay: " << delay << " hours)";
    }

    void NewsDelaySystem::Update(float current_game_time) {
        if (!entity_manager_) return;

        // Get all entities with inbox component
        auto players = entity_manager_->GetEntitiesWithComponent<MessageInboxComponent>();

        for (const auto& player_entity : players) {
            auto* inbox = entity_manager_->GetComponent<MessageInboxComponent>(player_entity);
            if (!inbox) continue;

            ProcessArrivedMessages(player_entity.id, inbox, current_game_time);

            // Check if regent should send report
            if (ShouldSendRegentReport(player_entity.id, current_game_time)) {
                auto* regent = GetRegent(player_entity.id);
                if (regent && regent->is_active) {
                    float period_start = regent->last_report_time;
                    float period_end = current_game_time;

                    RegentReport report = GenerateRegentReport(
                        player_entity.id,
                        period_start,
                        period_end
                    );

                    SendRegentReport(player_entity.id, report, current_game_time);
                    regent->last_report_time = current_game_time;
                }
            }
        }
    }

    float NewsDelaySystem::CalculateNewsDelay(
        game::types::EntityID player_entity,
        const map::Vector2& news_origin,
        NewsPriority priority,
        float current_game_time
    ) const {
        auto* location = GetPlayerLocation(player_entity);
        if (!location) {
            return minimum_delay_hours_;
        }

        // If player is at capital, news arrives very quickly (local)
        if (location->IsAtCapital()) {
            return minimum_delay_hours_;
        }

        // Calculate delay based on distance
        float delay = 0.0f;

        if (delay_calculator_) {
            military::CommunicationType comm_type = GetCommTypeForPriority(priority);

            delay = delay_calculator_->CalculateDelay(
                news_origin,
                location->current_position,
                comm_type,
                nullptr,  // No terrain data for now
                1.0f      // Weather modifier
            );
        } else {
            // Fallback: simple distance-based delay
            float distance = location->GetDistanceFrom(news_origin);
            float speed = 20.0f; // Default: 20 km/h

            // Adjust speed by priority
            switch (priority) {
                case NewsPriority::ROUTINE: speed = 15.0f; break;
                case NewsPriority::IMPORTANT: speed = 25.0f; break;
                case NewsPriority::URGENT: speed = 40.0f; break;
                case NewsPriority::CRITICAL: speed = 60.0f; break;
                case NewsPriority::EMERGENCY: speed = 100.0f; break;
                default: speed = 20.0f; break;
            }

            delay = distance / speed; // Hours
        }

        // Apply global multiplier
        delay *= base_delay_multiplier_;

        // Clamp to min/max
        delay = std::max(minimum_delay_hours_, std::min(delay, maximum_delay_hours_));

        return delay;
    }

    bool NewsDelaySystem::ShouldSendRegentReport(
        game::types::EntityID player_entity,
        float current_game_time
    ) const {
        auto* regent = GetRegent(player_entity);
        return regent && regent->ShouldSendReport(current_game_time);
    }

    void NewsDelaySystem::ActivateRegent(
        game::types::EntityID player_entity,
        player::RegentType regent_type,
        game::types::EntityID regent_character,
        float current_game_time
    ) {
        auto* regent = GetRegent(player_entity);
        if (!regent) {
            CORE_STREAM_WARN("NewsDelay") << "No regent component for player " << player_entity;
            return;
        }

        regent->is_active = true;
        regent->regent_type = regent_type;
        regent->regent_character = regent_character;
        regent->start_time = current_game_time;
        regent->last_report_time = current_game_time;

        CORE_STREAM_INFO("NewsDelay") << "Activated regent for player " << player_entity
                                      << " (type: " << player::RegentTypeToString(regent_type) << ")";
    }

    void NewsDelaySystem::DeactivateRegent(
        game::types::EntityID player_entity,
        float current_game_time
    ) {
        auto* regent = GetRegent(player_entity);
        if (!regent) return;

        if (regent->is_active) {
            regent->time_as_regent += (current_game_time - regent->start_time);
            regent->is_active = false;

            CORE_STREAM_INFO("NewsDelay") << "Deactivated regent for player " << player_entity;
        }
    }

    float NewsDelaySystem::GetRegentDecisionQuality(game::types::EntityID player_entity) const {
        auto* regent = GetRegent(player_entity);
        if (!regent || !regent->is_active) {
            return 1.0f; // Player is present, full quality
        }

        return regent->GetOverallCompetency() * regent->decision_quality_modifier;
    }

    RegentReport NewsDelaySystem::GenerateRegentReport(
        game::types::EntityID player_entity,
        float report_start_time,
        float report_end_time
    ) {
        RegentReport report;
        report.report_time = report_end_time;
        report.report_period_start = report_start_time;
        report.report_period_end = report_end_time;

        // TODO: Populate with actual game data
        // For now, create a placeholder report

        report.overall_status = "Stable";
        report.stability_rating = 0.75f;

        report.major_events.push_back("Routine administration continues");
        report.recommendations.push_back("Return to capital at your convenience");

        return report;
    }

    void NewsDelaySystem::SendRegentReport(
        game::types::EntityID player_entity,
        const RegentReport& report,
        float current_game_time
    ) {
        auto* location = GetPlayerLocation(player_entity);
        if (!location) return;

        // Create news message from report
        NewsMessage message = CreateRegentReportMessage(
            report,
            location->capital_position,
            current_game_time
        );

        SendNewsToPlayer(player_entity, message, current_game_time);
    }

    int NewsDelaySystem::GetPendingMessageCount(game::types::EntityID player_entity) const {
        auto* inbox = GetInbox(player_entity);
        return inbox ? static_cast<int>(inbox->pending_messages.size()) : 0;
    }

    int NewsDelaySystem::GetUnreadMessageCount(game::types::EntityID player_entity) const {
        auto* inbox = GetInbox(player_entity);
        return inbox ? inbox->unread_message_count : 0;
    }

    // ========================================================================
    // Private Helper Methods
    // ========================================================================

    player::PlayerLocationComponent* NewsDelaySystem::GetPlayerLocation(
        game::types::EntityID player_entity
    ) {
        if (!entity_manager_) return nullptr;
        return entity_manager_->GetComponent<player::PlayerLocationComponent>(
            ::core::ecs::EntityID{player_entity}
        );
    }

    const player::PlayerLocationComponent* NewsDelaySystem::GetPlayerLocation(
        game::types::EntityID player_entity
    ) const {
        if (!entity_manager_) return nullptr;
        return entity_manager_->GetComponent<player::PlayerLocationComponent>(
            ::core::ecs::EntityID{player_entity}
        );
    }

    player::RegentComponent* NewsDelaySystem::GetRegent(game::types::EntityID player_entity) {
        if (!entity_manager_) return nullptr;
        return entity_manager_->GetComponent<player::RegentComponent>(
            ::core::ecs::EntityID{player_entity}
        );
    }

    const player::RegentComponent* NewsDelaySystem::GetRegent(
        game::types::EntityID player_entity
    ) const {
        if (!entity_manager_) return nullptr;
        return entity_manager_->GetComponent<player::RegentComponent>(
            ::core::ecs::EntityID{player_entity}
        );
    }

    MessageInboxComponent* NewsDelaySystem::GetInbox(game::types::EntityID player_entity) {
        if (!entity_manager_) return nullptr;
        return entity_manager_->GetComponent<MessageInboxComponent>(
            ::core::ecs::EntityID{player_entity}
        );
    }

    const MessageInboxComponent* NewsDelaySystem::GetInbox(
        game::types::EntityID player_entity
    ) const {
        if (!entity_manager_) return nullptr;
        return entity_manager_->GetComponent<MessageInboxComponent>(
            ::core::ecs::EntityID{player_entity}
        );
    }

    void NewsDelaySystem::ProcessArrivedMessages(
        game::types::EntityID player_entity,
        MessageInboxComponent* inbox,
        float current_game_time
    ) {
        if (!inbox) return;

        // Find arrived messages
        std::vector<NewsMessage> arrived;
        auto it = inbox->pending_messages.begin();
        while (it != inbox->pending_messages.end()) {
            if (it->HasArrived(current_game_time)) {
                arrived.push_back(*it);
                it = inbox->pending_messages.erase(it);
            } else {
                ++it;
            }
        }

        // Move to inbox
        for (auto& message : arrived) {
            message.has_arrived = true;
            inbox->MoveToInbox(message);
            inbox->last_message_time = current_game_time;

            CORE_STREAM_INFO("NewsDelay") << "Message arrived for player " << player_entity
                                          << ": " << message.title;
        }
    }

    NewsMessage NewsDelaySystem::CreateRegentReportMessage(
        const RegentReport& report,
        const map::Vector2& capital_position,
        float current_game_time
    ) {
        NewsMessage message;
        message.category = NewsCategory::REGENT_REPORT;
        message.priority = NewsPriority::IMPORTANT;
        message.title = "Report from the Regent";
        message.sender_name = "Royal Council";
        message.origin_position = capital_position;
        message.event_time = current_game_time;

        // Build content from report
        message.content = "Status: " + report.overall_status + "\n";
        message.content += "Treasury: " + std::to_string(static_cast<int>(report.treasury_balance)) + " ducats\n";

        if (!report.major_events.empty()) {
            message.content += "\nMajor Events:\n";
            for (const auto& event : report.major_events) {
                message.content += "- " + event + "\n";
            }
        }

        if (!report.recommendations.empty()) {
            message.content += "\nRecommendations:\n";
            for (const auto& rec : report.recommendations) {
                message.content += "- " + rec + "\n";
            }
        }

        return message;
    }

    // ========================================================================
    // NewsFactory Implementation
    // ========================================================================

    NewsMessage NewsFactory::CreateBattleReport(
        const std::string& battle_name,
        const map::Vector2& battle_location,
        game::types::EntityID attacker_army,
        game::types::EntityID defender_army,
        bool victory,
        int casualties,
        float event_time
    ) {
        NewsMessage message;
        message.category = NewsCategory::MILITARY;
        message.priority = NewsPriority::URGENT;
        message.title = "Battle Report: " + battle_name;
        message.content = victory ? "Victory in battle!" : "Defeat in battle.";
        message.content += "\nCasualties: " + std::to_string(casualties);
        message.origin_position = battle_location;
        message.event_time = event_time;
        message.related_army = attacker_army;
        message.sender_name = "Field Commander";
        return message;
    }

    NewsMessage NewsFactory::CreateEconomicReport(
        game::types::EntityID province,
        const std::string& report_content,
        double income,
        double expenses,
        float event_time
    ) {
        NewsMessage message;
        message.category = NewsCategory::ECONOMIC;
        message.priority = NewsPriority::ROUTINE;
        message.title = "Economic Report";
        message.content = report_content;
        message.content += "\nIncome: " + std::to_string(static_cast<int>(income));
        message.content += "\nExpenses: " + std::to_string(static_cast<int>(expenses));
        message.event_time = event_time;
        message.related_province = province;
        message.sender_name = "Provincial Administrator";
        return message;
    }

    NewsMessage NewsFactory::CreateDiplomaticMessage(
        game::types::EntityID sender_nation,
        const std::string& sender_name,
        const std::string& message_content,
        bool requires_response,
        float event_time
    ) {
        NewsMessage message;
        message.category = NewsCategory::DIPLOMATIC;
        message.priority = requires_response ? NewsPriority::IMPORTANT : NewsPriority::ROUTINE;
        message.title = "Diplomatic Message from " + sender_name;
        message.content = message_content;
        message.event_time = event_time;
        message.related_nation = sender_nation;
        message.sender_name = sender_name;
        message.requires_response = requires_response;
        return message;
    }

    NewsMessage NewsFactory::CreateProvinceEvent(
        game::types::EntityID province,
        const map::Vector2& province_position,
        const std::string& event_description,
        NewsPriority priority,
        float event_time
    ) {
        NewsMessage message;
        message.category = NewsCategory::ADMINISTRATIVE;
        message.priority = priority;
        message.title = "Province Event";
        message.content = event_description;
        message.origin_position = province_position;
        message.event_time = event_time;
        message.related_province = province;
        message.sender_name = "Provincial Governor";
        return message;
    }

    NewsMessage NewsFactory::CreateEmergencyAlert(
        const std::string& alert_title,
        const std::string& alert_content,
        const map::Vector2& location,
        game::types::EntityID related_entity,
        float event_time
    ) {
        NewsMessage message;
        message.category = NewsCategory::EMERGENCY;
        message.priority = NewsPriority::EMERGENCY;
        message.title = alert_title;
        message.content = alert_content;
        message.origin_position = location;
        message.event_time = event_time;
        message.requires_decision = true;
        message.sender_name = "Emergency Alert System";
        return message;
    }

    NewsMessage NewsFactory::CreateTechnologyNews(
        const std::string& technology_name,
        const std::string& description,
        float event_time
    ) {
        NewsMessage message;
        message.category = NewsCategory::TECHNOLOGY;
        message.priority = NewsPriority::IMPORTANT;
        message.title = "Technology Breakthrough: " + technology_name;
        message.content = description;
        message.event_time = event_time;
        message.sender_name = "Royal Academy";
        return message;
    }

    NewsMessage NewsFactory::CreateCharacterNews(
        game::types::EntityID character,
        const std::string& character_name,
        const std::string& event_description,
        NewsPriority priority,
        float event_time
    ) {
        NewsMessage message;
        message.category = NewsCategory::COURT;
        message.priority = priority;
        message.title = "News: " + character_name;
        message.content = event_description;
        message.event_time = event_time;
        message.related_character = character;
        message.sender_name = "Court Herald";
        return message;
    }

    NewsMessage NewsFactory::CreateNavalBattleReport(
        const std::string& battle_name,
        const map::Vector2& battle_location,
        game::types::EntityID attacker_fleet,
        game::types::EntityID defender_fleet,
        bool victory,
        int ships_lost,
        float event_time
    ) {
        NewsMessage message;
        message.category = NewsCategory::NAVAL;
        message.priority = NewsPriority::URGENT;
        message.title = "Naval Battle: " + battle_name;
        message.content = victory ? "Victory at sea!" : "Defeat at sea.";
        message.content += "\nShips lost: " + std::to_string(ships_lost);
        message.origin_position = battle_location;
        message.event_time = event_time;
        message.sender_name = "Admiral";
        return message;
    }

} // namespace game::news
