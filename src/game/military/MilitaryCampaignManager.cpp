// ============================================================================
// MilitaryCampaignManager.cpp - Military Campaign Manager Implementation
// Created: November 18, 2025
// ============================================================================

#include "game/military/MilitaryCampaignManager.h"
#include "game/military/MilitaryComponents.h"
#include "map/ProvinceRenderComponent.h"
#include "core/logging/Logger.h"

namespace game::military {

    // ========================================================================
    // Constructor
    // ========================================================================

    MilitaryCampaignManager::MilitaryCampaignManager(::core::ecs::EntityManager& entity_manager)
        : entity_manager_(entity_manager)
    {
    }

    // ========================================================================
    // Initialization
    // ========================================================================

    bool MilitaryCampaignManager::Initialize() {
        CORE_STREAM_INFO("MilitaryCampaign") << "Initializing MilitaryCampaignManager...";

        // Initialize fog of war manager
        fog_of_war_manager_ = std::make_unique<map::FogOfWarManager>();

        // Initialize line of sight calculator
        los_calculator_ = std::make_unique<map::LineOfSightCalculator>();

        // Initialize command delay system
        command_delay_system_ = std::make_unique<CommandDelaySystem>();

        // Initialize news delay system
        news_delay_system_ = std::make_unique<news::NewsDelaySystem>();
        news_delay_system_->Initialize(
            entity_manager_,
            &command_delay_system_->GetCalculator()
        );

        CORE_STREAM_INFO("MilitaryCampaign") << "MilitaryCampaignManager initialized successfully";
        return true;
    }

    // ========================================================================
    // Main Update
    // ========================================================================

    void MilitaryCampaignManager::Update(float delta_time, float current_game_time) {
        // Update army orders
        UpdateArmyOrders(delta_time, current_game_time);

        // Process pending commands
        if (command_delay_enabled_) {
            ProcessPendingCommands(current_game_time);
        }

        // Update news system
        if (news_delay_enabled_) {
            news_delay_system_->Update(current_game_time);
        }

        // Generate automatic reports
        GenerateAutomaticReports(current_game_time);
    }

    // ========================================================================
    // Fog of War Updates
    // ========================================================================

    void MilitaryCampaignManager::UpdateFogOfWar(
        game::types::EntityID player_id,
        float current_game_time
    ) {
        if (!fog_of_war_enabled_ || !fog_of_war_manager_) return;

        // First, transition all visible cells to explored
        fog_of_war_manager_->UpdateVisibility(player_id, current_game_time);

        // Then update visibility based on army positions
        UpdateArmyVisibility(player_id, current_game_time);
    }

    void MilitaryCampaignManager::UpdateArmyVisibility(
        game::types::EntityID player_id,
        float current_game_time
    ) {
        if (!fog_of_war_manager_) return;

        // Get all armies belonging to player
        auto armies = entity_manager_.GetEntitiesWithComponent<ArmyComponent>();

        for (const auto& army_entity : armies) {
            auto army = entity_manager_.GetComponent<ArmyComponent>(army_entity);
            if (!army || !army->is_active) continue;

            // TODO: Check if army belongs to player (need owner tracking)
            // For now, reveal for all armies

            // Get army position
            map::Vector2 army_pos = GetArmyPosition(army_entity.id);

            // Get vision range
            float vision_range = GetArmyVisionRange(army_entity.id);

            // Reveal area around army
            fog_of_war_manager_->RevealArea(
                player_id,
                army_pos.x,
                army_pos.y,
                vision_range,
                current_game_time
            );
        }
    }

    // ========================================================================
    // Order Management
    // ========================================================================

    void MilitaryCampaignManager::IssueOrderToArmy(
        game::types::EntityID army_id,
        const MilitaryOrder& order,
        float current_game_time
    ) {
        auto army = entity_manager_.GetComponent<ArmyComponent>(::core::ecs::EntityID{army_id});
        if (!army) {
            CORE_STREAM_WARN("MilitaryCampaign") << "Cannot issue order to army " << army_id << " (not found)";
            return;
        }

        // Get or create orders component
        auto orders = entity_manager_.GetComponent<MilitaryOrdersComponent>(::core::ecs::EntityID{army_id});
        if (!orders) {
            // Create orders component if it doesn't exist
            orders = entity_manager_.AddComponent<MilitaryOrdersComponent>(::core::ecs::EntityID{army_id});
        }

        // If command delay is enabled, send command with delay
        if (command_delay_enabled_) {
            // Get player position (command origin)
            // TODO: Get actual player position from PlayerLocationComponent
            map::Vector2 player_pos(0.0f, 0.0f); // Placeholder

            // Get army position (command destination)
            map::Vector2 army_pos = GetArmyPosition(army_id);

            // Determine communication type based on order priority
            CommunicationType comm_type = CommunicationType::MESSENGER;
            switch (order.priority) {
                case OrderPriority::URGENT:
                    comm_type = CommunicationType::COURIER;
                    break;
                case OrderPriority::EMERGENCY:
                    comm_type = CommunicationType::SIGNAL_FIRE;
                    break;
                default:
                    comm_type = CommunicationType::MESSENGER;
                    break;
            }

            // Send command with delay
            command_delay_system_->SendCommand(
                army_id,
                0, // order_id (would need to generate unique ID)
                player_pos,
                army_pos,
                comm_type,
                current_game_time,
                std::string(order.GetTypeName())
            );
        }

        // Issue order immediately if delay disabled
        orders->IssueOrder(order);

        CORE_STREAM_INFO("MilitaryCampaign") << "Issued order to army " << army_id
                                             << ": " << order.GetTypeName();
    }

    // ========================================================================
    // Player Location Management
    // ========================================================================

    void MilitaryCampaignManager::SetPlayerLocation(
        game::types::EntityID player_id,
        player::PlayerLocationType location_type,
        game::types::EntityID location_entity,
        const map::Vector2& position,
        float current_game_time
    ) {
        auto location = entity_manager_.GetComponent<player::PlayerLocationComponent>(
            ::core::ecs::EntityID{player_id}
        );

        if (!location) {
            // Create component if it doesn't exist
            location = entity_manager_.AddComponent<player::PlayerLocationComponent>(::core::ecs::EntityID{player_id});
        }

        // Check if leaving capital
        bool was_at_capital = location->IsAtCapital();
        bool going_to_field = (location_type == player::PlayerLocationType::WITH_ARMY ||
                               location_type == player::PlayerLocationType::WITH_NAVY);

        // Update location
        location->location_type = location_type;
        location->current_position = position;

        if (location_type == player::PlayerLocationType::WITH_ARMY) {
            location->current_army = location_entity;
        } else if (location_type == player::PlayerLocationType::WITH_NAVY) {
            location->current_navy = location_entity;
        }

        location->UpdateDistanceFromCapital();

        // Activate regent if player leaves capital for the field
        if (was_at_capital && going_to_field) {
            ActivateRegent(
                player_id,
                player::RegentType::ROYAL_COUNCIL, // Default to royal council
                0, // No specific regent character
                current_game_time
            );
        }
        // Deactivate regent if returning to capital
        else if (!was_at_capital && location_type == player::PlayerLocationType::IN_CAPITAL) {
            DeactivateRegent(player_id, current_game_time);
        }

        CORE_STREAM_INFO("MilitaryCampaign") << "Player " << player_id
                                             << " moved to " << player::PlayerLocationTypeToString(location_type);
    }

    // ========================================================================
    // Regent Management
    // ========================================================================

    void MilitaryCampaignManager::ActivateRegent(
        game::types::EntityID player_id,
        player::RegentType regent_type,
        game::types::EntityID regent_character,
        float current_game_time
    ) {
        if (news_delay_system_) {
            news_delay_system_->ActivateRegent(
                player_id,
                regent_type,
                regent_character,
                current_game_time
            );
        }

        CORE_STREAM_INFO("MilitaryCampaign") << "Activated regent for player " << player_id
                                             << " (type: " << player::RegentTypeToString(regent_type) << ")";
    }

    void MilitaryCampaignManager::DeactivateRegent(
        game::types::EntityID player_id,
        float current_game_time
    ) {
        if (news_delay_system_) {
            news_delay_system_->DeactivateRegent(player_id, current_game_time);
        }

        CORE_STREAM_INFO("MilitaryCampaign") << "Deactivated regent for player " << player_id;
    }

    // ========================================================================
    // News Generation
    // ========================================================================

    void MilitaryCampaignManager::SendBattleReport(
        game::types::EntityID player_id,
        const std::string& battle_name,
        const map::Vector2& battle_location,
        bool victory,
        int casualties,
        float event_time
    ) {
        if (!news_delay_enabled_ || !news_delay_system_) return;

        news::NewsMessage message = news::NewsFactory::CreateBattleReport(
            battle_name,
            battle_location,
            0, // attacker_army
            0, // defender_army
            victory,
            casualties,
            event_time
        );

        news_delay_system_->SendNewsToPlayer(player_id, message, event_time);
    }

    void MilitaryCampaignManager::SendArmyStatusUpdate(
        game::types::EntityID player_id,
        game::types::EntityID army_id,
        const std::string& status_message,
        const map::Vector2& army_position,
        float event_time
    ) {
        if (!news_delay_enabled_ || !news_delay_system_) return;

        news::NewsMessage message;
        message.category = news::NewsCategory::MILITARY;
        message.priority = news::NewsPriority::ROUTINE;
        message.title = "Army Status Update";
        message.content = status_message;
        message.origin_position = army_position;
        message.event_time = event_time;
        message.related_army = army_id;
        message.sender_name = "Field Commander";

        news_delay_system_->SendNewsToPlayer(player_id, message, event_time);
    }

    // ========================================================================
    // Private Helper Methods
    // ========================================================================

    void MilitaryCampaignManager::UpdateArmyOrders(float delta_time, float current_game_time) {
        // Get all entities with orders component
        auto armies = entity_manager_.GetEntitiesWithComponent<MilitaryOrdersComponent>();

        for (const auto& army_entity : armies) {
            auto orders = entity_manager_.GetComponent<MilitaryOrdersComponent>(army_entity);
            if (!orders) continue;

            // Update order execution
            orders->UpdateOrders(delta_time, current_game_time);
        }
    }

    void MilitaryCampaignManager::ProcessPendingCommands(float current_game_time) {
        if (command_delay_system_) {
            command_delay_system_->Update(current_game_time);
        }
    }

    void MilitaryCampaignManager::GenerateAutomaticReports(float current_game_time) {
        // TODO: Implement automatic report generation
        // - Check for significant events (battles, sieges, etc.)
        // - Generate reports for player
        // - Send news messages
    }

    map::Vector2 MilitaryCampaignManager::GetArmyPosition(game::types::EntityID army_id) const {
        auto army = entity_manager_.GetComponent<ArmyComponent>(::core::ecs::EntityID{army_id});
        if (!army) return map::Vector2(0.0f, 0.0f);

        // Get province position
        auto province = entity_manager_.GetComponent<map::ProvinceRenderComponent>(
            ::core::ecs::EntityID{army->current_location}
        );

        if (province) {
            return province->center_position;
        }

        return map::Vector2(0.0f, 0.0f);
    }

    float MilitaryCampaignManager::GetArmyVisionRange(game::types::EntityID army_id) const {
        auto army = entity_manager_.GetComponent<ArmyComponent>(::core::ecs::EntityID{army_id});
        if (!army) return 50.0f; // Default range

        // Calculate based on army composition
        float max_range = 50.0f; // Base infantry range

        for (const auto& unit : army->units) {
            float unit_range = 50.0f;

            switch (unit.unit_class) {
                case UnitClass::CAVALRY:
                    unit_range = 100.0f; // Cavalry has better scouting
                    break;
                case UnitClass::INFANTRY:
                    unit_range = 50.0f;
                    break;
                case UnitClass::SIEGE:
                    unit_range = 40.0f; // Siege has limited vision
                    break;
                default:
                    unit_range = 50.0f;
                    break;
            }

            max_range = std::max(max_range, unit_range);
        }

        return max_range;
    }

} // namespace game::military
