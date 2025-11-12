// ============================================================================
// InfluenceCalculator.h - Sphere of Influence Calculations
// Created: November 11, 2025 - Phase 3: Sphere of Influence System
// Location: include/game/diplomacy/InfluenceCalculator.h
// ============================================================================

#pragma once

#include "game/diplomacy/InfluenceComponents.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include "game/realm/RealmComponents.h"
#include "core/types/game_types.h"
#include <vector>

namespace game {
namespace diplomacy {

// Forward declarations
namespace realm = game::realm;

/**
 * @brief Pure calculation functions for influence system
 * All functions are static with no side effects
 * Calculates power projection across 7 different influence types
 */
class InfluenceCalculator {
public:
    // ========================================================================
    // Main Influence Calculation Functions
    // ========================================================================

    /**
     * Calculate military influence projection
     * Based on: army size, military tech, military prestige, fortifications
     * Range: 2-4 hops (high decay rate: 0.40)
     */
    static double CalculateMilitaryInfluence(
        const realm::RealmComponent& realm,
        const DiplomaticState* diplo_state = nullptr);

    /**
     * Calculate economic influence projection
     * Based on: wealth, trade volume, trade hubs, economic dependency
     * Range: 5-8 hops (low decay rate: 0.15)
     */
    static double CalculateEconomicInfluence(
        const realm::RealmComponent& realm,
        const DiplomaticState* diplo_state = nullptr);

    /**
     * Calculate dynastic influence projection
     * Based on: marriage ties, dynasty prestige, family connections
     * Range: Unlimited (very low decay rate: 0.05)
     */
    static double CalculateDynasticInfluence(
        const realm::RealmComponent& source_realm,
        const realm::RealmComponent& target_realm,
        const realm::DynastyComponent* source_dynasty = nullptr,
        const realm::DynastyComponent* target_dynasty = nullptr);

    /**
     * Calculate personal influence projection
     * Based on: ruler friendships, character bonds, personal opinion
     * Range: 3-5 hops (medium decay rate: 0.25)
     */
    static double CalculatePersonalInfluence(
        const realm::RealmComponent& source_realm,
        const realm::RealmComponent& target_realm,
        const DiplomaticState* diplo_state = nullptr);

    /**
     * Calculate religious influence projection
     * Based on: religious authority, fervor, same faith bonus
     * Range: Unlimited for same faith (no decay rate: 0.00)
     */
    static double CalculateReligiousInfluence(
        const realm::RealmComponent& source_realm,
        const realm::RealmComponent& target_realm);

    /**
     * Calculate cultural influence projection
     * Based on: cultural similarity, cultural attraction, shared traditions
     * Range: 4-6 hops (medium decay rate: 0.20)
     */
    static double CalculateCulturalInfluence(
        const realm::RealmComponent& source_realm,
        const realm::RealmComponent& target_realm);

    /**
     * Calculate prestige influence projection
     * Based on: diplomatic reputation, glory, recent victories
     * Range: Global (low decay rate: 0.10)
     */
    static double CalculatePrestigeInfluence(
        const realm::RealmComponent& realm,
        const realm::DynastyComponent* dynasty = nullptr);

    // ========================================================================
    // Component Calculations for Military Influence
    // ========================================================================

    /**
     * Calculate military strength score (0-100)
     * Based on standing army + levies
     */
    static double CalculateMilitaryStrength(const realm::RealmComponent& realm);

    /**
     * Calculate military technology bonus (0-20)
     * Placeholder for future tech system
     */
    static double CalculateMilitaryTechBonus(const realm::RealmComponent& realm);

    /**
     * Calculate military prestige bonus (0-30)
     * Based on recent victories and realm rank
     */
    static double CalculateMilitaryPrestigeBonus(const realm::RealmComponent& realm);

    // ========================================================================
    // Component Calculations for Economic Influence
    // ========================================================================

    /**
     * Calculate wealth score (0-60)
     * Based on treasury and monthly income
     */
    static double CalculateWealthScore(const realm::RealmComponent& realm);

    /**
     * Calculate trade dominance (0-30)
     * Based on trade volume and economic dependency
     */
    static double CalculateTradeDominance(
        const realm::RealmComponent& realm,
        const DiplomaticState* diplo_state);

    /**
     * Calculate trade hub bonus (0-10)
     * Based on capital and trade route control
     */
    static double CalculateTradeHubBonus(const realm::RealmComponent& realm);

    // ========================================================================
    // Component Calculations for Dynastic Influence
    // ========================================================================

    /**
     * Calculate marriage tie strength (0-50)
     * Returns higher values if realms share marriage ties
     */
    static double CalculateMarriageTieStrength(
        const realm::RealmComponent& source_realm,
        const realm::RealmComponent& target_realm);

    /**
     * Calculate dynasty prestige contribution (0-30)
     */
    static double CalculateDynastyPrestige(const realm::DynastyComponent* dynasty);

    /**
     * Calculate family connection bonus (0-20)
     * Bonus if dynasties are related
     */
    static double CalculateFamilyConnectionBonus(
        const realm::DynastyComponent* source_dynasty,
        const realm::DynastyComponent* target_dynasty);

    // ========================================================================
    // Component Calculations for Personal Influence
    // ========================================================================

    /**
     * Calculate ruler friendship strength (0-60)
     * Based on opinion and personal relationship
     */
    static double CalculateRulerFriendship(
        const realm::RealmComponent& source_realm,
        const realm::RealmComponent& target_realm,
        const DiplomaticState* diplo_state);

    /**
     * Calculate trust bonus (0-20)
     * Based on diplomatic trust level
     */
    static double CalculateTrustBonus(const DiplomaticState* diplo_state);

    /**
     * Calculate personal bond strength (0-20)
     * Based on character traits and compatibility
     */
    static double CalculatePersonalBondStrength(
        const realm::RealmComponent& source_realm,
        const realm::RealmComponent& target_realm);

    // ========================================================================
    // Component Calculations for Religious Influence
    // ========================================================================

    /**
     * Calculate religious authority (0-60)
     * Theocracies and high-piety rulers have more
     */
    static double CalculateReligiousAuthority(const realm::RealmComponent& realm);

    /**
     * Calculate same faith bonus (0-40)
     * High bonus if both realms share same faith
     */
    static double CalculateSameFaithBonus(
        const realm::RealmComponent& source_realm,
        const realm::RealmComponent& target_realm);

    // ========================================================================
    // Component Calculations for Cultural Influence
    // ========================================================================

    /**
     * Calculate cultural similarity (0-70)
     * Higher if cultures are similar or same
     */
    static double CalculateCulturalSimilarity(
        const realm::RealmComponent& source_realm,
        const realm::RealmComponent& target_realm);

    /**
     * Calculate cultural attraction (0-30)
     * Advanced/prestigious cultures are more attractive
     */
    static double CalculateCulturalAttraction(const realm::RealmComponent& realm);

    // ========================================================================
    // Component Calculations for Prestige Influence
    // ========================================================================

    /**
     * Calculate diplomatic reputation (0-50)
     * Based on realm rank and stability
     */
    static double CalculateDiplomaticReputation(const realm::RealmComponent& realm);

    /**
     * Calculate glory score (0-30)
     * Based on achievements and dynasty prestige
     */
    static double CalculateGloryScore(
        const realm::RealmComponent& realm,
        const realm::DynastyComponent* dynasty);

    /**
     * Calculate recent victory bonus (0-20)
     * Temporary boost from military successes
     */
    static double CalculateRecentVictoryBonus(const realm::RealmComponent& realm);

    // ========================================================================
    // Utility Functions
    // ========================================================================

    /**
     * Normalize influence value to 0-100 range
     */
    static double NormalizeInfluence(double raw_value, double max_value = 100.0);

    /**
     * Apply relationship modifier to influence
     * Opinion affects effectiveness of influence projection
     */
    static double ApplyRelationshipModifier(double base_influence, int opinion);

    /**
     * Calculate geographic decay based on distance
     * Returns modifier (0-1) based on hops and influence type
     */
    static double CalculateGeographicDecay(int hops, InfluenceType type);

    /**
     * Check if two realms are neighbors (share border)
     */
    static bool AreNeighbors(
        const realm::RealmComponent& realm1,
        const realm::RealmComponent& realm2);

    /**
     * Clamp value to range
     */
    static double Clamp(double value, double min_val, double max_val);
};

} // namespace diplomacy
} // namespace game
