#include "preamble.glsl"

uniform vec4 background;
uniform sampler2D color_buffer;
uniform sampler2D depth_buffer;
uniform int counter;

in vec2 t;
out vec4 fragColor;

void main() {
    vec4 color = texture2D(color_buffer, t);
    float depth = texture2D(depth_buffer, t).x;

    float fog_scale = 50*(1-depth);
    float fog_factor = exp(-fog_scale*fog_scale);
    vec4 fog_color = mix(color, background, fog_factor);

    fragColor = fog_color;
    gl_FragDepth = depth;
}
