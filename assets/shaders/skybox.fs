#version 330

in vec3 fragPosition;

// we get no warning if we mispell this. thanks raylib.

// raylib standard uniforms
uniform samplerCube environmentMap;


out vec4 finalColor;

void main() {
    vec3 color = texture(environmentMap, fragPosition).rgb;
    finalColor = vec4(color, 1.0);
}