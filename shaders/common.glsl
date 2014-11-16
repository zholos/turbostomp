uniform Common {
    mat4 PV;
    mat4 P;
    mat4 V;
    vec3 V_origin; // so we don't have to invert V in GLSL
};

const vec3 light_source = normalize(vec3(1, 2, 3));

const float tile_light_factor = .2;
