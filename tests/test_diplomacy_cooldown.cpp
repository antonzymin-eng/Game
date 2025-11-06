// ============================================================================
// test_diplomacy_cooldown.cpp - Test cooldown tracking system
// Created: 2025-11-05
// ============================================================================

#include "game/diplomacy/DiplomacyComponents.h"
#include "game/config/GameConfig.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace game::diplomacy;
using namespace game::types;

void test_cooldown_basic() {
    std::cout << "Testing basic cooldown functionality..." << std::endl;
    
    DiplomaticState state(EntityID{1});
    
    // Initially no cooldown
    assert(!state.IsActionOnCooldown(DiplomaticAction::DECLARE_WAR));
    assert(state.GetRemainingCooldownDays(DiplomaticAction::DECLARE_WAR) == 0);
    
    // Set cooldown
    state.SetActionCooldown(DiplomaticAction::DECLARE_WAR, 10);
    
    // Should be on cooldown now
    assert(state.IsActionOnCooldown(DiplomaticAction::DECLARE_WAR));
    int days = state.GetRemainingCooldownDays(DiplomaticAction::DECLARE_WAR);
    assert(days > 0 && days <= 10);
    
    std::cout << "  ✓ Cooldown active with " << days << " days remaining" << std::endl;
}

void test_cooldown_multiple_actions() {
    std::cout << "Testing multiple action cooldowns..." << std::endl;
    
    DiplomaticState state(EntityID{2});
    
    // Set cooldowns for different actions
    state.SetActionCooldown(DiplomaticAction::DECLARE_WAR, 365);
    state.SetActionCooldown(DiplomaticAction::PROPOSE_ALLIANCE, 180);
    state.SetActionCooldown(DiplomaticAction::PROPOSE_TRADE, 90);
    
    // All should be on cooldown
    assert(state.IsActionOnCooldown(DiplomaticAction::DECLARE_WAR));
    assert(state.IsActionOnCooldown(DiplomaticAction::PROPOSE_ALLIANCE));
    assert(state.IsActionOnCooldown(DiplomaticAction::PROPOSE_TRADE));
    
    // Different actions should have different cooldowns
    int war_days = state.GetRemainingCooldownDays(DiplomaticAction::DECLARE_WAR);
    int alliance_days = state.GetRemainingCooldownDays(DiplomaticAction::PROPOSE_ALLIANCE);
    int trade_days = state.GetRemainingCooldownDays(DiplomaticAction::PROPOSE_TRADE);
    
    assert(war_days > alliance_days);
    assert(alliance_days > trade_days);
    
    std::cout << "  ✓ War cooldown: " << war_days << " days" << std::endl;
    std::cout << "  ✓ Alliance cooldown: " << alliance_days << " days" << std::endl;
    std::cout << "  ✓ Trade cooldown: " << trade_days << " days" << std::endl;
}

void test_cooldown_expiry() {
    std::cout << "Testing cooldown expiry (fast simulation)..." << std::endl;
    
    DiplomaticState state(EntityID{3});
    
    // Set a very short cooldown for testing (1 day)
    state.SetActionCooldown(DiplomaticAction::SEND_GIFT, 1);
    
    assert(state.IsActionOnCooldown(DiplomaticAction::SEND_GIFT));
    
    // Simulate passage of time by manipulating the cooldown directly
    // In real usage, time naturally passes
    auto& cooldowns = state.action_cooldowns;
    auto now = std::chrono::system_clock::now();
    cooldowns[DiplomaticAction::SEND_GIFT] = now - std::chrono::hours(25); // Set to past
    
    // Should no longer be on cooldown
    assert(!state.IsActionOnCooldown(DiplomaticAction::SEND_GIFT));
    assert(state.GetRemainingCooldownDays(DiplomaticAction::SEND_GIFT) == 0);
    
    std::cout << "  ✓ Cooldown expires correctly" << std::endl;
}

void test_cooldown_config_defaults() {
    std::cout << "Testing config-based default cooldowns..." << std::endl;
    
    auto& config = game::config::GameConfig::Instance();
    
    DiplomaticState state(EntityID{4});
    
    // Set cooldown without specifying days (should use config defaults)
    state.SetActionCooldown(DiplomaticAction::DECLARE_WAR);
    
    int war_days = state.GetRemainingCooldownDays(DiplomaticAction::DECLARE_WAR);
    
    // Should be close to config value (365 days)
    assert(war_days >= 364 && war_days <= 366);
    
    std::cout << "  ✓ Config default applied: " << war_days << " days (expected ~365)" << std::endl;
}

void test_last_major_action_tracking() {
    std::cout << "Testing last major action timestamp..." << std::endl;
    
    DiplomaticState state(EntityID{5});
    
    auto initial_time = state.last_major_action;
    
    // Perform action
    state.SetActionCooldown(DiplomaticAction::DECLARE_WAR);
    
    // last_major_action should be updated
    assert(state.last_major_action > initial_time);
    
    std::cout << "  ✓ Last major action timestamp updated" << std::endl;
}

int main() {
    std::cout << "\n=== Diplomacy Cooldown System Tests ===" << std::endl;
    std::cout << std::endl;
    
    try {
        test_cooldown_basic();
        test_cooldown_multiple_actions();
        test_cooldown_expiry();
        test_cooldown_config_defaults();
        test_last_major_action_tracking();
        
        std::cout << "\n✓ All cooldown tests passed!" << std::endl;
        std::cout << "\nCooldown system features:" << std::endl;
        std::cout << "  • Prevents spam of diplomatic actions" << std::endl;
        std::cout << "  • Configurable per-action cooldowns" << std::endl;
        std::cout << "  • Tracks remaining cooldown time" << std::endl;
        std::cout << "  • Automatic expiry after duration" << std::endl;
        std::cout << "  • Last major action timestamp tracking" << std::endl;
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
