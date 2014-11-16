#include "preamble.glsl"
#include "lib.glsl"

flat in vec4 face_color, edge_color;
in vec4 border;

out vec4 fragColor;

const float edge_width = 0.05;

void main() {
    // TODO: smoothstep
    bool edge = comp_min(border) < edge_width;
    fragColor = edge ? edge_color : face_color;
}
