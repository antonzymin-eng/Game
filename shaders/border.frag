#version 330 core

// ============================================================================
// Province Border Fragment Shader
// Purpose: Color borders based on type (province/nation/coastline)
// ============================================================================

in VS_OUT {
    float border_type;
    float thickness_factor;
} fs_in;

uniform vec4 border_color_normal;       // Default province border color
uniform vec4 border_color_coastline;    // Coastline color
uniform vec4 border_color_nation;       // Nation border color

out vec4 frag_color;

void main() {
    // Select color based on border type
    vec4 color;

    if (fs_in.border_type > 1.5) {
        // Nation border (thick black or custom)
        color = border_color_nation;
    } else if (fs_in.border_type > 0.5) {
        // Coastline (blue-ish)
        color = border_color_coastline;
    } else {
        // Normal province border
        color = border_color_normal;
    }

    frag_color = color;
}
