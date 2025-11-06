#include "game/diplomacy/DiplomacyComponents.h"
#include "game/config/GameConfig.h"
#include <algorithm>

namespace game::diplomacy {

    // ============================================================================
    // Data Structure Implementations
    // ============================================================================

    DiplomaticState::DiplomaticState(game::types::EntityID realm) : other_realm(realm) {
        last_contact = std::chrono::system_clock::now();
    }

    bool DiplomaticState::IsActionOnCooldown(DiplomaticAction action) const {
        auto it = action_cooldowns.find(action);
        if (it == action_cooldowns.end()) {
            return false;
        }
        return std::chrono::system_clock::now() < it->second;
    }

    void DiplomaticState::SetActionCooldown(DiplomaticAction action, int cooldown_days) {
        auto& config = game::config::GameConfig::Instance();
        
        // Get default cooldowns from config if not specified
        if (cooldown_days == 0) {
            switch (action) {
                case DiplomaticAction::DECLARE_WAR:
                    cooldown_days = config.GetInt("diplomacy.cooldown.declare_war", 365); // 1 year default
                    break;
                case DiplomaticAction::PROPOSE_ALLIANCE:
                    cooldown_days = config.GetInt("diplomacy.cooldown.propose_alliance", 180); // 6 months
                    break;
                case DiplomaticAction::PROPOSE_TRADE:
                    cooldown_days = config.GetInt("diplomacy.cooldown.propose_trade", 90); // 3 months
                    break;
                case DiplomaticAction::DEMAND_TRIBUTE:
                    cooldown_days = config.GetInt("diplomacy.cooldown.demand_tribute", 365); // 1 year
                    break;
                case DiplomaticAction::ISSUE_ULTIMATUM:
                    cooldown_days = config.GetInt("diplomacy.cooldown.issue_ultimatum", 180); // 6 months
                    break;
                case DiplomaticAction::ARRANGE_MARRIAGE:
                    cooldown_days = config.GetInt("diplomacy.cooldown.arrange_marriage", 730); // 2 years
                    break;
                default:
                    cooldown_days = config.GetInt("diplomacy.cooldown.default", 30); // 1 month default
                    break;
            }
        }
        
        auto now = std::chrono::system_clock::now();
        action_cooldowns[action] = now + std::chrono::hours(24 * cooldown_days);
        last_major_action = now;
    }

    int DiplomaticState::GetRemainingCooldownDays(DiplomaticAction action) const {
        auto it = action_cooldowns.find(action);
        if (it == action_cooldowns.end()) {
            return 0;
        }
        
        auto now = std::chrono::system_clock::now();
        if (now >= it->second) {
            return 0;
        }
        
        auto duration = std::chrono::duration_cast<std::chrono::hours>(it->second - now);
        return static_cast<int>(duration.count() / 24);
    }

    void DiplomaticState::ApplyOpinionDecay(float time_delta, int neutral_baseline) {
        auto& config = game::config::GameConfig::Instance();
        
        // Get decay rate from config (default: 0.1 points per time unit)
        double decay_rate = config.GetDouble("diplomacy.opinion_decay_rate", 0.1) * time_delta;
        
        // Positive opinions decay toward baseline
        if (opinion > neutral_baseline) {
            int decay_amount = static_cast<int>(decay_rate);
            opinion = std::max(neutral_baseline, opinion - decay_amount);
        }
        // Negative opinions recover toward baseline
        else if (opinion < neutral_baseline) {
            int recovery_amount = static_cast<int>(decay_rate);
            opinion = std::min(neutral_baseline, opinion + recovery_amount);
        }
    }

    void DiplomaticState::ApplyTrustDecay(float time_delta, double neutral_baseline) {
        auto& config = game::config::GameConfig::Instance();
        
        // Get decay rate from config (default: 0.01 per time unit)
        double decay_rate = config.GetDouble("diplomacy.trust_decay_rate", 0.01) * time_delta;
        
        // Trust drifts toward baseline (usually 0.5)
        if (trust > neutral_baseline) {
            trust = std::max(neutral_baseline, trust - decay_rate);
        }
        else if (trust < neutral_baseline) {
            trust = std::min(neutral_baseline, trust + decay_rate);
        }
        
        // Clamp trust to valid range [0.0, 1.0]
        trust = std::clamp(trust, 0.0, 1.0);
    }

    void DiplomaticState::UpdateOpinionHistory(int new_opinion) {
        auto& config = game::config::GameConfig::Instance();
        
        // Get history window size from config (default: 12 data points)
        int history_window = config.GetInt("diplomacy.opinion_history_window", 12);
        
        // Add new opinion to history
        opinion_history.push_back(new_opinion);
        
        // Trim history to window size
        while (opinion_history.size() > static_cast<size_t>(history_window)) {
            opinion_history.pop_front();
        }
        
        // Recalculate rolling average
        if (!opinion_history.empty()) {
            double sum = 0.0;
            for (int val : opinion_history) {
                sum += val;
            }
            historical_opinion_average = sum / opinion_history.size();
        } else {
            historical_opinion_average = 0.0;
        }
    }

    Treaty::Treaty(TreatyType treaty_type, game::types::EntityID realm_a, game::types::EntityID realm_b)
        : type(treaty_type), signatory_a(realm_a), signatory_b(realm_b) {
        signed_date = std::chrono::system_clock::now();
        
        int duration_years = 10;
        switch (treaty_type) {
        case TreatyType::NON_AGGRESSION:
            duration_years = 10; break;
        case TreatyType::TRADE_AGREEMENT:
            duration_years = 5; break;
        case TreatyType::ALLIANCE:
            duration_years = 20; break;
        case TreatyType::MARRIAGE_PACT:
            duration_years = 50; break;
        default:
            duration_years = 10; break;
        }
        
        expiry_date = signed_date + std::chrono::hours(24 * 365 * duration_years);
        treaty_id = std::to_string(realm_a) + "_" + std::to_string(realm_b) + "_" + std::to_string(static_cast<int>(treaty_type));
    }

    bool Treaty::IsExpired() const {
        return std::chrono::system_clock::now() > expiry_date;
    }

    bool Treaty::IsBroken() const {
        return !is_active || (compliance_a < 0.5 || compliance_b < 0.5);
    }

    double Treaty::GetOverallCompliance() const {
        return (compliance_a + compliance_b) / 2.0;
    }

    DynasticMarriage::DynasticMarriage(game::types::EntityID bride, game::types::EntityID groom)
        : bride_realm(bride), groom_realm(groom) {
        marriage_date = std::chrono::system_clock::now();
        marriage_id = std::to_string(bride) + "_" + std::to_string(groom) + "_marriage";
        bride_character = bride; // Simplified - normally would be specific character
        groom_character = groom; // Simplified - normally would be specific character
    }

    DiplomaticProposal::DiplomaticProposal(game::types::EntityID from, game::types::EntityID to, DiplomaticAction action)
        : proposer(from), target(to), action_type(action) {
        proposed_date = std::chrono::system_clock::now();
        expiry_date = proposed_date + std::chrono::hours(24 * 30);
        proposal_id = std::to_string(from) + "_" + std::to_string(to) + "_" + std::to_string(static_cast<int>(action));
    }

    // ============================================================================
    // DiplomacyComponent Implementation
    // ============================================================================

    DiplomaticState* DiplomacyComponent::GetRelationship(game::types::EntityID other_realm) {
        auto it = relationships.find(other_realm);
        if (it != relationships.end()) {
            return &it->second;
        }

        relationships[other_realm] = DiplomaticState(other_realm);
        return &relationships[other_realm];
    }

    const DiplomaticState* DiplomacyComponent::GetRelationship(game::types::EntityID other_realm) const {
        auto it = relationships.find(other_realm);
        return (it != relationships.end()) ? &it->second : nullptr;
    }

    void DiplomacyComponent::SetRelation(game::types::EntityID other_realm, DiplomaticRelation relation) {
        auto* state = GetRelationship(other_realm);
        if (state) {
            state->relation = relation;

            auto ally_it = std::find(allies.begin(), allies.end(), other_realm);
            auto enemy_it = std::find(enemies.begin(), enemies.end(), other_realm);

            if (relation == DiplomaticRelation::ALLIED && ally_it == allies.end()) {
                allies.push_back(other_realm);
                if (enemy_it != enemies.end()) {
                    enemies.erase(enemy_it);
                }
            }
            else if (relation == DiplomaticRelation::AT_WAR && enemy_it == enemies.end()) {
                enemies.push_back(other_realm);
                if (ally_it != allies.end()) {
                    allies.erase(ally_it);
                }
            }
        }
    }

    void DiplomacyComponent::ModifyOpinion(game::types::EntityID other_realm, int opinion_change, const std::string& reason) {
        auto& config = game::config::GameConfig::Instance();
        int max_opinion = config.GetInt("diplomacy.max_opinion", 100);
        int min_opinion = config.GetInt("diplomacy.min_opinion", -100);
        
        auto* state = GetRelationship(other_realm);
        if (state) {
            state->opinion += opinion_change;
            state->opinion = std::clamp(state->opinion, min_opinion, max_opinion);
            state->recent_actions.push_back(reason + " (" +
                (opinion_change > 0 ? "+" : "") + std::to_string(opinion_change) + ")");

            int max_recent_actions = config.GetInt("diplomacy.max_recent_actions", 10);
            if (state->recent_actions.size() > static_cast<size_t>(max_recent_actions)) {
                state->recent_actions.pop_front();
            }
            
            // Update long-term opinion history for AI memory
            state->UpdateOpinionHistory(state->opinion);
        }
    }

    void DiplomacyComponent::AddTreaty(const Treaty& treaty) {
        active_treaties.push_back(treaty);
    }

    void DiplomacyComponent::RemoveTreaty(const std::string& treaty_id) {
        active_treaties.erase(
            std::remove_if(active_treaties.begin(), active_treaties.end(),
                [&treaty_id](const Treaty& t) { return t.treaty_id == treaty_id; }),
            active_treaties.end());
    }

    void DiplomacyComponent::BreakTreaty(game::types::EntityID other_realm, TreatyType type) {
        // Deactivate the treaty
        for (auto& treaty : active_treaties) {
            if ((treaty.signatory_a == other_realm || treaty.signatory_b == other_realm) &&
                treaty.type == type && treaty.is_active) {
                treaty.is_active = false;
            }
        }
        
        // Apply trust penalty for breaking treaty
        auto* relationship = GetRelationship(other_realm);
        if (relationship) {
            // Significant trust penalty - breaking treaties is a serious diplomatic offense
            relationship->trust = std::max(0.0, relationship->trust - 0.3);
            
            // Add diplomatic incident
            relationship->diplomatic_incidents++;
            
            // Opinion penalty (scaled by treaty importance)
            int opinion_penalty = -15;  // Base penalty
            switch(type) {
                case TreatyType::ALLIANCE:
                    opinion_penalty = -30;  // Breaking alliance is very serious
                    break;
                case TreatyType::NON_AGGRESSION:
                    opinion_penalty = -25;
                    break;
                case TreatyType::TRADE_AGREEMENT:
                    opinion_penalty = -15;
                    break;
                case TreatyType::DEFENSIVE_LEAGUE:
                    opinion_penalty = -20;
                    break;
                default:
                    opinion_penalty = -10;
                    break;
            }
            
            ModifyOpinion(other_realm, opinion_penalty, "Broke treaty");
        }
    }

    std::vector<Treaty*> DiplomacyComponent::GetTreatiesWith(game::types::EntityID other_realm) {
        std::vector<Treaty*> treaties;
        for (auto& treaty : active_treaties) {
            if (treaty.signatory_a == other_realm || treaty.signatory_b == other_realm) {
                treaties.push_back(&treaty);
            }
        }
        return treaties;
    }

    bool DiplomacyComponent::HasTreatyType(game::types::EntityID other_realm, TreatyType type) const {
        for (const auto& treaty : active_treaties) {
            if ((treaty.signatory_a == other_realm || treaty.signatory_b == other_realm) &&
                treaty.type == type && treaty.is_active) {
                return true;
            }
        }
        return false;
    }

    bool DiplomacyComponent::IsAtWar() const {
        return !enemies.empty();
    }

    bool DiplomacyComponent::IsAtWarWith(game::types::EntityID other_realm) const {
        return std::find(enemies.begin(), enemies.end(), other_realm) != enemies.end();
    }

    bool DiplomacyComponent::IsAlliedWith(game::types::EntityID other_realm) const {
        return std::find(allies.begin(), allies.end(), other_realm) != allies.end();
    }

    std::vector<game::types::EntityID> DiplomacyComponent::GetWarEnemies() const {
        return enemies;
    }

    std::vector<game::types::EntityID> DiplomacyComponent::GetMilitaryAllies() const {
        return allies;
    }

} // namespace game::diplomacy