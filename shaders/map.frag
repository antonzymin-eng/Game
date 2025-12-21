#version 330 core

// ============================================================================
// Map Province Fragment Shader
// Purpose: Color provinces based on game state (political, terrain, etc.)
// ============================================================================

in VS_OUT {
    flat uint province_id;
    vec2 uv;
    vec2 world_pos;
} fs_in;

// Province data texture (stores color, owner, etc.)
uniform sampler2D province_data;              // RGB = color, A = unused
uniform sampler2D province_metadata;          // R = terrain type, G = owner, etc.

// Rendering mode
uniform int render_mode;                       // 0 = political, 1 = terrain, 2 = trade, etc.
uniform vec2 viewport_size;

// Selection and hover
uniform uint selected_province_id;
uniform uint hovered_province_id;
uniform float selection_glow_time;

out vec4 frag_color;

// Retrieve province color from texture
vec4 GetProvinceColor(uint province_id) {
    // Texture lookup: province_id maps to texel coordinate
    float u = float(province_id % 256u) / 256.0;
    float v = float(province_id / 256u) / 256.0;
    return texture(province_data, vec2(u, v));
}

// Terrain color palette
vec3 GetTerrainColor(uint terrain_type) {
    if (terrain_type == 0u) return vec3(0.2, 0.6, 0.2);  // Plains (green)
    if (terrain_type == 1u) return vec3(0.1, 0.4, 0.1);  // Forest (dark green)
    if (terrain_type == 2u) return vec3(0.5, 0.5, 0.5);  // Mountains (gray)
    if (terrain_type == 3u) return vec3(0.8, 0.8, 0.6);  // Desert (tan)
    if (terrain_type == 4u) return vec3(0.9, 0.9, 0.9);  // Snow (white)
    if (terrain_type == 5u) return vec3(0.1, 0.3, 0.5);  // Water (blue)
    return vec3(0.5, 0.5, 0.5);                           // Unknown (gray)
}

// Selection glow effect
vec3 ApplySelectionGlow(vec3 base_color, bool is_selected, bool is_hovered) {
    vec3 result = base_color;

    if (is_selected) {
        // Pulsing white glow
        float pulse = abs(sin(selection_glow_time * 3.0)) * 0.5 + 0.5;
        vec3 glow = vec3(1.0, 1.0, 1.0) * pulse * 0.4;
        result = mix(base_color, base_color + glow, 0.6);
    }
    else if (is_hovered) {
        // Subtle highlight
        result = base_color * 1.2;
    }

    return result;
}

void main() {
    // Get province color based on render mode
    vec4 province_color;

    if (render_mode == 0) {
        // Political mode: use province color from texture
        province_color = GetProvinceColor(fs_in.province_id);
    }
    else if (render_mode == 1) {
        // Terrain mode: use terrain-based colors
        vec4 metadata = texture(province_metadata, vec2(float(fs_in.province_id % 256u) / 256.0,
                                                         float(fs_in.province_id / 256u) / 256.0));
        uint terrain_type = uint(metadata.r * 10.0);
        province_color = vec4(GetTerrainColor(terrain_type), 1.0);
    }
    else {
        // Fallback: white
        province_color = vec4(1.0, 1.0, 1.0, 1.0);
    }

    // Apply selection/hover effects
    bool is_selected = (fs_in.province_id == selected_province_id);
    bool is_hovered = (fs_in.province_id == hovered_province_id);

    vec3 final_color = ApplySelectionGlow(province_color.rgb, is_selected, is_hovered);

    frag_color = vec4(final_color, province_color.a);
}
