#include "preamble.glsl"
#include "lib.glsl"
#include "common.glsl"

flat in vec4 face_color, edge_color;
flat in vec4 Sr2;
in vec3 wb;
flat in vec4 q;

out vec4 fragColor;

const float light_factor = .2;

//                                  S (Sphere center)
//                                  *
//                                 /|\
//                          '     / | \     '
//                           '.  /  |  \  .'
//       0 *--------------------*---+---*'
//     (Eye)   w -->            N   R
//           (Ray       (Near point
//         direction)    on spehere)
//
//    ||S-N|| = r (sphere radius)
//    P = project(S, w)
//    discard if ||S-P|| > r
//    ||N-P|| = sqrt(r^2 - ||S-P||)
//    N = P - ||N-P|| w

void main() {
    vec3 S = Sr2.xyz; float r2 = Sr2.w;
    vec3 w = normalize(S + wb);
    vec3 R = w * dot(w, S);
    float NR2 = r2 - dot(S-R, S-R);
    if (NR2 < 0)
        discard;
    vec3 N = R - w * sqrt(NR2);

    vec3 tm = N - S; // texture coordinate in model space
    vec3 tw = quat_rotate(q, tm); // in world space

    vec4 clip = PV * vec4(N + V_origin, 1);
    float depth = clip.z / clip.w;
    gl_FragDepth = .5*(gl_DepthRange.near + gl_DepthRange.far +
                       gl_DepthRange.diff * depth);

    float light = -log(.5+.5*light_factor*dot(tm, light_source));

    float f = comp_min(abs(tw)) * 2;
    vec4 color = mix(edge_color, face_color,
        smoothstep(.08, .09, f) + 1-smoothstep(.01, .02, f));

    fragColor = adjust_luma(color, light);
}
