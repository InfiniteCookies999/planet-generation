#version 330

// raylib standard attributes
in vec2 fragTexCoord;

out vec4 finalColor;


const int PERLIN_GRID_CELL_COUNT = 64;
const int PERLIN_GRID_CELL_MASK = PERLIN_GRID_CELL_COUNT - 1;
const int PERLIN_GRID_VECTOR_WIDTH_COUNT = PERLIN_GRID_CELL_COUNT + 1;
const int PERLIN_GRID_SIZE = PERLIN_GRID_VECTOR_WIDTH_COUNT * PERLIN_GRID_VECTOR_WIDTH_COUNT;


// raylib standard uniforms
uniform sampler2D texture0;

uniform int u_tick;

float perlin_grad(int hash, float x, float y) {
    if      (hash == 0) return x;  // dot product with ( 1,  0)
    else if (hash == 1) return -x; // dot product with (-1,  0)
    else if (hash == 2) return y;  // dot product with ( 0,  1)
    else                return -y; // dot product with ( 0, -1)
}

float fade(float t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

int sample_perlin_grad_hashes(int i) {
    float r = texelFetch(texture0, ivec2(i, 0), 0).r;
    return int(r * 255.0);
}

float perlin_noise_sample_x_wrapped(float x, float y, int x_cell_wrap) {

    // Find the cell that the point x,y lies in.
    int cell_x = int(floor(x)) % (x_cell_wrap + 1);
    int cell_y = int(floor(y)) & PERLIN_GRID_CELL_MASK;


    int cell_x_right;
    if (int(floor(x)) == x_cell_wrap) {
        // want to select gradiant vectors from start/end of grid on the x axis.
        // The right vectors end up being at the start of the grid and the left
        // vectors at the end.
        cell_x_right = 0;
    } else {
        cell_x_right = cell_x + 1;
    }

    int gbl = sample_perlin_grad_hashes(cell_y*PERLIN_GRID_VECTOR_WIDTH_COUNT + cell_x);
    int gbr = sample_perlin_grad_hashes(cell_y*PERLIN_GRID_VECTOR_WIDTH_COUNT + cell_x_right);
    int gtl = sample_perlin_grad_hashes((cell_y+1)*PERLIN_GRID_VECTOR_WIDTH_COUNT + cell_x);
    int gtr = sample_perlin_grad_hashes((cell_y+1)*PERLIN_GRID_VECTOR_WIDTH_COUNT + cell_x_right);

    // Distance of x/y in cell.
    float dx = x - floor(x);
    float dy = y - floor(y);

    // Computing dot product of distance vectors against gradient vectors.
    float bl = perlin_grad(gbl, dx      , dy      );
    float br = perlin_grad(gbr, dx - 1.0, dy      );
    float tl = perlin_grad(gtl, dx      , dy - 1.0);
    float tr = perlin_grad(gtr, dx - 1.0, dy - 1.0);

    // Interpolate the contribution of each vector along the X and Y axis.

    // Use of fade here makes it so when the sample points are close to the edges
    // of the cells it smoothly fades between them instead of having a harsh cutoff.
    float tx = fade(dx);
    float ty = fade(dy);
    return mix(mix(bl, br, tx), mix(tl, tr, tx), ty);
}

float perlin_noise_sample_wrapped(float x, float y, int cell_wrap) {

    // Find the cell that the point x,y lies in.
    int cell_x = int(floor(x)) % (cell_wrap + 1);
    int cell_y = int(floor(y)) % (cell_wrap + 1);


    int cell_x_right;
    int cell_y_top;
    if (int(floor(x)) == cell_wrap) {
        // want to select gradiant vectors from start/end of grid on the x axis.
        // The right vectors end up being at the start of the grid and the left
        // vectors at the end.
        cell_x_right = 0;
    } else {
        cell_x_right = cell_x + 1;
    }
    if (int(floor(y)) == cell_wrap) {
        // Same explanation as for x.
        cell_y_top = 0;
    } else {
        cell_y_top = cell_y + 1;
    }

    int gbl = sample_perlin_grad_hashes(cell_y*PERLIN_GRID_VECTOR_WIDTH_COUNT + cell_x);
    int gbr = sample_perlin_grad_hashes(cell_y*PERLIN_GRID_VECTOR_WIDTH_COUNT + cell_x_right);
    int gtl = sample_perlin_grad_hashes(cell_y_top*PERLIN_GRID_VECTOR_WIDTH_COUNT + cell_x);
    int gtr = sample_perlin_grad_hashes(cell_y_top*PERLIN_GRID_VECTOR_WIDTH_COUNT + cell_x_right);

    // Distance of x/y in cell.
    float dx = x - floor(x);
    float dy = y - floor(y);

    // Computing dot product of distance vectors against gradient vectors.
    float bl = perlin_grad(gbl, dx      , dy      );
    float br = perlin_grad(gbr, dx - 1.0, dy      );
    float tl = perlin_grad(gtl, dx      , dy - 1.0);
    float tr = perlin_grad(gtr, dx - 1.0, dy - 1.0);

    // Interpolate the contribution of each vector along the X and Y axis.

    // Use of fade here makes it so when the sample points are close to the edges
    // of the cells it smoothly fades between them instead of having a harsh cutoff.
    float tx = fade(dx);
    float ty = fade(dy);
    return mix(mix(bl, br, tx), mix(tl, tr, tx), ty);
}

float perlin_noise_sample2(float x, float y) {

    // Find the cell that the point x,y lies in.
    int cell_x = int(floor(x)) & PERLIN_GRID_CELL_MASK;
    int cell_y = int(floor(y)) & PERLIN_GRID_CELL_MASK;

    // Sample the grid of gradient hashes.
    int gbl = sample_perlin_grad_hashes(cell_y*PERLIN_GRID_VECTOR_WIDTH_COUNT + cell_x);
    int gbr = sample_perlin_grad_hashes(cell_y*PERLIN_GRID_VECTOR_WIDTH_COUNT + (cell_x+1));
    int gtl = sample_perlin_grad_hashes((cell_y+1)*PERLIN_GRID_VECTOR_WIDTH_COUNT + cell_x);
    int gtr = sample_perlin_grad_hashes((cell_y+1)*PERLIN_GRID_VECTOR_WIDTH_COUNT + (cell_x+1));

    // Distance of x/y in cell.
    float dx = x - floor(x);
    float dy = y - floor(y);

    // Computing dot product of distance vectors against gradient vectors.
    float bl = perlin_grad(gbl, dx      , dy      );
    float br = perlin_grad(gbr, dx - 1.0, dy      );
    float tl = perlin_grad(gtl, dx      , dy - 1.0);
    float tr = perlin_grad(gtr, dx - 1.0, dy - 1.0);

    // Interpolate the contribution of each vector along the X and Y axis.

    // Use of fade here makes it so when the sample points are close to the edges
    // of the cells it smoothly fades between them instead of having a harsh cutoff.
    float tx = fade(dx);
    float ty = fade(dy);
    return mix(mix(bl, br, tx), mix(tl, tr, tx), ty);
}

float perlin_noise_sample(float x, float y, float scale) {

    // Code to convert to convert to polar coordinates. Fixes longitude discontinuity
    // but introduces pole distortion particularly on the bottom pole.
    //
    // The distortion comes from the fact that when y approaches zero all the noise
    // approaches the same location.
    // const float PI = 3.14159265358979323846;
    // float angle = x * (2 * PI);   // 0 to 1 -> 0 to PI
    // float r = y * scale;
    // float x0 = r * cos(angle);
    // float y0 = r * sin(angle);

    // // Must shift the x,y so the values stay in the positive range of the
    // // perlin grid.
    // return perlin_noise_sample2(x0 + scale, y0 + scale);



    // This is an attempt at donut shaping the result but does not seem to be working.
    // const float hole = 0.8;
    // const float PI = 3.14159265358979323846;
    // float angle = x * (2 * PI);   // 0 to 1 -> 0 to PI
    // float r = (y + hole) * scale;
    // float x0 = r * cos(angle);
    // float y0 = r * sin(angle);

    // // Must shift the x,y so the values stay in the positive range of the
    // // perlin grid.
    // float shift = scale + hole*scale;
    // return perlin_noise_sample2(x0 + shift, y0 + shift);


    // This is essentially working except that at the part where they meet there is still a seem.
    float sx = x * scale;
    float sy = y * scale;
    const float C = 0.2;
    if (x <= C) {
        float a = (C - x) / C; // x=0.1 -> 0.1-x=0 -> 0
                               // x=0   -> 0.1-x=0.1 -> 0.1/0.1=1

        float sample0 = perlin_noise_sample2(sx, sy);                       // when a=1 -> sx=0
        float sample1 = perlin_noise_sample2(scale - C*(1 - a)*scale, sy);  // when a=1 -> sx=1

        // Divide by 2 so that its 50% where they meet.
        float t = smoothstep(0.0, 1.0, a * 0.5);
        return mix(sample0, sample1, t);
    } else if (x >= 1 - C) {
        float a = (x - (1 - C)) / C; // x=0.9 -> 0.9-0.9=0 -> 0
                                     // x=1   -> 1-0.9=0.1 -> 0.1/0.1=1

        float sample0 = perlin_noise_sample2(sx, sy);                     // when a=1 -> sx=1
        float sample1 = perlin_noise_sample2(C*scale - C*a*scale, sy);    // when a=1 -> sx=0

        // Divide by 2 so that its 50% where they meet.
        float t = smoothstep(0.0, 1.0, a * 0.5);
        return mix(sample0, sample1, t);
    } else {
        return perlin_noise_sample2(sx, sy);
    }


    // if (x <= 0.1) {
    //     float a = (0.1 - x) / 0.1; // x=0.1 -> 0.1-x=0 -> 0
    //                                // x=0   -> 0.1-x=0.1 -> 0.1/0.1=1

    //     float sample0 = perlin_noise_sample2(sx, sy);                         // when a=1 -> sx=0
    //     float sample1 = perlin_noise_sample2(scale - 0.1*(1 - a)*scale, sy);  // when a=1 -> sx=1

    //     // Divide by 2 so that its 50% where they meet.
    //     float t = smoothstep(0.0, 1.0, a * 0.5);
    //     return mix(sample0, sample1, t);
    // } else if (x >= 0.9) {
    //     float a = (x - 0.9) / 0.1; // x=0.9 -> 0.9-0.9=0 -> 0
    //                                // x=1   -> 1-0.9=0.1 -> 0.1/0.1=1

    //     float sample0 = perlin_noise_sample2(sx, sy);                         // when a=1 -> sx=1
    //     float sample1 = perlin_noise_sample2(0.1*scale - 0.1*a*scale, sy);    // when a=1 -> sx=0

    //     // Divide by 2 so that its 50% where they meet.
    //     float t = smoothstep(0.0, 1.0, a * 0.5);
    //     return mix(sample0, sample1, t);
    // } else {
    //     return perlin_noise_sample2(sx, sy);
    // }


    // Naive no wrapping causes discontinuity along longitude. But has
    // the advantage of not having nearly as much distortion at the poles.
    // return perlin_noise_sample2(x * scale, y * scale);
}

float perlin_noise_cell_wrapped_sample(float x, float y, int scale) {

    float sx = x * float(scale);
    float sy = y * float(scale);
    int cell_wrap = scale - 1;

    return perlin_noise_sample_wrapped(sx, sy, cell_wrap);
}

vec4 hex_color_rgb(int hex) {
    int r = (hex >> 16) & 0xFF;
    int g = (hex >> 8) & 0xFF;
    int b = hex & 0xFF;
    float fr = float(r) / 255.0;
    float fg = float(g) / 255.0;
    float fb = float(b) / 255.0;
    return vec4(fr, fg, fb, 1.0);
}

void main() {

    // if we only want to cover a single cell i think we just use the coordinates
    // directly.

    //float offset = sin(float(u_tick) * 0.01) * 0.25 + 0.25;
    // float tx = mod(fragTexCoord.x, 1.0) + offset;
    // float ty = fragTexCoord.y + offset;

    // float tx = mod(fragTexCoord.x + offset, 1.0);
    // float ty = mod(fragTexCoord.y + offset, 1.0);

    // this just makes it looks like it slowly moves
    float offset = float(u_tick) * 0.00015;
    float tx = mod(fragTexCoord.x + offset, 1.0);
    float ty = mod(fragTexCoord.y + offset, 1.0);

    // float density0 = perlin_noise_cell_wrapped_sample(tx, ty, 10);
    // float density1 = perlin_noise_cell_wrapped_sample(tx, ty, 24);
    // float density2 = perlin_noise_cell_wrapped_sample(tx, ty, 40);

    // float density = density0 + density1 * 0.5 + density2 * 0.4;

    // float density0 = perlin_noise_cell_wrapped_sample(tx, ty, 20);
    // float density1 = perlin_noise_cell_wrapped_sample(tx, ty, 60);

    float density0 = perlin_noise_cell_wrapped_sample(tx, ty, 40);
    float density1 = perlin_noise_cell_wrapped_sample(tx, ty, 80);

    // this makes it more or less look like it shrinks and grows.
    float amp0 = 1.0 + cos(u_tick * 0.015) * 0.1;
    float amp1 = 0.5 + (sin(u_tick * 0.015) + 1) * 0.15;

    float density = density0 * amp0 + density1 * amp1;


    density = (density + 1) * 0.5; // convert range to 0 to 1.

    // TODO (maddie): if this approximation is off thats an issue.
    // perlin noise has the issue of not covering full range very well.
    // 0.2 to 0.8 -> 0 to 1
    // -0.2
    // 0.0 to 0.6
    // multiply by 1/0.6
    density = (density - 0.2) * (1.0 / 0.6) - 0.05;
    density = clamp(density, 0.0, 1.0);


    // approximate range is probably like -1 to +1
    // if (density < 0.0) {
    //     finalColor = vec4(1.0, 0.0, 0.0, 1.0);
    // } else {
    //     finalColor = vec4(density, density, density, 1.0);
    // }

    // colorizing :)

    // vec4 red = hex_color_rgb(0xed570c); // darker red
    // //vec4 orange = hex_color_rgb(0xffc369); // orange

    // vec4 orange = hex_color_rgb(0xf0a54a);
    // //vec4 yellow = hex_color_rgb(0xfeffb3); // bright yellow
    // vec4 yellow = hex_color_rgb(0xffe482);
    // // vec4 yellow = hex_color_rgb(0xf5ae51);


    vec4 dark_red = hex_color_rgb(0xc71b04);
    vec4 red = hex_color_rgb(0xe02500);
    vec4 orange = hex_color_rgb(0xf28211);
    vec4 yellow = hex_color_rgb(0xff9d2e);
    vec4 white = hex_color_rgb(0xffe1a6);

    // finalColor = mix(color0, color1, density);

    // ** look into: logistic function

    // float l = 0.9;
    // vec4 bright_spots = vec4(0.0);//mix(vec4(0.0, 0.0, 0.0, 1.0), yellow, max(density - 0.95, 0.0));
    // if (density > l) {
    //     float t = (density - l) / (1.0 - l);
    //     bright_spots = mix(orange, yellow, t * t * t * t);
    // }

    // vec4 background = mix(orange, red, 1.0 - density);

    // if (bright_spots.a != 0.0) {
    //     finalColor = bright_spots;
    // } else {
    //     finalColor = background;
    // }


    // TODO (maddie): we could also try using proper color mix functions these are just lerps.
    vec4 background = mix(orange, red, 1.0 - density);

    const float bright_threshold = 0.75;
    const float white_threshold = 0.98;
    const float dark_threshold = 0.35;
    if (density > bright_threshold) {


        // the threshold seems too strict kinda implying our noise isnt getting enough proper range.
        if (density > white_threshold) {
            float t = (density - white_threshold) / (1.0 - white_threshold);
            finalColor = mix(yellow, white, t*t*t);
            // finalColor = vec4(t, t, t, 1.0);
        } else {
            float t = (density - bright_threshold) / (1.0 - bright_threshold);
            finalColor = mix(background, yellow, t);
        }

        // float t = (density - bright_threshold) / (1.0 - bright_threshold);
        // finalColor = mix(background, yellow, t);
    } else if (density < dark_threshold) {
        float t = 1.0 - density / dark_threshold;
        t = clamp(t * t * t, 0.0, 1.0);
        finalColor = mix(background, dark_red, t);
    } else {
        finalColor = background;
    }



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
