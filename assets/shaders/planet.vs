#version 330

// raylib standard attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

// raylib standard uniforms
uniform mat4 mvp;
uniform mat4 matNormal;

out vec3 fragNormal;

void main() {
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
