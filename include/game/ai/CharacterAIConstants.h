// Created: November 19, 2025
// Location: include/game/ai/CharacterAIConstants.h
// Purpose: Named constants for CharacterAI system to improve maintainability

#pragma once

namespace AI {
namespace CharacterAIConstants {

// ============================================================================
// Personality Thresholds
// ============================================================================

namespace Personality {
    constexpr float HIGH_AMBITION = 0.8f;
    constexpr float MODERATE_AMBITION = 0.6f;
    constexpr float LOW_LOYALTY = 0.3f;
    constexpr float MODERATE_LOYALTY = 0.5f;
    constexpr float HIGH_LOYALTY = 0.7f;
    constexpr float LOW_HONOR = 0.4f;
    constexpr float HIGH_HONOR = 0.7f;
    constexpr float HIGH_GREED = 0.7f;
    constexpr float HIGH_BOLDNESS = 0.7f;
    constexpr float HIGH_COMPASSION = 0.7f;
}

// ============================================================================
// Plot Decision Constants
// ============================================================================

namespace Plot {
    constexpr float ASSASSINATION_RISK = 0.9f;
    constexpr float ASSASSINATION_BASE_SUCCESS = 0.3f;
    constexpr float COUP_RISK = 0.95f;
    constexpr float COUP_BASE_SUCCESS = 0.2f;
    constexpr float BLACKMAIL_RISK = 0.6f;
    constexpr float BLACKMAIL_BASE_SUCCESS = 0.5f;
    constexpr float FABRICATE_CLAIM_RISK = 0.4f;
    constexpr float FABRICATE_CLAIM_SUCCESS = 0.6f;

    constexpr float MIN_DESIRABILITY_THRESHOLD = 0.6f;
    constexpr float MIN_SUCCESS_THRESHOLD = 0.3f;
    constexpr float MIN_VIABLE_SUCCESS = 0.4f;
    constexpr float RISK_MULTIPLIER = 0.5f;
}

// ============================================================================
// Relationship Constants
// ============================================================================

namespace Relationship {
    constexpr float POSITIVE_DECAY_RATE = -0.5f;
    constexpr float NEGATIVE_RECOVERY_RATE = 0.3f;
    constexpr float RIVAL_THRESHOLD = -50.0f;
    constexpr float FRIEND_THRESHOLD = 70.0f;
    constexpr float NEUTRAL_LOWER = 30.0f;
    constexpr float MIN_OPINION = -100.0f;
    constexpr float MAX_OPINION = 100.0f;
}

// ============================================================================
// Proposal Constants
// ============================================================================

namespace Proposal {
    constexpr float TITLE_BASE_ACCEPTANCE = 0.3f;
    constexpr float TITLE_LOYALTY_MODIFIER = 0.4f;
    constexpr float GOLD_BASE_ACCEPTANCE = 0.4f;
    constexpr float GOLD_LOYALTY_MODIFIER = 0.3f;
    constexpr float MARRIAGE_BASE_ACCEPTANCE = 0.5f;
    constexpr float COUNCIL_BASE_ACCEPTANCE = 0.2f;
    constexpr float COUNCIL_LOYALTY_MODIFIER = 0.5f;
    constexpr float WAR_BASE_ACCEPTANCE = 0.3f;
    constexpr float COMPASSION_MODIFIER = 0.2f;
    constexpr float SUCCESS_MIN_THRESHOLD = 0.5f;
}

// ============================================================================
// Ambition Constants
// ============================================================================

namespace Ambition {
    constexpr int ACHIEVEMENT_TIME_HOURS = 720; // 30 days
    constexpr float PROGRESS_HALF = 0.5f;
}

// ============================================================================
// Mood Constants
// ============================================================================

namespace Mood {
    constexpr float HAPPY_THRESHOLD = 0.7f;
    constexpr float CONTENT_THRESHOLD = 0.3f;
    constexpr float STRESSED_THRESHOLD = -0.3f;
    constexpr float ANGRY_THRESHOLD = -0.7f;

    constexpr float RELATIONSHIP_WEIGHT = 0.01f;
    constexpr float AMBITION_ACHIEVED_BONUS = 0.5f;
    constexpr float AMBITION_UNACHIEVED_PENALTY = 0.2f;
    constexpr float NEGATIVE_EVENT_PENALTY = 0.1f;
    constexpr float ACTIVE_PLOT_STRESS = 0.15f;
    constexpr float RIVAL_STRESS = 0.3f;

    constexpr float HAPPY_MODIFIER = 1.2f;
    constexpr float CONTENT_MODIFIER = 1.0f;
    constexpr float AMBITIOUS_MODIFIER = 1.3f;
    constexpr float STRESSED_MODIFIER = 0.9f;
    constexpr float ANGRY_MODIFIER = 1.3f;
    constexpr float AFRAID_MODIFIER = 0.5f;
    constexpr float DESPERATE_MODIFIER = 1.5f;
}

// ============================================================================
// Memory Constants
// ============================================================================

namespace Memory {
    constexpr size_t MAX_MEMORIES = 30;
    constexpr int MEMORY_LIFETIME_HOURS = 8760; // 1 year
}

// ============================================================================
// Decision Modifiers
// ============================================================================

namespace DecisionModifiers {
    constexpr float AMBITION_BASE_WEIGHT = 0.4f;
    constexpr float HONOR_PENALTY_ASSASSINATION = 0.5f;
    constexpr float HONOR_PENALTY_BLACKMAIL = 0.5f;
    constexpr float LOYALTY_PENALTY_COUP = 0.6f;
    constexpr float BOLDNESS_RISK_WEIGHT = 0.3f;
    constexpr float SUCCESS_WEIGHT = 0.4f;

    constexpr float DESPERATE_BONUS = 0.3f;
    constexpr float ANGRY_BONUS = 0.2f;
    constexpr float AMBITIOUS_BONUS = 0.25f;
    constexpr float AFRAID_PENALTY = 0.2f;
}

// ============================================================================
// Opinion Changes
// ============================================================================

namespace OpinionChange {
    constexpr float PLOTTING_PENALTY = -30.0f;
    constexpr float PROPOSAL_GRANTED = 10.0f;
    constexpr float PROPOSAL_DENIED = -5.0f;
    constexpr float BEFRIEND_BONUS = 15.0f;
    constexpr float ROMANCE_BONUS = 30.0f;
    constexpr float RIVAL_PENALTY = -40.0f;
    constexpr float MENTOR_BONUS = 20.0f;
    constexpr float BLACKMAIL_PENALTY = -50.0f;
    constexpr float MARRIAGE_BONUS = 50.0f;
    constexpr float DIVORCE_PENALTY = -30.0f;
    constexpr float FEAST_BONUS = 5.0f;
    constexpr float ARTIFACT_BONUS = 10.0f;
    constexpr float REBELLION_PENALTY = -20.0f;
}

// ============================================================================
// Information Processing
// ============================================================================

namespace Information {
    constexpr float HIGH_SEVERITY_THRESHOLD = 0.7f;
    constexpr float MODERATE_SEVERITY_THRESHOLD = 0.5f;
    constexpr float SEDUCTION_DESIRABILITY_THRESHOLD = 0.7f;
}

// ============================================================================
// Stress and Mood Modifiers
// ============================================================================

namespace Stress {
    constexpr float MINOR_STRESS = 0.2f;
    constexpr float MODERATE_STRESS = 0.5f;
    constexpr float MINOR_RELIEF = 0.1f;
    constexpr float MODERATE_RELIEF = 0.3f;
    constexpr float MAJOR_RELIEF = 0.4f;
}

// ============================================================================
// Personal Action Costs and Benefits
// ============================================================================

namespace PersonalAction {
    constexpr float SKILL_IMPROVEMENT_BENEFIT = 0.8f;
    constexpr float SKILL_IMPROVEMENT_COST = 50.0f;
    constexpr float ESTATE_MANAGEMENT_BENEFIT = 0.7f;
    constexpr float ESTATE_MANAGEMENT_COST = 100.0f;
    constexpr float PILGRIMAGE_BENEFIT = 0.6f;
    constexpr float PILGRIMAGE_COST = 200.0f;
    constexpr float ARTIFACT_BENEFIT = 0.5f;
    constexpr float ARTIFACT_COST = 500.0f;
    constexpr float FEAST_BENEFIT = 0.4f;
    constexpr float FEAST_COST = 150.0f;
}

// ============================================================================
// Relationship Desirability
// ============================================================================

namespace RelationshipDesire {
    constexpr float HIGH_OPINION_THRESHOLD = 0.5f;
    constexpr float BEFRIEND_DESIRABILITY = 0.6f;
    constexpr float SEDUCTION_DESIRABILITY = 0.8f;
    constexpr float BLACKMAIL_DESIRABILITY = 0.7f;
    constexpr float RIVAL_DESIRABILITY = 0.5f;
    constexpr float MENTOR_DESIRABILITY = 0.4f;
    constexpr float NEUTRAL_DESIRABILITY = 0.3f;
    constexpr float RELATIONSHIP_VALUE_WEIGHT = 0.005f;
    constexpr float COMPASSION_WEIGHT = 0.2f;
    constexpr float RIVAL_VALUE_PENALTY = 0.5f;
    constexpr float FRIEND_VALUE_BONUS = 0.3f;
}

} // namespace CharacterAIConstants
} // namespace AI
