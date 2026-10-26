#version 330

// raylib standard attributes
in vec3 fragPosition;
in vec2 fragTexCoord;

out vec4 finalColor;

// raylib standard uniforms
uniform sampler2D texture0;
uniform sampler2D texture1;

uniform vec4 u_dark_color;
uniform vec4 u_dark_primary_color;
uniform vec4 u_bright_primary_color;
uniform vec4 u_brighter_color;
uniform vec4 u_white_color;

uniform int u_tick;

void main() {

    // atan with 2 arguments in glsl is atan2.
    // float longitude = atan2_approx(fragPosition.z, fragPosition.x);
    // if (longitude < 0) longitude += 2.0 * PI; // 0 to 2 PI
    // float tx = longitude * INV_2PI;

    // // TODO (maddie): hardcoding radius!
    // float radius = 4.0;
    // float latitude = asin(fragPosition.y / radius) + H_PI; // TODO (maddie): approximate
    // float ty = latitude * INV_PI;

    // float offset = float(u_tick) * 0.00005;
    // tx = mod(tx + offset, 1.0);

    vec2 uv = fragTexCoord; //vec2(tx, ty);
    float offset = float(u_tick) * 0.00005;
    uv.x = mod(uv.x + offset, 1.0);

    float density0 = texture(texture0, uv).r;
    float density1 = texture(texture1, uv).r;

    float amp0 = 1.0 + cos(u_tick * 0.015) * 0.05;
    float amp1 = 0.5 + (sin(u_tick * 0.015) + 1) * 0.05;
    float density = density0 * amp0 + density1 * amp1;
    density *= 0.5; // average

    // float density = texture(texture0, fragTexCoord).r;

    // finalColor = vec4(density, density, density, 1.0);


    // // this just makes it looks like it slowly moves
    // float offset = 0.0;//float(u_tick) * 0.00015;
    // float tx = mod(fragTexCoord.x + offset, 1.0);
    // float ty = mod(fragTexCoord.y + offset, 1.0);

    // // perlin_noise_sample_circle(tx, ty, 40);
    // // perlin_noise_sample_circle(tx, ty, 80);

    // float density0 = perlin_noise_cell_wrapped_sample(tx, ty, 40);
    // float density1 = perlin_noise_cell_wrapped_sample(tx, ty, 80);

    // // this makes it more or less look like it shrinks and grows.
    // float amp0 = 1.0 + cos(u_tick * 0.015) * 0.1;
    // float amp1 = 0.5 + (sin(u_tick * 0.015) + 1) * 0.15;

    // float density = density0 * amp0 + density1 * amp1;


    // density = (density + 1) * 0.5; // convert range to 0 to 1.

    // // TODO (maddie): if this approximation is off thats an issue.
    // // perlin noise has the issue of not covering full range very well.
    // density = (density - 0.2) * (1.0 / 0.6) - 0.05;
    // density = clamp(density, 0.0, 1.0);


    // colorizing :)

    // vec4 u_dark_color = hex_color_rgb(0xc71b04);
    // vec4 u_dark_primary_color = hex_color_rgb(0xe02500);
    // vec4 u_bright_primary_color = hex_color_rgb(0xf28211);
    // vec4 u_brighter_color = hex_color_rgb(0xff9d2e);
    // vec4 u_white_color = hex_color_rgb(0xffe1a6);

    // TODO (maddie): we could also try using proper color mix functions these are just lerps.

    float approx_max = 0.65;
    vec4 background = mix(u_bright_primary_color, u_dark_primary_color, 0.95 - density);

    const float bright_threshold = 0.4;
    const float white_threshold = 0.55;
    const float dark_threshold = 0.3;
    if (density > bright_threshold) {
        float t0 = (density - bright_threshold) / (approx_max - bright_threshold);
        if (density > white_threshold) {
            float t = (density - white_threshold) / (approx_max - white_threshold);
            finalColor = mix(background, mix(u_brighter_color, u_white_color, t), t0);
        } else {
            finalColor = mix(background, u_brighter_color, t0);
        }
    } else if (density < dark_threshold) {
        float t = 1.0 - density / dark_threshold;
        finalColor = mix(background, u_dark_color, t);
    } else {
        finalColor = background;
    }

    // // finalColor = mix(orange, red, density);

    // const float bright_threshold = 0.75;
    // const float white_threshold = 0.9;
    // const float dark_threshold = 0.35;
    // if (density > bright_threshold) {
    //     if (density > white_threshold) {
    //         float t = (density - white_threshold) / (1.0 - white_threshold);
    //         finalColor = mix(yellow, white, t * t * t);
    //     } else {
    //         float t = (density - bright_threshold) / (1.0 - bright_threshold);
    //         finalColor = mix(background, yellow, t);
    //     }
    // } else if (density < dark_threshold) {
    //     float t = 1.0 - density / dark_threshold;
    //     t = clamp(t * t * t, 0.0, 1.0);
    //     finalColor = mix(background, dark_red, t);
    // } else {
    //     finalColor = background;
    // }



    // finalColor = mix(orange, red, density);


    // if (density < 0.35) {
    //     float t = density * (1.0 / 0.35);
    //     finalColor = mix(orange, yellow, t);
    // } else {
    //     float t = (density - 0.35) * (1.0 / 0.35);
    //     // t = pow(t, 2.0);
    //     finalColor = mix(yellow, red, t);
    // }


    // if (density < 0.5) {
    //     float t = density * 2.0;
    //     finalColor = mix(orange, yellow, t);
    // } else {
    //     float t = (density - 0.5) * 2.0;
    //     finalColor = mix(yellow, red, t);
    // }

    // float r=1.0;
    // if (density < 0.5) {
    //     // TODO (maddie): we still seem not to reach lower values?
    //     float t = pow(density, r);
    //     finalColor = mix(orange, yellow, t * 4.0);
    // } else {
    //     float t = (density - 0.5) * 2.0;
    //     t = pow(t, 1.0 / r);
    //     finalColor = mix(yellow, red, t);
    //     // finalColor = vec4(1.0);
    // }


    // if (density < 0.5) {
    //     finalColor = mix(color1, color2, density * 2.0);
    // } else {
    //     finalColor = mix(color2, color0, (density - 0.5) * 2.0);
    // }
}
