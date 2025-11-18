# Military Campaign Systems Integration Guide

## Overview

This document describes how to integrate and use the military campaign systems including fog of war, line of sight, military orders, command delays, and news propagation.

## System Architecture

```
MilitaryCampaignManager (Central Coordinator)
├── FogOfWarManager (Visibility tracking)
├── LineOfSightCalculator (LOS calculations)
├── CommandDelaySystem (Order delays)
└── NewsDelaySystem (Message delays)
    └── CommandDelayCalculator (Shared delay calculation)
```

## Quick Start

### 1. Initialize the Campaign Manager

```cpp
#include "game/military/MilitaryCampaignManager.h"

// In your game initialization
core::ecs::EntityManager entity_manager;
auto campaign_manager = std::make_unique<game::military::MilitaryCampaignManager>(entity_manager);
campaign_manager->Initialize();
```

### 2. Set Up Player Location Tracking

```cpp
using namespace game;

// Create player entity with location component
auto player_entity = entity_manager.CreateEntity();
auto location = std::make_shared<player::PlayerLocationComponent>();

// Configure capital
location->capital_province = capital_province_id;
location->capital_position = map::Vector2(500.0f, 300.0f);
location->location_type = player::PlayerLocationType::IN_CAPITAL;
location->current_position = location->capital_position;

entity_manager.AddComponent(player_entity, location);
```

### 3. Initialize Fog of War for Player

```cpp
// Initialize fog of war grid (1000x1000 cells at 1m resolution)
campaign_manager->GetFogOfWarManager()->InitializeForPlayer(
    player_entity.id,  // Player ID
    1000,              // Grid width
    1000,              // Grid height
    1.0f               // Cell size (1 meter)
);
```

### 4. Set Up Message Inbox

```cpp
// Create inbox component for player
auto inbox = std::make_shared<news::MessageInboxComponent>();
inbox->max_inbox_size = 100;
inbox->max_archive_size = 500;
inbox->filter_routine_when_in_field = true;

entity_manager.AddComponent(player_entity, inbox);
```

### 5. Create Regent Component

```cpp
// Create regent component (activated when player leaves capital)
auto regent = std::make_shared<player::RegentComponent>();
regent->regent_type = player::RegentType::ROYAL_COUNCIL;
regent->administrative_skill = 0.7f;
regent->diplomatic_skill = 0.6f;
regent->military_skill = 0.5f;
regent->economic_skill = 0.8f;
regent->authority = 0.7f;
regent->loyalty = 0.9f;
regent->report_frequency = 7.0f; // Report every 7 days

entity_manager.AddComponent(player_entity, regent);
```

## Main Game Loop Integration

```cpp
void GameLoop::Update(float delta_time) {
    float current_game_time = GetCurrentGameTime();

    // Update campaign systems
    campaign_manager->Update(delta_time, current_game_time);

    // Update fog of war for all players
    for (auto& player_id : active_players) {
        campaign_manager->UpdateFogOfWar(player_id, current_game_time);
    }
}
```

## Using the Systems

### Issue Orders to Armies

```cpp
using namespace game::military;

// Create a move order
MilitaryOrder order;
order.type = OrderType::MOVE;
order.priority = OrderPriority::NORMAL;
order.target_province = destination_province_id;
order.target_position = destination_position;
order.allow_engagement = true;
order.issue_time = current_game_time;

// Issue order with automatic delay calculation
campaign_manager->IssueOrderToArmy(
    army_id,
    order,
    current_game_time
);
```

### Move Player to Field with Army

```cpp
// Player joins an army - automatically activates regent
campaign_manager->SetPlayerLocation(
    player_id,
    player::PlayerLocationType::WITH_ARMY,
    army_id,              // Location entity (army ID)
    army_position,        // Current position
    current_game_time
);

// This automatically:
// - Activates the regent at capital
// - Starts sending periodic regent reports
// - Applies news delays based on distance from capital
```

### Send Battle Reports

```cpp
// Send battle report with automatic delay
campaign_manager->SendBattleReport(
    player_id,
    "Battle of Waterloo",
    battle_location,
    true,              // Victory
    1500,              // Casualties
    current_game_time
);

// Report arrives after delay based on:
// - Distance from player to battle location
// - Priority level (battle = URGENT)
// - Terrain between locations
// - Weather conditions
```

### Return Player to Capital

```cpp
// Player returns to capital - deactivates regent
campaign_manager->SetPlayerLocation(
    player_id,
    player::PlayerLocationType::IN_CAPITAL,
    capital_province_id,
    capital_position,
    current_game_time
);

// This automatically:
// - Deactivates the regent
// - Stops regent reports
// - Removes news delays (instant updates at capital)
```

## Rendering Integration

### Integrate Fog of War with MapRenderer

```cpp
// In MapRenderer::Render() at LOD 4 (tactical zoom)
if (current_lod_ == LODLevel::TACTICAL) {
    // ... render terrain ...

    // Render fog of war overlay
    if (fog_of_war_renderer_ && fog_of_war_manager_) {
        auto* visibility_grid = fog_of_war_manager_->GetVisibilityGrid(player_id);
        if (visibility_grid) {
            fog_of_war_renderer_->RenderFogOfWar(
                *visibility_grid,
                camera_,
                draw_list,
                player_id
            );
        }
    }
}
```

### Only Render Visible Units

```cpp
// In UnitRenderer::RenderUnit()
bool UnitRenderer::ShouldRenderUnit(
    const UnitVisual& unit,
    game::types::EntityID player_id,
    map::FogOfWarManager* fow_manager
) {
    if (!fow_manager) return true; // No FOW = show all

    // Check if unit position is visible
    return fow_manager->IsPositionVisible(
        player_id,
        unit.world_position.x,
        unit.world_position.y
    );
}
```

## Advanced Usage

### Custom Communication Technology

```cpp
// Set communication type for a nation (e.g., telegraph available)
auto delay_component = std::make_shared<military::CommandDelayComponent>();
delay_component->available_comm_type = military::CommunicationType::TELEGRAPH;
delay_component->road_network_bonus = 0.3f;        // 30% faster with good roads
delay_component->signal_network_bonus = 0.8f;      // Signal towers installed
delay_component->communication_infrastructure = 0.6f;

entity_manager.AddComponent(nation_entity, delay_component);
```

### Custom News Messages

```cpp
using namespace game::news;

// Create custom diplomatic message
NewsMessage message;
message.category = NewsCategory::DIPLOMATIC;
message.priority = NewsPriority::IMPORTANT;
message.title = "Peace Offer from France";
message.content = "The French ambassador offers terms of peace...";
message.origin_position = paris_position;
message.event_time = current_game_time;
message.requires_response = true;
message.sender_name = "French Ambassador";

// Send with automatic delay
campaign_manager->GetNewsDelaySystem()->SendNewsToPlayer(
    player_id,
    message,
    current_game_time
);
```

### Read Messages from Inbox

```cpp
auto inbox = entity_manager.GetComponent<news::MessageInboxComponent>(player_entity);
if (inbox) {
    // Display unread count
    int unread = inbox->unread_message_count;

    // Get urgent messages
    int urgent_count = inbox->GetUnreadCount(news::NewsPriority::URGENT);

    // Read a message
    if (!inbox->inbox_messages.empty()) {
        auto& message = inbox->inbox_messages[0];

        // Display message to player
        DisplayMessage(message.title, message.content);

        // Mark as read
        inbox->MarkAsRead(0);
    }
}
```

## Configuration Options

### Disable Systems Individually

```cpp
// Disable fog of war (show entire map)
campaign_manager->SetFogOfWarEnabled(false);

// Disable news delays (instant delivery)
campaign_manager->SetNewsDelayEnabled(false);

// Disable command delays (instant orders)
campaign_manager->SetCommandDelayEnabled(false);
```

### Adjust Delay Settings

```cpp
auto* news_system = campaign_manager->GetNewsDelaySystem();

// Make messages arrive faster
news_system->SetBaseDelayMultiplier(0.5f);  // Half normal delay

// Set minimum delay to 5 minutes
news_system->SetMinimumDelay(5.0f / 60.0f);  // 5 minutes in hours

// Set maximum delay to 15 days
news_system->SetMaximumDelay(15.0f * 24.0f); // 15 days in hours
```

### Customize Fog of War Rendering

```cpp
auto* fow_renderer = map_renderer->GetFogOfWarRenderer();

// Use different rendering mode
fow_renderer->SetRenderMode(map::FogRenderMode::GRAYSCALE);

// Adjust opacity
fow_renderer->SetUnexploredOpacity(1.0f);  // Fully opaque black
fow_renderer->SetExploredOpacity(0.4f);    // Semi-transparent grey

// Set fade animation time
fow_renderer->SetFadeTime(0.5f);  // 0.5 second fade
```

## Events and Callbacks

### Monitor Order Completion

```cpp
// Get orders component for army
auto orders = entity_manager.GetComponent<military::MilitaryOrdersComponent>(army_entity);
if (orders) {
    auto* current = orders->order_queue.GetCurrentOrder();
    if (current && current->status == military::OrderStatus::COMPLETED) {
        // Order completed - send notification
        campaign_manager->SendArmyStatusUpdate(
            player_id,
            army_id,
            "Army has reached destination",
            army_position,
            current_game_time
        );
    }
}
```

### React to Regent Reports

```cpp
auto inbox = entity_manager.GetComponent<news::MessageInboxComponent>(player_entity);
if (inbox) {
    // Check for new regent reports
    for (auto& message : inbox->inbox_messages) {
        if (message.category == news::NewsCategory::REGENT_REPORT &&
            !message.has_been_read) {

            // Display regent report to player
            ShowRegentReport(message);

            // Check stability
            if (message.content.find("Crisis") != std::string::npos) {
                // Alert player - country in crisis!
                ShowUrgentAlert("Regent reports crisis at home!");
            }
        }
    }
}
```

## Performance Considerations

### Fog of War Grid Size

- **Small maps** (100x100): Very fast, suitable for tactical battles
- **Medium maps** (500x500): Good performance, suitable for regional campaigns
- **Large maps** (1000x1000): 1 million cells, suitable for strategic campaigns
- **Very large maps** (2000x2000+): May impact performance, consider chunking

### Update Frequency

```cpp
// Update fog of war every frame (real-time)
campaign_manager->UpdateFogOfWar(player_id, current_game_time);

// Or update less frequently for performance (every 0.5 seconds)
static float fow_update_timer = 0.0f;
fow_update_timer += delta_time;

if (fow_update_timer >= 0.5f) {
    campaign_manager->UpdateFogOfWar(player_id, current_game_time);
    fow_update_timer = 0.0f;
}
```

### Message Processing

```cpp
// Limit inbox size to prevent memory issues
inbox->max_inbox_size = 100;
inbox->max_archive_size = 500;

// Periodically clean old archives (e.g., older than 30 days)
inbox->CleanArchive(current_game_time, 30.0f * 24.0f);
```

## Troubleshooting

### Messages Not Arriving

1. Check player has inbox component
2. Verify news delay system is initialized
3. Check news delay is enabled
4. Verify current_game_time is advancing

### Fog of War Not Updating

1. Verify fog of war manager is initialized for player
2. Check fog of war is enabled
3. Ensure UpdateFogOfWar() is called regularly
4. Verify armies have valid positions

### Orders Not Executing

1. Check army has MilitaryOrdersComponent
2. Verify orders are being issued correctly
3. Check command delay system is processing
4. Ensure order queue is not empty

## Example: Complete Campaign Setup

```cpp
void SetupMilitaryCampaign(
    core::ecs::EntityManager& entity_manager,
    game::types::EntityID player_id,
    game::types::EntityID capital_id
) {
    // Create campaign manager
    auto campaign_manager = std::make_unique<military::MilitaryCampaignManager>(entity_manager);
    campaign_manager->Initialize();

    // Set up player location
    auto location = std::make_shared<player::PlayerLocationComponent>();
    location->capital_province = capital_id;
    location->capital_position = GetProvincePosition(capital_id);
    location->location_type = player::PlayerLocationType::IN_CAPITAL;
    location->current_position = location->capital_position;
    entity_manager.AddComponent(player_entity, location);

    // Initialize fog of war
    campaign_manager->GetFogOfWarManager()->InitializeForPlayer(
        player_id, 1000, 1000, 1.0f
    );

    // Set up inbox
    auto inbox = std::make_shared<news::MessageInboxComponent>();
    entity_manager.AddComponent(player_entity, inbox);

    // Set up regent
    auto regent = std::make_shared<player::RegentComponent>();
    regent->regent_type = player::RegentType::CHIEF_MINISTER;
    regent->administrative_skill = 0.8f;
    entity_manager.AddComponent(player_entity, regent);

    // Store campaign manager in game state
    game_state.campaign_manager = std::move(campaign_manager);
}
```

## See Also

- [Fog of War System](FogOfWar.h)
- [Line of Sight System](LineOfSight.h)
- [Military Orders System](MilitaryOrders.h)
- [Command Delay System](CommandDelay.h)
- [News System](NewsSystem.h)
- [Player Location Tracking](PlayerLocation.h)
