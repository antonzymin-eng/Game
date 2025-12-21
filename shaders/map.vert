#version 330 core

// ============================================================================
// Map Province Vertex Shader
// Purpose: Transform world coordinates to screen space using camera matrix
// ============================================================================

layout(location = 0) in vec2 position;       // World coordinates (x, y)
layout(location = 1) in uint province_id;     // Province identifier
layout(location = 2) in vec2 uv;              // Texture coordinates (for future use)

uniform mat4 view_projection;                 // Camera matrix (world -> screen)

out VS_OUT {
    flat uint province_id;                    // Province ID (flat = no interpolation)
    vec2 uv;
    vec2 world_pos;                           // Original world position
} vs_out;

void main() {
    // Transform world position to clip space
    gl_Position = view_projection * vec4(position, 0.0, 1.0);

    // Pass data to fragment shader
    vs_out.province_id = province_id;
    vs_out.uv = uv;
    vs_out.world_pos = position;
}
