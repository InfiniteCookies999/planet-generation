#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;

out vec4 finalColor;

uniform vec3 u_star_center;
uniform float u_star_radius;
// uniform float u_ray_radius;
uniform float u_rays_length;
uniform uint u_seed;
uniform vec3 u_ray_color_near;
uniform vec3 u_ray_color_far;
uniform int u_tick;

const float PI = 3.14159265358979323846;

// MurmurHash3
uint mix32(uint h) {
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

float mix32_float(uint x) {
    // TODO (maddie): do we need to set precision to highp? we may be getting lower than 24 bits.
    // see: https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl

    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    x = mix32(x);

    x &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    x |= ieeeOne;                          // Add fractional part to 1.0

    return uintBitsToFloat(x) - 1.0;

}

float noise1d(float x) {
    // see: https://eev.ee/blog/2016/05/29/perlin-noise/

    int cell0 = int(floor(x));
    int cell1 = cell0 + 1;

    float dx0 = x - float(cell0);
    float dx1 = x - float(cell1);

    float slope0 = mix32_float(uint(cell0) ^ u_seed);
    float slope1 = mix32_float(uint(cell1) ^ u_seed);

    // points along the slopes.
    float h0 = dx0 * slope0;
    float h1 = dx1 * slope1;

    float t = dx0;
    return h1*t + h0*(1.0 - t);
}

void main() {

    // Thoughts: since we are rendering only using the angular distance
    // around the sphere if we rotate around the sphere we see the exact
    // same rays.
    // A possible way of achieving more accurate results could be to then
    // use the longitude and latitude combined with the angular distance
    // to do 3D noise.

    const float BEND_FACTOR = 0.35;
    const float SUB_FACTOR = 0.8;
    const float ADD_FACTOR = 0.5;

    // compute distance from center.
    // TODO (maddie): we could potentially do this in squared units instead.
    float abs_dist = length(fragPosition - u_star_center);
    float dist = abs_dist - u_star_radius; // absolute distance offset from star.

    float angle = atan(fragTexCoord.y - 0.5, fragTexCoord.x - 0.5);
    if (angle < 0.0) angle += 2.0 * PI;

    float bend = noise1d(angle * 4.0) * BEND_FACTOR;

    // Apply a slight bend to some rays so they do not all appear straight.
    angle += dist * bend;

    float length_sub = (noise1d(angle * 35.0) + 1.0) / 2.0;
    float length_sub_mod = (noise1d(angle * 15.0 + sin(float(u_tick) * 0.05) * 0.25 ) + 1.0) / 2.0;
    float length_sub_mod2 = (noise1d(angle * 20.0) + 1.0) / 2.0;

    length_sub *= length_sub_mod * 2.1;


    float length_add = (noise1d(angle * 2.0) + 1.0) / 2.0;

    // scale the difference to be more dramatic.
    length_add = (length_add - 0.3) * 2.4;
    length_add = clamp(length_add, 0.0, 1.0);

    // vary the length of each ray.
    float ray_length = u_rays_length - (length_sub * SUB_FACTOR) + (length_add * ADD_FACTOR);

    // relative distance away from star center.
    float l = dist / ray_length;
    l = 1.0 - l;
    l = clamp(l, 0.0, 1.0);

    // background glow
    // TODO (maddie): we could potentially do this in squared units instead.
    float glow_dist = abs_dist - u_star_radius;
    glow_dist /= ((u_rays_length + u_star_radius) + 0.9) - u_star_radius;
    float glow_t = 1.0 - glow_dist;
    if (glow_t < 0.0) glow_t = 0.0;
    vec4 background = vec4(u_ray_color_far, glow_t);


    vec3 ray_color = mix(u_ray_color_far, u_ray_color_near, l * l);
    vec4 rays_color_blended = vec4(ray_color, l);

    finalColor = background*0.15 + rays_color_blended;

}
