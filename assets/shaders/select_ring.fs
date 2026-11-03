#version 330

in vec3 fragPosition;

out vec4 finalColor;

uniform vec3 u_center;
uniform float u_radius;

void main() {

    float dist = length(fragPosition - u_center);

    float extent = u_radius * 0.4;

    float t = ((dist - u_radius) / extent);
    t = 1.0 - t * t * t;
    finalColor = vec4(82.0 / 255.0, 220.0 / 255.0, 1.0, t);
}