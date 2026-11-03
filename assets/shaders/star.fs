#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

// raylib standard uniforms
uniform sampler2D texture0;
uniform sampler2D texture1;

uniform vec3 u_dark_color;
uniform vec3 u_dark_primary_color;
uniform vec3 u_bright_primary_color;
uniform vec3 u_brighter_color;
uniform vec3 u_white_color;

uniform int u_tick;

void main() {

    vec2 uv = fragTexCoord;
    float offset = float(u_tick) * 0.00005;
    uv.x = mod(uv.x + offset, 1.0);

    float density0 = texture(texture0, uv).r;
    float density1 = texture(texture1, uv).r;

    float amp0 = 1.0 + cos(u_tick * 0.015) * 0.05;
    float amp1 = 0.5 + (sin(u_tick * 0.015) + 1) * 0.05;
    float density = density0 * amp0 + density1 * amp1;
    density *= 0.5; // average

    // TODO (maddie): we could also try using proper color mix functions these are just lerps.

    float approx_max = 0.65;
    vec3 background = mix(u_bright_primary_color, u_dark_primary_color, 0.95 - density);

    const float bright_threshold = 0.4;
    const float white_threshold = 0.55;
    const float dark_threshold = 0.3;
    vec3 color = vec3(1.0);
    if (density > bright_threshold) {
        float t0 = (density - bright_threshold) / (approx_max - bright_threshold);
        if (density > white_threshold) {
            float t = (density - white_threshold) / (approx_max - white_threshold);
            color = mix(background, mix(u_brighter_color, u_white_color, t), t0);
        } else {
            color = mix(background, u_brighter_color, t0);
        }
    } else if (density < dark_threshold) {
        float t = 1.0 - density / dark_threshold;
        color = mix(background, u_dark_color, t);
    } else {
        color = background;
    }

    finalColor = vec4(color, 1.0);
}
