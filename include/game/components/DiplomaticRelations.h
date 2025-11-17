#pragma once

#include "utils/PlatformMacros.h"
#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <unordered_map>
#include <memory>
#include <sstream>
#include <json/json.h>

// Forward declaration for AI systems
// Full implementation in game/realm/RealmComponents.h as DiplomaticRelationsComponent

struct DiplomaticRelations {
    // Stub structure for AI systems that need basic diplomatic data
    // Real implementation is in RealmComponents.h as DiplomaticRelationsComponent
    
    enum class RelationType {
        NEUTRAL = 0,
        FRIENDLY,
        HOSTILE,
        ALLIED,
        AT_WAR
    };
    
    std::unordered_map<uint32_t, RelationType> relations;
    
    DiplomaticRelations() = default;
    ~DiplomaticRelations() = default;
    
    // Serialization methods required by ECS
    std::string Serialize() const {
        Json::Value json;
        Json::Value relationsJson(Json::objectValue);
        for (const auto& [entityId, relType] : relations) {
            relationsJson[std::to_string(entityId)] = static_cast<int>(relType);
        }
        json["relations"] = relationsJson;
        Json::StreamWriterBuilder builder;
        return Json::writeString(builder, json);
    }
    
    bool Deserialize(const std::string& data) {
        Json::CharReaderBuilder builder;
        Json::Value json;
        std::string errors;
        std::istringstream stream(data);
        
        if (!Json::parseFromStream(builder, stream, &json, &errors)) {
            return false;
        }
        
        relations.clear();
        if (json.isMember("relations") && json["relations"].isObject()) {
            for (const auto& key : json["relations"].getMemberNames()) {
                uint32_t entityId = std::stoul(key);
                int relTypeInt = json["relations"][key].asInt();
                relations[entityId] = static_cast<RelationType>(relTypeInt);
            }
        }
        return true;
    }
};
