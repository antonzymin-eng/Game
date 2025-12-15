// Created: November 19, 2025
// Location: include/game/character/CharacterEducation.h
// Purpose: Character education and skill progression system

#pragma once

#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <vector>
#include <string>
#include <chrono>
#include <unordered_map>

namespace game {
namespace character {

// ============================================================================
// Education Focus
// ============================================================================

enum class EducationFocus : uint8_t {
    DIPLOMACY,      // Focus on diplomatic skills
    MARTIAL,        // Focus on military skills
    STEWARDSHIP,    // Focus on administrative skills
    INTRIGUE,       // Focus on espionage and intrigue
    LEARNING,       // Focus on scholarship
    BALANCED,       // Balanced education
    NONE,
    COUNT
};

// ============================================================================
// Education Quality
// ============================================================================

enum class EducationQuality : uint8_t {
    POOR,           // +1 to focus stat
    AVERAGE,        // +2 to focus stat
    GOOD,           // +3 to focus stat
    EXCELLENT,      // +4 to focus stat
    OUTSTANDING,    // +5 to focus stat, +1 to others
    COUNT
};

// ============================================================================
// Skill Experience
// ============================================================================

struct SkillExperience {
    int diplomacy_xp = 0;
    int martial_xp = 0;
    int stewardship_xp = 0;
    int intrigue_xp = 0;
    int learning_xp = 0;

    // XP required for next level (varies by current skill level)
    static int XPForNextLevel(int current_level) {
        return 100 + (current_level * 50);  // 100, 150, 200, 250...
    }

    // Check if skill can level up
    bool CanLevelUp(int current_skill, int xp) const {
        if (current_skill >= 20) return false;  // Max skill is 20
        return xp >= XPForNextLevel(current_skill);
    }
};

// ============================================================================
// Character Education Component (ECS)
// ============================================================================

class CharacterEducationComponent : public ::core::ecs::Component<CharacterEducationComponent> {
public:
    types::EntityID character_id{0};

    // Education history
    bool is_educated = false;
    EducationFocus education_focus = EducationFocus::NONE;
    EducationQuality education_quality = EducationQuality::AVERAGE;
    types::EntityID educator{0};  // Tutor/mentor character ID
    std::chrono::system_clock::time_point education_start;
    std::chrono::system_clock::time_point education_end;

    // Skill progression
    SkillExperience skill_xp;

    // Learning modifiers
    float learning_rate_modifier = 1.0f;  // Affected by traits, tutor quality, etc.

    // Education traits gained
    std::vector<std::string> education_traits;  // e.g., "scholarly", "martial_educated"

    CharacterEducationComponent() = default;
    explicit CharacterEducationComponent(types::EntityID char_id)
        : character_id(char_id)
    {}

    // ========================================================================
    // Education Management
    // ========================================================================

    /**
     * Start education with a tutor
     */
    void StartEducation(EducationFocus focus,
                       types::EntityID tutor_id = 0,
                       float tutor_quality = 1.0f) {
        education_focus = focus;
        educator = tutor_id;
        education_start = std::chrono::system_clock::now();
        learning_rate_modifier = tutor_quality;
        is_educated = true;
    }

    /**
     * Complete education and determine quality
     */
    EducationQuality CompleteEducation(int total_xp_gained) {
        education_end = std::chrono::system_clock::now();

        // Determine education quality based on XP gained
        if (total_xp_gained < 100) {
            education_quality = EducationQuality::POOR;
        } else if (total_xp_gained < 250) {
            education_quality = EducationQuality::AVERAGE;
        } else if (total_xp_gained < 500) {
            education_quality = EducationQuality::GOOD;
        } else if (total_xp_gained < 800) {
            education_quality = EducationQuality::EXCELLENT;
        } else {
            education_quality = EducationQuality::OUTSTANDING;
        }

        return education_quality;
    }

    /**
     * Check if currently being educated
     */
    bool IsInEducation() const {
        return is_educated &&
               education_start.time_since_epoch().count() > 0 &&
               education_end.time_since_epoch().count() == 0;
    }

    // ========================================================================
    // Skill Progression
    // ========================================================================

    /**
     * Gain experience in a skill
     */
    void GainExperience(EducationFocus skill, int amount) {
        int modified_amount = static_cast<int>(amount * learning_rate_modifier);

        switch (skill) {
            case EducationFocus::DIPLOMACY:
                skill_xp.diplomacy_xp += modified_amount;
                break;
            case EducationFocus::MARTIAL:
                skill_xp.martial_xp += modified_amount;
                break;
            case EducationFocus::STEWARDSHIP:
                skill_xp.stewardship_xp += modified_amount;
                break;
            case EducationFocus::INTRIGUE:
                skill_xp.intrigue_xp += modified_amount;
                break;
            case EducationFocus::LEARNING:
                skill_xp.learning_xp += modified_amount;
                break;
            default:
                break;
        }
    }

    /**
     * Check if any skill is ready to level up
     */
    struct LevelUpCheck {
        bool diplomacy_ready = false;
        bool martial_ready = false;
        bool stewardship_ready = false;
        bool intrigue_ready = false;
        bool learning_ready = false;
    };

    LevelUpCheck CheckLevelUps(int diplomacy_skill, int martial_skill,
                              int stewardship_skill, int intrigue_skill,
                              int learning_skill) const {
        LevelUpCheck result;
        result.diplomacy_ready = skill_xp.CanLevelUp(diplomacy_skill, skill_xp.diplomacy_xp);
        result.martial_ready = skill_xp.CanLevelUp(martial_skill, skill_xp.martial_xp);
        result.stewardship_ready = skill_xp.CanLevelUp(stewardship_skill, skill_xp.stewardship_xp);
        result.intrigue_ready = skill_xp.CanLevelUp(intrigue_skill, skill_xp.intrigue_xp);
        result.learning_ready = skill_xp.CanLevelUp(learning_skill, skill_xp.learning_xp);
        return result;
    }

    /**
     * Consume XP after leveling up a skill
     */
    void ConsumeXP(EducationFocus skill, int current_level) {
        int required_xp = SkillExperience::XPForNextLevel(current_level);

        switch (skill) {
            case EducationFocus::DIPLOMACY:
                skill_xp.diplomacy_xp -= required_xp;
                break;
            case EducationFocus::MARTIAL:
                skill_xp.martial_xp -= required_xp;
                break;
            case EducationFocus::STEWARDSHIP:
                skill_xp.stewardship_xp -= required_xp;
                break;
            case EducationFocus::INTRIGUE:
                skill_xp.intrigue_xp -= required_xp;
                break;
            case EducationFocus::LEARNING:
                skill_xp.learning_xp -= required_xp;
                break;
            default:
                break;
        }
    }

    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * Get XP for the current education focus
     * @return XP value for the active focus, or 0 if no education
     */
    int GetCurrentFocusXP() const {
        switch (education_focus) {
            case EducationFocus::DIPLOMACY:
                return skill_xp.diplomacy_xp;
            case EducationFocus::MARTIAL:
                return skill_xp.martial_xp;
            case EducationFocus::STEWARDSHIP:
                return skill_xp.stewardship_xp;
            case EducationFocus::INTRIGUE:
                return skill_xp.intrigue_xp;
            case EducationFocus::LEARNING:
                return skill_xp.learning_xp;
            default:
                return 0;
        }
    }

    /**
     * Get education duration in years
     */
    int GetEducationDurationYears() const {
        if (!is_educated || education_start.time_since_epoch().count() == 0) {
            return 0;
        }

        auto end = (education_end.time_since_epoch().count() > 0) ?
                   education_end : std::chrono::system_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::hours>(
            end - education_start);
        return static_cast<int>(duration.count() / 8760); // hours per year
    }

    /**
     * Get education quality as string
     */
    std::string GetEducationQualityString() const {
        switch (education_quality) {
            case EducationQuality::POOR: return "Poor";
            case EducationQuality::AVERAGE: return "Average";
            case EducationQuality::GOOD: return "Good";
            case EducationQuality::EXCELLENT: return "Excellent";
            case EducationQuality::OUTSTANDING: return "Outstanding";
            default: return "None";
        }
    }

    /**
     * Get education focus as string
     */
    std::string GetEducationFocusString() const {
        switch (education_focus) {
            case EducationFocus::DIPLOMACY: return "Diplomacy";
            case EducationFocus::MARTIAL: return "Martial";
            case EducationFocus::STEWARDSHIP: return "Stewardship";
            case EducationFocus::INTRIGUE: return "Intrigue";
            case EducationFocus::LEARNING: return "Learning";
            case EducationFocus::BALANCED: return "Balanced";
            default: return "None";
        }
    }

    // ========================================================================
    // Phase 6.5: Serialization
    // ========================================================================

    std::string Serialize() const override;
    bool Deserialize(const std::string& data) override;
};

// ============================================================================
// Experience Gain Events
// ============================================================================

struct ExperienceGainEvent {
    EducationFocus skill;
    int amount;
    std::string source;  // "battle", "study", "governing", etc.

    ExperienceGainEvent(EducationFocus s, int amt, const std::string& src)
        : skill(s), amount(amt), source(src) {}
};

// ============================================================================
// Education System Helper Functions
// ============================================================================

namespace EducationUtils {

    /**
     * Calculate XP gain from an activity
     */
    inline int CalculateXPGain(const std::string& activity_type,
                              int activity_difficulty = 1,
                              float character_learning = 5.0f) {
        // Base XP depends on activity
        int base_xp = 10;
        if (activity_type == "battle") base_xp = 20;
        else if (activity_type == "study") base_xp = 15;
        else if (activity_type == "governing") base_xp = 12;
        else if (activity_type == "scheming") base_xp = 18;

        // Difficulty multiplier (1-5)
        int difficulty_bonus = activity_difficulty * 5;

        // Learning stat bonus (0-20 range gives 0-10 bonus)
        int learning_bonus = static_cast<int>(character_learning / 2.0f);

        return base_xp + difficulty_bonus + learning_bonus;
    }

    /**
     * Get education trait based on focus and quality
     */
    inline std::string GetEducationTrait(EducationFocus focus, EducationQuality quality) {
        if (quality < EducationQuality::GOOD) {
            return "";  // No trait for poor/average education
        }

        std::string prefix;
        switch (focus) {
            case EducationFocus::DIPLOMACY: prefix = "diplomatic"; break;
            case EducationFocus::MARTIAL: prefix = "martial"; break;
            case EducationFocus::STEWARDSHIP: prefix = "administrative"; break;
            case EducationFocus::INTRIGUE: prefix = "cunning"; break;
            case EducationFocus::LEARNING: prefix = "scholarly"; break;
            default: return "";
        }

        if (quality == EducationQuality::OUTSTANDING) {
            return prefix + "_genius";
        } else if (quality == EducationQuality::EXCELLENT) {
            return prefix + "_master";
        } else {
            return prefix + "_educated";
        }
    }

    /**
     * Calculate tutor quality modifier
     */
    inline float CalculateTutorQuality(int tutor_learning_skill,
                                      int tutor_focus_skill,
                                      bool has_scholarly_trait) {
        float base = 1.0f;

        // Learning skill bonus (5-20 range gives 0-0.75 bonus)
        base += (tutor_learning_skill - 5) * 0.05f;

        // Focus skill bonus
        base += (tutor_focus_skill - 5) * 0.05f;

        // Trait bonus
        if (has_scholarly_trait) {
            base += 0.3f;
        }

        return std::max(0.5f, std::min(2.0f, base));  // Clamp to 0.5-2.0
    }

} // namespace EducationUtils

} // namespace character
} // namespace game
