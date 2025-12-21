#version 330 core

// ============================================================================
// Province Border Vertex Shader
// Purpose: Render province boundaries with varying thickness
// ============================================================================

layout(location = 0) in vec2 position;        // World coordinates
layout(location = 1) in vec2 normal;          // Border normal for thickness
layout(location = 2) in float border_type;    // 0=normal, 1=coastline, 2=nation border

uniform mat4 view_projection;
uniform float zoom_level;

out VS_OUT {
    float border_type;
    float thickness_factor;
} vs_out;

void main() {
    // Calculate border thickness based on zoom and type
    float base_thickness = 1.0;
    if (border_type > 1.5) {
        // Nation border (thick)
        base_thickness = 3.0;
    } else if (border_type > 0.5) {
        // Coastline (medium)
        base_thickness = 2.0;
    }

    // Scale thickness inversely with zoom (maintains visual size)
    float screen_thickness = base_thickness / zoom_level;

    // Extrude vertex along normal for line thickness
    vec2 offset_pos = position + normal * screen_thickness * 0.001; // 0.001 = world units

    gl_Position = view_projection * vec4(offset_pos, 0.0, 1.0);

    vs_out.border_type = border_type;
    vs_out.thickness_factor = screen_thickness;
}
