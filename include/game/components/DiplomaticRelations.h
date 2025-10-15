#pragma once

#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <unordered_map>
#include <memory>

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
};
