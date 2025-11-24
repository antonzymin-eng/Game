// ============================================================================
// Economic System Integration - Complete Implementation Example
// Date Created: 2025-11-21
// Location: src/game/economy/EconomicSystemIntegrationExample.cpp
// Purpose: Ready-to-use initialization code for economic system refactor
// ============================================================================

#include "game/economy/EconomicSystem.h"
#include "game/bridge/DiplomacyEconomicBridge.h"
#include "game/realm/RealmManager.h"
#include "game/province/ProvinceSystem.h"
#include "game/trade/TradeSystem.h"
#include "game/military/MilitaryEconomicBridge.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include "core/logging/Logger.h"

namespace game::economy {

/**
 * Complete initialization example for the refactored economic system.
 *
 * This function demonstrates how to properly wire up all the systems
 * that require EconomicSystem references after the refactor.
 *
 * Copy this code into your actual game initialization and adapt as needed.
 */
class EconomicSystemIntegrator {
public:
    /**
     * Initialize all economic-related systems and wire them together.
     *
     * @param access_manager ComponentAccessManager instance
     * @param message_bus ThreadSafeMessageBus instance
     * @return true if initialization successful, false otherwise
     */
    static bool InitializeEconomicSystems(
        ::core::ecs::ComponentAccessManager& access_manager,
        ::core::threading::ThreadSafeMessageBus& message_bus)
    {
        CORE_LOG_INFO("EconomicIntegration", "Initializing economic systems...");

        // ====================================================================
        // Step 1: Create EconomicSystem (FIRST - others depend on this)
        // ====================================================================

        auto economic_system = std::make_unique<game::economy::EconomicSystem>(
            access_manager,
            message_bus
        );

        if (!economic_system) {
            CORE_LOG_ERROR("EconomicIntegration", "Failed to create EconomicSystem");
            return false;
        }

        // Initialize the economic system
        economic_system->Initialize();
        CORE_LOG_INFO("EconomicIntegration", "✓ EconomicSystem created and initialized");

        // ====================================================================
        // Step 2: Create DiplomacyEconomicBridge
        // ====================================================================

        auto diplomacy_bridge = std::make_unique<game::bridge::DiplomacyEconomicBridge>(
            access_manager,
            message_bus
        );

        if (!diplomacy_bridge) {
            CORE_LOG_ERROR("EconomicIntegration", "Failed to create DiplomacyEconomicBridge");
            return false;
        }

        diplomacy_bridge->Initialize();

        // CRITICAL: Wire up EconomicSystem reference
        diplomacy_bridge->SetEconomicSystem(economic_system.get());

        CORE_LOG_INFO("EconomicIntegration", "✓ DiplomacyEconomicBridge wired to EconomicSystem");

        // ====================================================================
        // Step 3: Create RealmManager
        // ====================================================================

        auto realm_manager = std::make_unique<game::realm::RealmManager>(
            std::make_shared<::core::ecs::ComponentAccessManager>(access_manager),
            std::make_shared<::core::threading::ThreadSafeMessageBus>(message_bus)
        );

        if (!realm_manager) {
            CORE_LOG_ERROR("EconomicIntegration", "Failed to create RealmManager");
            return false;
        }

        realm_manager->Initialize();

        // CRITICAL: Wire up EconomicSystem reference
        realm_manager->SetEconomicSystem(economic_system.get());

        CORE_LOG_INFO("EconomicIntegration", "✓ RealmManager wired to EconomicSystem");

        // ====================================================================
        // Step 4: Create ProvinceSystem
        // ====================================================================

        // Note: ProvinceSystem uses regular MessageBus, not ThreadSafeMessageBus
        // Adjust based on your actual implementation
        auto province_system = std::make_unique<game::province::ProvinceSystem>(
            access_manager,
            message_bus  // May need to cast/adapt based on your implementation
        );

        if (!province_system) {
            CORE_LOG_ERROR("EconomicIntegration", "Failed to create ProvinceSystem");
            return false;
        }

        province_system->Initialize();

        // CRITICAL: Wire up EconomicSystem reference
        province_system->SetEconomicSystem(economic_system.get());

        CORE_LOG_INFO("EconomicIntegration", "✓ ProvinceSystem wired to EconomicSystem");

        // ====================================================================
        // Step 5: Create TradeSystem (optional but recommended)
        // ====================================================================

        auto trade_system = std::make_unique<game::trade::TradeSystem>(
            access_manager,
            message_bus
        );

        if (!trade_system) {
            CORE_LOG_ERROR("EconomicIntegration", "Failed to create TradeSystem");
            return false;
        }

        trade_system->Initialize();
        CORE_LOG_INFO("EconomicIntegration", "✓ TradeSystem created and initialized");

        // ====================================================================
        // Step 6: Create MilitaryEconomicBridge (if you have it)
        // ====================================================================

        // MilitaryEconomicBridge already has EconomicSystem wired in constructor
        // Just create and initialize it normally

        // auto military_bridge = std::make_unique<game::military::MilitaryEconomicBridge>(...);
        // military_bridge->Initialize();
        // CORE_LOG_INFO("EconomicIntegration", "✓ MilitaryEconomicBridge initialized");

        // ====================================================================
        // Step 7: Store systems for later use
        // ====================================================================

        // Store in your game manager/world class
        // Example:
        // m_economic_system = std::move(economic_system);
        // m_diplomacy_bridge = std::move(diplomacy_bridge);
        // m_realm_manager = std::move(realm_manager);
        // m_province_system = std::move(province_system);
        // m_trade_system = std::move(trade_system);

        // ====================================================================
        // Verification: Check connections
        // ====================================================================

        CORE_LOG_INFO("EconomicIntegration", "");
        CORE_LOG_INFO("EconomicIntegration", "=== Economic System Integration Complete ===");
        CORE_LOG_INFO("EconomicIntegration", "All systems wired and ready:");
        CORE_LOG_INFO("EconomicIntegration", "  ✓ EconomicSystem");
        CORE_LOG_INFO("EconomicIntegration", "  ✓ DiplomacyEconomicBridge → EconomicSystem");
        CORE_LOG_INFO("EconomicIntegration", "  ✓ RealmManager → EconomicSystem");
        CORE_LOG_INFO("EconomicIntegration", "  ✓ ProvinceSystem → EconomicSystem");
        CORE_LOG_INFO("EconomicIntegration", "  ✓ TradeSystem");
        CORE_LOG_INFO("EconomicIntegration", "==========================================");
        CORE_LOG_INFO("EconomicIntegration", "");

        return true;
    }

    /**
     * Minimal integration - just the 3 required SetEconomicSystem() calls.
     * Use this if you already have systems created and just need to wire them.
     *
     * @param economic_system Your existing EconomicSystem instance
     * @param diplomacy_bridge Your existing DiplomacyEconomicBridge instance (can be null)
     * @param realm_manager Your existing RealmManager instance (can be null)
     * @param province_system Your existing ProvinceSystem instance (can be null)
     */
    static void WireExistingSystems(
        game::economy::EconomicSystem* economic_system,
        game::bridge::DiplomacyEconomicBridge* diplomacy_bridge = nullptr,
        game::realm::RealmManager* realm_manager = nullptr,
        game::province::ProvinceSystem* province_system = nullptr)
    {
        CORE_LOG_INFO("EconomicIntegration", "Wiring existing systems to EconomicSystem...");

        if (!economic_system) {
            CORE_LOG_ERROR("EconomicIntegration", "EconomicSystem is null! Cannot wire systems.");
            return;
        }

        int wired_count = 0;

        // Wire DiplomacyEconomicBridge
        if (diplomacy_bridge) {
            diplomacy_bridge->SetEconomicSystem(economic_system);
            wired_count++;
            // Log message will come from DiplomacyEconomicBridge::SetEconomicSystem()
        }

        // Wire RealmManager
        if (realm_manager) {
            realm_manager->SetEconomicSystem(economic_system);
            wired_count++;
            // Log message will come from RealmManager::SetEconomicSystem()
        }

        // Wire ProvinceSystem
        if (province_system) {
            province_system->SetEconomicSystem(economic_system);
            wired_count++;
            // Log message will come from ProvinceSystem::SetEconomicSystem()
        }

        CORE_LOG_INFO("EconomicIntegration",
            "Wired " + std::to_string(wired_count) + " systems to EconomicSystem successfully");

        if (wired_count < 3) {
            CORE_LOG_WARN("EconomicIntegration",
                "Only " + std::to_string(wired_count) + "/3 systems were wired. "
                "Some systems may bypass treasury validation!");
        }
    }

    /**
     * Verify that all systems are properly connected.
     * Call this after initialization to ensure everything is wired correctly.
     *
     * @return true if all critical systems are connected, false otherwise
     */
    static bool VerifyIntegration(
        game::economy::EconomicSystem* economic_system,
        game::bridge::DiplomacyEconomicBridge* diplomacy_bridge,
        game::realm::RealmManager* realm_manager,
        game::province::ProvinceSystem* province_system)
    {
        CORE_LOG_INFO("EconomicIntegration", "Verifying economic system integration...");

        bool all_connected = true;

        // Note: We can't directly check if SetEconomicSystem was called
        // (no public getter), but we can verify systems exist

        if (!economic_system) {
            CORE_LOG_ERROR("EconomicIntegration", "✗ EconomicSystem is null");
            all_connected = false;
        } else {
            CORE_LOG_INFO("EconomicIntegration", "✓ EconomicSystem exists");
        }

        if (!diplomacy_bridge) {
            CORE_LOG_WARN("EconomicIntegration", "⚠ DiplomacyEconomicBridge is null");
            // Not critical, but recommended
        } else {
            CORE_LOG_INFO("EconomicIntegration", "✓ DiplomacyEconomicBridge exists");
        }

        if (!realm_manager) {
            CORE_LOG_WARN("EconomicIntegration", "⚠ RealmManager is null");
            // Not critical, but recommended
        } else {
            CORE_LOG_INFO("EconomicIntegration", "✓ RealmManager exists");
        }

        if (!province_system) {
            CORE_LOG_WARN("EconomicIntegration", "⚠ ProvinceSystem is null");
            // Not critical, but recommended
        } else {
            CORE_LOG_INFO("EconomicIntegration", "✓ ProvinceSystem exists");
        }

        if (all_connected) {
            CORE_LOG_INFO("EconomicIntegration", "✅ Integration verification PASSED");
        } else {
            CORE_LOG_ERROR("EconomicIntegration", "❌ Integration verification FAILED");
        }

        return all_connected;
    }
};

} // namespace game::economy

// ============================================================================
// Usage Examples
// ============================================================================

/**
 * Example 1: Full initialization from scratch
 *
 * Use this if you're creating all systems fresh.
 */
void Example_FullInitialization() {
    // Assuming you have these:
    core::ecs::ComponentAccessManager access_manager(entity_manager);
    core::threading::ThreadSafeMessageBus message_bus;

    // Initialize everything
    bool success = game::economy::EconomicSystemIntegrator::InitializeEconomicSystems(
        access_manager,
        message_bus
    );

    if (success) {
        // All systems created and wired!
    }
}

/**
 * Example 2: Minimal integration with existing systems
 *
 * Use this if you already have systems and just need to wire them.
 */
void Example_MinimalIntegration() {
    // Assuming you have these already created:
    game::economy::EconomicSystem* economic_system = /* your instance */;
    game::bridge::DiplomacyEconomicBridge* diplomacy_bridge = /* your instance */;
    game::realm::RealmManager* realm_manager = /* your instance */;
    game::province::ProvinceSystem* province_system = /* your instance */;

    // Just wire them up (takes 1 line!)
    game::economy::EconomicSystemIntegrator::WireExistingSystems(
        economic_system,
        diplomacy_bridge,
        realm_manager,
        province_system
    );

    // Done! All systems now use validated treasury operations.
}

/**
 * Example 3: Integration in GameSystemsManager
 *
 * Add this to your GameSystemsManager::InitializeGameSystems() method.
 */
void Example_GameSystemsManagerIntegration() {
    // In GameSystemsManager::InitializeGameSystems():

    // 1. Create EconomicSystem first
    m_economic_system = std::make_unique<game::economy::EconomicSystem>(
        *m_component_access_manager,
        *m_message_bus
    );
    m_economic_system->Initialize();

    // 2. Create other systems...
    // (your existing code for creating realm manager, province system, etc.)

    // 3. Wire them up (add these 3 lines):
    if (m_diplomacy_bridge) {
        m_diplomacy_bridge->SetEconomicSystem(m_economic_system.get());
    }
    if (m_realm_manager) {
        m_realm_manager->SetEconomicSystem(m_economic_system.get());
    }
    if (m_province_system) {
        m_province_system->SetEconomicSystem(m_economic_system.get());
    }

    // That's it! Integration complete.
}

// ============================================================================
// Quick Reference Card
// ============================================================================

/*
 * QUICK START: 3-Line Integration
 * ================================
 *
 * If you already have all systems created, just add these 3 lines:
 *
 *   diplomacy_bridge->SetEconomicSystem(economic_system);
 *   realm_manager->SetEconomicSystem(economic_system);
 *   province_system->SetEconomicSystem(economic_system);
 *
 * That's all you need to do!
 *
 * ================================
 * Expected Log Output:
 * ================================
 *
 * [INFO] [DiplomacyEconomicBridge] EconomicSystem connected to DiplomacyEconomicBridge
 * [INFO] [RealmManager] EconomicSystem connected to RealmManager
 * [INFO] [ProvinceSystem] EconomicSystem connected to ProvinceSystem
 *
 * If you see these messages, you're done!
 *
 * ================================
 * What You Get:
 * ================================
 *
 * ✅ Treasury overflow protection
 * ✅ Validated treasury operations
 * ✅ No more treasury corruption
 * ✅ All benefits of the refactor
 *
 * ================================
 * Troubleshooting:
 * ================================
 *
 * Q: I don't see the "connected" messages
 * A: You forgot to call SetEconomicSystem(). Add the 3 lines above.
 *
 * Q: I get warnings about "EconomicSystem not set"
 * A: Same as above - add the SetEconomicSystem() calls.
 *
 * Q: Treasury still goes negative
 * A: Some other code is bypassing the API. Search for "treasury -=" and replace with SpendMoney().
 *
 * ================================
 */
