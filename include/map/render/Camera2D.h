// ============================================================================
// Camera2D.h - Shared 2D Camera Interface for Map Rendering
// Created: December 22, 2025
// Description: Common camera abstraction for both CPU and GPU renderers
// ============================================================================

#pragma once

#include <glm/glm.hpp>

namespace game::map {

// ============================================================================
// Camera2D - Shared camera state for map rendering
// ============================================================================

struct Camera2D {
    glm::vec2 position;          // World position (center of view)
    float zoom;                  // Zoom level (1.0 = normal, 2.0 = 2x closer)
    float viewport_width;        // Viewport width in pixels
    float viewport_height;       // Viewport height in pixels

    // Default constructor
    Camera2D()
        : position(0.0f, 0.0f)
        , zoom(1.0f)
        , viewport_width(1280.0f)
        , viewport_height(720.0f)
    {}

    // Calculate view-projection matrix for rendering
    glm::mat4 GetViewProjectionMatrix() const {
        float half_width = (viewport_width / zoom) / 2.0f;
        float half_height = (viewport_height / zoom) / 2.0f;

        float left = position.x - half_width;
        float right = position.x + half_width;
        float bottom = position.y - half_height;
        float top = position.y + half_height;

        return glm::ortho(left, right, bottom, top);
    }

    // Get visible world bounds (for frustum culling)
    struct Bounds {
        float left, right, top, bottom;
    };

    Bounds GetVisibleBounds() const {
        float half_width = (viewport_width / zoom) / 2.0f;
        float half_height = (viewport_height / zoom) / 2.0f;

        return Bounds{
            position.x - half_width,   // left
            position.x + half_width,   // right
            position.y + half_height,  // top
            position.y - half_height   // bottom
        };
    }
};

} // namespace game::map
