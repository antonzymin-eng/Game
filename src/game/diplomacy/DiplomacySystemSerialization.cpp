// ============================================================================
// Date/Time Created: September 27, 2025 - 4:00 PM PST
// Intended Folder Location: src/game/diplomacy/DiplomacySystemSerialization.cpp
// DiplomacySystem Serialization Implementation
// ============================================================================

#include "game/diplomacy/DiplomacySystem.h"
#include "core/logging/Logger.h"
#include "utils/PlatformCompat.h"

namespace game {
    namespace diplomacy {

        // ============================================================================
        // Serialization Implementation
        // ============================================================================

        Json::Value DiplomacySystem::Serialize(int version) const {
            Json::Value root;
            root["version"] = version;
            root["system_name"] = "DiplomacySystem";

            // System state
            root["initialized"] = m_initialized;
            root["accumulated_time"] = m_accumulated_time;
            root["monthly_timer"] = m_monthly_timer;
            root["update_interval"] = m_update_interval;

            // Configuration
            root["base_war_weariness"] = m_base_war_weariness;
            root["diplomatic_speed"] = m_diplomatic_speed;
            root["alliance_reliability"] = m_alliance_reliability;

            // Serialize pending proposals
            Json::Value proposals(Json::arrayValue);
            for (const auto& proposal : m_pending_proposals) {
                Json::Value p;
                p["proposal_id"] = proposal.proposal_id;
                p["proposer"] = static_cast<int>(proposal.proposer);
                p["target"] = static_cast<int>(proposal.target);
                p["action_type"] = static_cast<int>(proposal.action_type);
                p["message"] = proposal.message;
                p["is_pending"] = proposal.is_pending;
                p["ai_evaluation"] = proposal.ai_evaluation;
                p["acceptance_chance"] = proposal.acceptance_chance;

                // Serialize terms
                Json::Value terms;
                for (const auto& [key, value] : proposal.terms) {
                    terms[key] = value;
                }
                p["terms"] = terms;

                // Serialize conditions
                Json::Value conditions(Json::arrayValue);
                for (const auto& condition : proposal.conditions) {
                    conditions.append(condition);
                }
                p["conditions"] = conditions;

                // Serialize timestamps
                auto proposed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    proposal.proposed_date.time_since_epoch()).count();
                auto expiry_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    proposal.expiry_date.time_since_epoch()).count();
                p["proposed_date_ms"] = static_cast<Json::Int64>(proposed_ms);
                p["expiry_date_ms"] = static_cast<Json::Int64>(expiry_ms);

                proposals.append(p);
            }
            root["pending_proposals"] = proposals;

            // Serialize diplomatic cooldowns
            Json::Value cooldowns;
            for (const auto& [key, timestamp] : m_diplomatic_cooldowns) {
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    timestamp.time_since_epoch()).count();
                cooldowns[key] = static_cast<Json::Int64>(ms);
            }
            root["diplomatic_cooldowns"] = cooldowns;

            // ========== Serialize all DiplomacyComponents ==========
            Json::Value components(Json::arrayValue);

            auto* entity_manager = m_access_manager.GetEntityManager();
            if (entity_manager) {
                auto all_realms = GetAllRealms();

                for (auto realm_id : all_realms) {
                    auto component = GetDiplomacyComponent(realm_id);
                    if (!component) continue;

                    Json::Value comp;
                    comp["realm_id"] = static_cast<Json::Int64>(realm_id);

                    // Basic component data
                    comp["personality"] = static_cast<int>(component->personality);
                    comp["prestige"] = component->prestige;
                    comp["diplomatic_reputation"] = component->diplomatic_reputation;
                    comp["war_weariness"] = component->war_weariness;

                    // Serialize allies
                    Json::Value allies_array(Json::arrayValue);
                    for (auto ally_id : component->allies) {
                        allies_array.append(static_cast<Json::Int64>(ally_id));
                    }
                    comp["allies"] = allies_array;

                    // Serialize enemies
                    Json::Value enemies_array(Json::arrayValue);
                    for (auto enemy_id : component->enemies) {
                        enemies_array.append(static_cast<Json::Int64>(enemy_id));
                    }
                    comp["enemies"] = enemies_array;

                    // Serialize relationships
                    Json::Value relationships(Json::arrayValue);
                    for (const auto& [other_realm, state] : component->relationships) {
                        Json::Value rel;
                        rel["other_realm"] = static_cast<Json::Int64>(other_realm);
                        rel["relation"] = static_cast<int>(state.relation);
                        rel["opinion"] = state.opinion;
                        rel["trust"] = state.trust;
                        rel["prestige_difference"] = state.prestige_difference;
                        rel["diplomatic_incidents"] = state.diplomatic_incidents;
                        rel["trade_volume"] = state.trade_volume;
                        rel["economic_dependency"] = state.economic_dependency;
                        rel["military_access"] = state.military_access;
                        rel["has_common_enemies"] = state.has_common_enemies;
                        rel["has_border_tensions"] = state.has_border_tensions;
                        rel["historical_opinion_average"] = state.historical_opinion_average;

                        // Serialize recent actions
                        Json::Value actions(Json::arrayValue);
                        for (const auto& action : state.recent_actions) {
                            actions.append(action);
                        }
                        rel["recent_actions"] = actions;

                        // Serialize timestamps
                        auto last_contact_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            state.last_contact.time_since_epoch()).count();
                        rel["last_contact_ms"] = static_cast<Json::Int64>(last_contact_ms);

                        auto last_major_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            state.last_major_action.time_since_epoch()).count();
                        rel["last_major_action_ms"] = static_cast<Json::Int64>(last_major_ms);

                        // Serialize action cooldowns
                        Json::Value action_cooldowns;
                        for (const auto& [action, cooldown_time] : state.action_cooldowns) {
                            auto cooldown_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                cooldown_time.time_since_epoch()).count();
                            action_cooldowns[std::to_string(static_cast<int>(action))] =
                                static_cast<Json::Int64>(cooldown_ms);
                        }
                        rel["action_cooldowns"] = action_cooldowns;

                        // Serialize opinion history
                        Json::Value opinion_hist(Json::arrayValue);
                        for (auto opinion_val : state.opinion_history) {
                            opinion_hist.append(opinion_val);
                        }
                        rel["opinion_history"] = opinion_hist;

                        // Serialize opinion modifiers
                        Json::Value modifiers(Json::arrayValue);
                        for (const auto& modifier : state.opinion_modifiers) {
                            Json::Value mod;
                            mod["source"] = modifier.source;
                            mod["value"] = modifier.value;
                            mod["weight"] = modifier.weight;
                            mod["is_permanent"] = modifier.is_permanent;
                            auto created_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                modifier.created.time_since_epoch()).count();
                            mod["created_ms"] = static_cast<Json::Int64>(created_ms);
                            modifiers.append(mod);
                        }
                        rel["opinion_modifiers"] = modifiers;

                        // Serialize historical opinion data
                        Json::Value hist_data;

                        Json::Value monthly_ops(Json::arrayValue);
                        for (auto val : state.historical_data.monthly_opinions) {
                            monthly_ops.append(val);
                        }
                        hist_data["monthly_opinions"] = monthly_ops;

                        Json::Value yearly_ops(Json::arrayValue);
                        for (auto val : state.historical_data.yearly_opinions) {
                            yearly_ops.append(val);
                        }
                        hist_data["yearly_opinions"] = yearly_ops;

                        hist_data["short_term_average"] = state.historical_data.short_term_average;
                        hist_data["medium_term_average"] = state.historical_data.medium_term_average;
                        hist_data["long_term_average"] = state.historical_data.long_term_average;
                        hist_data["highest_ever"] = state.historical_data.highest_ever;
                        hist_data["lowest_ever"] = state.historical_data.lowest_ever;

                        auto best_date_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            state.historical_data.best_relations_date.time_since_epoch()).count();
                        hist_data["best_relations_date_ms"] = static_cast<Json::Int64>(best_date_ms);

                        auto worst_date_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            state.historical_data.worst_relations_date.time_since_epoch()).count();
                        hist_data["worst_relations_date_ms"] = static_cast<Json::Int64>(worst_date_ms);

                        rel["historical_data"] = hist_data;

                        // Serialize hidden opinion features
                        rel["hide_true_opinion"] = state.hide_true_opinion;
                        rel["displayed_opinion"] = state.displayed_opinion;
                        rel["deception_quality"] = state.deception_quality;

                        relationships.append(rel);
                    }
                    comp["relationships"] = relationships;

                    // Serialize treaties
                    Json::Value treaties(Json::arrayValue);
                    for (const auto& treaty : component->active_treaties) {
                        Json::Value t;
                        t["treaty_id"] = treaty.treaty_id;
                        t["type"] = static_cast<int>(treaty.type);
                        t["signatory_a"] = static_cast<Json::Int64>(treaty.signatory_a);
                        t["signatory_b"] = static_cast<Json::Int64>(treaty.signatory_b);
                        t["is_active"] = treaty.is_active;
                        t["compliance_a"] = treaty.compliance_a;
                        t["compliance_b"] = treaty.compliance_b;
                        t["tribute_amount"] = treaty.tribute_amount;
                        t["trade_bonus"] = treaty.trade_bonus;

                        // Serialize terms
                        Json::Value treaty_terms;
                        for (const auto& [key, value] : treaty.terms) {
                            treaty_terms[key] = value;
                        }
                        t["terms"] = treaty_terms;

                        // Serialize conditions
                        Json::Value treaty_conditions(Json::arrayValue);
                        for (const auto& condition : treaty.conditions) {
                            treaty_conditions.append(condition);
                        }
                        t["conditions"] = treaty_conditions;

                        // Serialize timestamps
                        auto signed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            treaty.signed_date.time_since_epoch()).count();
                        t["signed_date_ms"] = static_cast<Json::Int64>(signed_ms);

                        auto expiry_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            treaty.expiry_date.time_since_epoch()).count();
                        t["expiry_date_ms"] = static_cast<Json::Int64>(expiry_ms);

                        // Serialize secret diplomacy fields
                        t["is_secret"] = treaty.is_secret;
                        t["secrecy_level"] = treaty.secrecy_level;

                        Json::Value known_by_array(Json::arrayValue);
                        for (auto realm_id : treaty.known_by) {
                            known_by_array.append(static_cast<Json::Int64>(realm_id));
                        }
                        t["known_by"] = known_by_array;

                        auto discovery_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            treaty.last_discovery_check.time_since_epoch()).count();
                        t["last_discovery_check_ms"] = static_cast<Json::Int64>(discovery_ms);

                        treaties.append(t);
                    }
                    comp["active_treaties"] = treaties;

                    // Serialize marriages
                    Json::Value marriages(Json::arrayValue);
                    for (const auto& marriage : component->marriages) {
                        Json::Value m;
                        m["marriage_id"] = marriage.marriage_id;
                        m["bride_realm"] = static_cast<Json::Int64>(marriage.bride_realm);
                        m["groom_realm"] = static_cast<Json::Int64>(marriage.groom_realm);
                        m["bride_character"] = static_cast<Json::Int64>(marriage.bride_character);
                        m["groom_character"] = static_cast<Json::Int64>(marriage.groom_character);
                        m["diplomatic_bonus"] = marriage.diplomatic_bonus;
                        m["inheritance_claim"] = marriage.inheritance_claim;
                        m["produces_alliance"] = marriage.produces_alliance;
                        m["is_active"] = marriage.is_active;

                        auto marriage_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            marriage.marriage_date.time_since_epoch()).count();
                        m["marriage_date_ms"] = static_cast<Json::Int64>(marriage_ms);

                        Json::Value children_array(Json::arrayValue);
                        for (auto child_id : marriage.children) {
                            children_array.append(static_cast<Json::Int64>(child_id));
                        }
                        m["children"] = children_array;

                        marriages.append(m);
                    }
                    comp["marriages"] = marriages;

                    components.append(comp);
                }
            }

            root["diplomacy_components"] = components;

            return root;
        }

        bool DiplomacySystem::Deserialize(const Json::Value& data, int version) {
            try {
                if (!data.isMember("system_name") || data["system_name"].asString() != "DiplomacySystem") {
                    CORE_LOG_ERROR("DiplomacySystem", "Invalid system data in deserialize");
                    return false;
                }

                // System state
                m_initialized = data.get("initialized", false).asBool();
                m_accumulated_time = data.get("accumulated_time", 0.0f).asFloat();
                m_monthly_timer = data.get("monthly_timer", 0.0f).asFloat();
                m_update_interval = data.get("update_interval", 1.0f).asFloat();

                // Configuration
                m_base_war_weariness = data.get("base_war_weariness", 0.1).asDouble();
                m_diplomatic_speed = data.get("diplomatic_speed", 1.0).asDouble();
                m_alliance_reliability = data.get("alliance_reliability", 0.8).asDouble();

                // Deserialize pending proposals
                m_pending_proposals.clear();
                if (data.isMember("pending_proposals") && data["pending_proposals"].isArray()) {
                    const Json::Value& proposals = data["pending_proposals"];
                    for (const auto& p : proposals) {
                        DiplomaticProposal proposal;
                        proposal.proposal_id = p.get("proposal_id", "").asString();
                        proposal.proposer = static_cast<types::EntityID>(p.get("proposer", 0).asInt());
                        proposal.target = static_cast<types::EntityID>(p.get("target", 0).asInt());
                        proposal.action_type = static_cast<DiplomaticAction>(p.get("action_type", 0).asInt());
                        proposal.message = p.get("message", "").asString();
                        proposal.is_pending = p.get("is_pending", true).asBool();
                        proposal.ai_evaluation = p.get("ai_evaluation", 0.0).asDouble();
                        proposal.acceptance_chance = p.get("acceptance_chance", 0.0).asDouble();

                        // Deserialize terms
                        if (p.isMember("terms")) {
                            const Json::Value& terms = p["terms"];
                            for (const auto& key : terms.getMemberNames()) {
                                proposal.terms[key] = terms[key].asDouble();
                            }
                        }

                        // Deserialize conditions
                        if (p.isMember("conditions") && p["conditions"].isArray()) {
                            const Json::Value& conditions = p["conditions"];
                            for (const auto& condition : conditions) {
                                proposal.conditions.push_back(condition.asString());
                            }
                        }

                        // Deserialize timestamps
                        if (p.isMember("proposed_date_ms")) {
                            auto ms = p["proposed_date_ms"].asInt64();
                            proposal.proposed_date = std::chrono::system_clock::time_point(
                                std::chrono::milliseconds(ms));
                        }

                        if (p.isMember("expiry_date_ms")) {
                            auto ms = p["expiry_date_ms"].asInt64();
                            proposal.expiry_date = std::chrono::system_clock::time_point(
                                std::chrono::milliseconds(ms));
                        }

                        m_pending_proposals.push_back(proposal);
                    }
                }

                // Deserialize diplomatic cooldowns
                m_diplomatic_cooldowns.clear();
                if (data.isMember("diplomatic_cooldowns")) {
                    const Json::Value& cooldowns = data["diplomatic_cooldowns"];
                    for (const auto& key : cooldowns.getMemberNames()) {
                        auto ms = cooldowns[key].asInt64();
                        m_diplomatic_cooldowns[key] = std::chrono::system_clock::time_point(
                            std::chrono::milliseconds(ms));
                    }
                }

                // ========== Deserialize all DiplomacyComponents ==========
                if (data.isMember("diplomacy_components") && data["diplomacy_components"].isArray()) {
                    const Json::Value& components = data["diplomacy_components"];

                    for (const auto& comp : components) {
                        auto realm_id = static_cast<types::EntityID>(comp.get("realm_id", 0).asInt64());
                        if (realm_id == 0) continue;

                        auto component = GetDiplomacyComponent(realm_id);
                        if (!component) continue;

                        // Deserialize basic component data
                        component->personality = static_cast<DiplomaticPersonality>(
                            comp.get("personality", 0).asInt());
                        component->prestige = comp.get("prestige", 0.0).asDouble();
                        component->diplomatic_reputation = comp.get("diplomatic_reputation", 1.0).asDouble();
                        component->war_weariness = comp.get("war_weariness", 0.0).asDouble();

                        // Deserialize allies
                        component->allies.clear();
                        if (comp.isMember("allies") && comp["allies"].isArray()) {
                            for (const auto& ally : comp["allies"]) {
                                component->allies.push_back(static_cast<types::EntityID>(ally.asInt64()));
                            }
                        }

                        // Deserialize enemies
                        component->enemies.clear();
                        if (comp.isMember("enemies") && comp["enemies"].isArray()) {
                            for (const auto& enemy : comp["enemies"]) {
                                component->enemies.push_back(static_cast<types::EntityID>(enemy.asInt64()));
                            }
                        }

                        // Deserialize relationships
                        component->relationships.clear();
                        if (comp.isMember("relationships") && comp["relationships"].isArray()) {
                            const Json::Value& relationships = comp["relationships"];
                            for (const auto& rel : relationships) {
                                auto other_realm = static_cast<types::EntityID>(rel.get("other_realm", 0).asInt64());
                                if (other_realm == 0) continue;

                                DiplomaticState state(other_realm);
                                state.relation = static_cast<DiplomaticRelation>(rel.get("relation", 0).asInt());
                                state.opinion = rel.get("opinion", 0).asInt();
                                state.trust = rel.get("trust", 0.5).asDouble();
                                state.prestige_difference = rel.get("prestige_difference", 0.0).asDouble();
                                state.diplomatic_incidents = rel.get("diplomatic_incidents", 0).asInt();
                                state.trade_volume = rel.get("trade_volume", 0.0).asDouble();
                                state.economic_dependency = rel.get("economic_dependency", 0.0).asDouble();
                                state.military_access = rel.get("military_access", false).asBool();
                                state.has_common_enemies = rel.get("has_common_enemies", false).asBool();
                                state.has_border_tensions = rel.get("has_border_tensions", false).asBool();
                                state.historical_opinion_average = rel.get("historical_opinion_average", 0.0).asDouble();

                                // Deserialize recent actions
                                if (rel.isMember("recent_actions") && rel["recent_actions"].isArray()) {
                                    state.recent_actions.clear();
                                    for (const auto& action : rel["recent_actions"]) {
                                        state.recent_actions.push_back(action.asString());
                                    }
                                }

                                // Deserialize timestamps
                                if (rel.isMember("last_contact_ms")) {
                                    auto ms = rel["last_contact_ms"].asInt64();
                                    state.last_contact = std::chrono::system_clock::time_point(
                                        std::chrono::milliseconds(ms));
                                }

                                if (rel.isMember("last_major_action_ms")) {
                                    auto ms = rel["last_major_action_ms"].asInt64();
                                    state.last_major_action = std::chrono::system_clock::time_point(
                                        std::chrono::milliseconds(ms));
                                }

                                // Deserialize action cooldowns
                                if (rel.isMember("action_cooldowns")) {
                                    const Json::Value& cooldowns = rel["action_cooldowns"];
                                    for (const auto& key : cooldowns.getMemberNames()) {
                                        auto action = static_cast<DiplomaticAction>(std::stoi(key));
                                        auto ms = cooldowns[key].asInt64();
                                        state.action_cooldowns[action] = std::chrono::system_clock::time_point(
                                            std::chrono::milliseconds(ms));
                                    }
                                }

                                // Deserialize opinion history
                                if (rel.isMember("opinion_history") && rel["opinion_history"].isArray()) {
                                    state.opinion_history.clear();
                                    for (const auto& opinion_val : rel["opinion_history"]) {
                                        state.opinion_history.push_back(opinion_val.asInt());
                                    }
                                }

                                // Deserialize opinion modifiers
                                if (rel.isMember("opinion_modifiers") && rel["opinion_modifiers"].isArray()) {
                                    state.opinion_modifiers.clear();
                                    for (const auto& mod : rel["opinion_modifiers"]) {
                                        DiplomaticState::OpinionModifier modifier;
                                        modifier.source = mod.get("source", "").asString();
                                        modifier.value = mod.get("value", 0).asInt();
                                        modifier.weight = mod.get("weight", 1.0).asDouble();
                                        modifier.is_permanent = mod.get("is_permanent", false).asBool();

                                        if (mod.isMember("created_ms")) {
                                            auto ms = mod["created_ms"].asInt64();
                                            modifier.created = std::chrono::system_clock::time_point(
                                                std::chrono::milliseconds(ms));
                                        }

                                        state.opinion_modifiers.push_back(modifier);
                                    }
                                }

                                // Deserialize historical opinion data
                                if (rel.isMember("historical_data")) {
                                    const Json::Value& hist = rel["historical_data"];

                                    if (hist.isMember("monthly_opinions") && hist["monthly_opinions"].isArray()) {
                                        state.historical_data.monthly_opinions.clear();
                                        for (const auto& val : hist["monthly_opinions"]) {
                                            state.historical_data.monthly_opinions.push_back(val.asInt());
                                        }
                                    }

                                    if (hist.isMember("yearly_opinions") && hist["yearly_opinions"].isArray()) {
                                        state.historical_data.yearly_opinions.clear();
                                        for (const auto& val : hist["yearly_opinions"]) {
                                            state.historical_data.yearly_opinions.push_back(val.asInt());
                                        }
                                    }

                                    state.historical_data.short_term_average = hist.get("short_term_average", 0.0).asDouble();
                                    state.historical_data.medium_term_average = hist.get("medium_term_average", 0.0).asDouble();
                                    state.historical_data.long_term_average = hist.get("long_term_average", 0.0).asDouble();
                                    state.historical_data.highest_ever = hist.get("highest_ever", 0).asInt();
                                    state.historical_data.lowest_ever = hist.get("lowest_ever", 0).asInt();

                                    if (hist.isMember("best_relations_date_ms")) {
                                        auto ms = hist["best_relations_date_ms"].asInt64();
                                        state.historical_data.best_relations_date = std::chrono::system_clock::time_point(
                                            std::chrono::milliseconds(ms));
                                    }

                                    if (hist.isMember("worst_relations_date_ms")) {
                                        auto ms = hist["worst_relations_date_ms"].asInt64();
                                        state.historical_data.worst_relations_date = std::chrono::system_clock::time_point(
                                            std::chrono::milliseconds(ms));
                                    }
                                }

                                // Deserialize hidden opinion features
                                state.hide_true_opinion = rel.get("hide_true_opinion", false).asBool();
                                state.displayed_opinion = rel.get("displayed_opinion", 0).asInt();
                                state.deception_quality = rel.get("deception_quality", 0.5).asDouble();

                                component->relationships[other_realm] = state;
                            }
                        }

                        // Deserialize treaties
                        component->active_treaties.clear();
                        if (comp.isMember("active_treaties") && comp["active_treaties"].isArray()) {
                            const Json::Value& treaties = comp["active_treaties"];
                            for (const auto& t : treaties) {
                                Treaty treaty;
                                treaty.treaty_id = t.get("treaty_id", "").asString();
                                treaty.type = static_cast<TreatyType>(t.get("type", 0).asInt());
                                treaty.signatory_a = static_cast<types::EntityID>(t.get("signatory_a", 0).asInt64());
                                treaty.signatory_b = static_cast<types::EntityID>(t.get("signatory_b", 0).asInt64());
                                treaty.is_active = t.get("is_active", true).asBool();
                                treaty.compliance_a = t.get("compliance_a", 1.0).asDouble();
                                treaty.compliance_b = t.get("compliance_b", 1.0).asDouble();
                                treaty.tribute_amount = t.get("tribute_amount", 0.0).asDouble();
                                treaty.trade_bonus = t.get("trade_bonus", 0.0).asDouble();

                                // Deserialize terms
                                if (t.isMember("terms")) {
                                    const Json::Value& terms = t["terms"];
                                    for (const auto& key : terms.getMemberNames()) {
                                        treaty.terms[key] = terms[key].asDouble();
                                    }
                                }

                                // Deserialize conditions
                                if (t.isMember("conditions") && t["conditions"].isArray()) {
                                    for (const auto& condition : t["conditions"]) {
                                        treaty.conditions.push_back(condition.asString());
                                    }
                                }

                                // Deserialize timestamps
                                if (t.isMember("signed_date_ms")) {
                                    auto ms = t["signed_date_ms"].asInt64();
                                    treaty.signed_date = std::chrono::system_clock::time_point(
                                        std::chrono::milliseconds(ms));
                                }

                                if (t.isMember("expiry_date_ms")) {
                                    auto ms = t["expiry_date_ms"].asInt64();
                                    treaty.expiry_date = std::chrono::system_clock::time_point(
                                        std::chrono::milliseconds(ms));
                                }

                                // Deserialize secret diplomacy fields
                                treaty.is_secret = t.get("is_secret", false).asBool();
                                treaty.secrecy_level = t.get("secrecy_level", 0.0).asDouble();

                                if (t.isMember("known_by") && t["known_by"].isArray()) {
                                    for (const auto& realm : t["known_by"]) {
                                        treaty.known_by.push_back(static_cast<types::EntityID>(realm.asInt64()));
                                    }
                                }

                                if (t.isMember("last_discovery_check_ms")) {
                                    auto ms = t["last_discovery_check_ms"].asInt64();
                                    treaty.last_discovery_check = std::chrono::system_clock::time_point(
                                        std::chrono::milliseconds(ms));
                                }

                                component->active_treaties.push_back(treaty);
                            }
                        }

                        // Deserialize marriages
                        component->marriages.clear();
                        if (comp.isMember("marriages") && comp["marriages"].isArray()) {
                            const Json::Value& marriages = comp["marriages"];
                            for (const auto& m : marriages) {
                                DynasticMarriage marriage;
                                marriage.marriage_id = m.get("marriage_id", "").asString();
                                marriage.bride_realm = static_cast<types::EntityID>(m.get("bride_realm", 0).asInt64());
                                marriage.groom_realm = static_cast<types::EntityID>(m.get("groom_realm", 0).asInt64());
                                marriage.bride_character = static_cast<types::EntityID>(m.get("bride_character", 0).asInt64());
                                marriage.groom_character = static_cast<types::EntityID>(m.get("groom_character", 0).asInt64());
                                marriage.diplomatic_bonus = m.get("diplomatic_bonus", 20.0).asDouble();
                                marriage.inheritance_claim = m.get("inheritance_claim", 0.0).asDouble();
                                marriage.produces_alliance = m.get("produces_alliance", false).asBool();
                                marriage.is_active = m.get("is_active", true).asBool();

                                if (m.isMember("marriage_date_ms")) {
                                    auto ms = m["marriage_date_ms"].asInt64();
                                    marriage.marriage_date = std::chrono::system_clock::time_point(
                                        std::chrono::milliseconds(ms));
                                }

                                if (m.isMember("children") && m["children"].isArray()) {
                                    for (const auto& child : m["children"]) {
                                        marriage.children.push_back(static_cast<types::EntityID>(child.asInt64()));
                                    }
                                }

                                component->marriages.push_back(marriage);
                            }
                        }
                    }

                    CORE_LOG_INFO("DiplomacySystem",
                        "Deserialized " + std::to_string(components.size()) + " diplomacy components");
                }

                CORE_LOG_INFO("DiplomacySystem", "Deserialization successful");
                return true;

            } catch (const std::exception& e) {
                CORE_LOG_ERROR("DiplomacySystem",
                    "Deserialization failed: " + std::string(e.what()));
                return false;
            }
        }

    } // namespace diplomacy
} // namespace game
