struct loc {
    vec4 ps; // position, scale
    vec4 q;
};

vec4 quat_conjugate(vec4 q) {
    return vec4(-q.xyz, q.w);
}

vec3 quat_rotate(vec4 q, vec3 v) {
    // see e.g. glm
    return v + 2*q.w*cross(q.xyz, v) + 2*cross(q.xyz, cross(q.xyz, v));
}

vec4 loc_apply(loc l, vec4 v) {
    // TODO: do something with v.w
    return vec4(l.ps.xyz + l.ps.w * quat_rotate(l.q, v.xyz), 1);
}

float comp_min(vec2 v) { return min(v.x, v.y); }
float comp_min(vec3 v) { return min(min(v.x, v.y), v.z); }
float comp_max(vec3 v) { return max(max(v.x, v.y), v.z); }
float comp_min(vec4 v) { return min(min(v.x, v.y), min(v.z, v.w)); }

vec4 gamma_mix(vec4 x, vec4 y, float a) {
    return sqrt(mix(x*x, y*y, a));
}

vec3 RGB_to_sRGB(vec3 c) {
    return mix(12.92*c, 1.055*pow(c, vec3(1/2.4))-.055, step(.0031308, c));
}

vec3 sRGB_to_RGB(vec3 c) {
    return mix(c/12.92, pow((c+.055)/1.055, vec3(2.4)), step(.04045, c));
}

vec3 sRGB_to_HSL(vec3 c) {
    float M = comp_max(c),
          m = comp_min(c),
          C = M - m,
          L = (M + m)/2;
    float H = C==0 ? 0 :
              M==c.g ? 2 + (c.b-c.r)/C :
              M==c.b ? 4 + (c.r-c.g)/C :
                       mod((c.g-c.b)/C, 6);
    float S = C==0 ? 0 : C/(1-abs(2*L-1));
    return vec3(H, S, L);
}

vec3 sRGB_to_YCH(vec3 c) {
    float M = comp_max(c),
          m = comp_min(c),
          C = M - m;
    float Y = .2126*c.r + .7152*c.g + .0722*c.b; // Rec. 709 formula
    return vec3(Y, C, sRGB_to_HSL(c).x);
}

vec3 HSL_to_sRGB(vec3 c) {
    float C = c.y*(1-abs(2*c.z-1));
    float m = c.z - C/2;
    return clamp(abs(mod(c.x-vec3(0,2,4), 6)-3)-1, 0, 1) * C + m;
}

vec3 YCH_to_sRGB(vec3 c) {
    vec3 c1 = HSL_to_sRGB(vec3(c.z, 1, .5));
    float y1 = sRGB_to_YCH(c1).x;
    return clamp((c1-y1)*c.y + c.x, 0, 1); // TODO: better fix
}

vec3 adjust_luma(vec3 c, float p) {
    return sRGB_to_RGB(YCH_to_sRGB(
        pow(sRGB_to_YCH(RGB_to_sRGB(c)), vec3(p, 1, 1))));
}

vec4 adjust_luma(vec4 c, float p) {
    return vec4(adjust_luma(vec3(c), p), c.a);
}
