#version 330

// raylib standard attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;

// raylib standard uniforms
uniform mat4 mvp;

out vec2 fragTexCoord;
out vec3 fragPosition;

void main() {
    gl_Position = mvp * vec4(vertexPosition, 1.0);
    fragPosition = vertexPosition;
    fragTexCoord = vertexTexCoord;
}
