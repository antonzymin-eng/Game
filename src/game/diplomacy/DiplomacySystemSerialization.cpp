// ============================================================================
// Date/Time Created: September 27, 2025 - 4:00 PM PST
// Intended Folder Location: src/game/diplomacy/DiplomacySystemSerialization.cpp
// DiplomacySystem Serialization Implementation
// ============================================================================

#include "game/diplomacy/DiplomacySystem.h"
#include "core/logging/Logger.h"
#include <jsoncpp/json/json.h>

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
            
            return root;
        }

        bool DiplomacySystem::Deserialize(const Json::Value& data, int version) {
            try {
                if (!data.isMember("system_name") || data["system_name"].asString() != "DiplomacySystem") {
                    ::core::logging::LogError("DiplomacySystem", "Invalid system data in deserialize");
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
                
                ::core::logging::LogInfo("DiplomacySystem", "Deserialization successful");
                return true;
                
            } catch (const std::exception& e) {
                ::core::logging::LogError("DiplomacySystem", 
                    "Deserialization failed: " + std::string(e.what()));
                return false;
            }
        }

    } // namespace diplomacy
} // namespace game
