#version 330

// raylib standard attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// raylib standard uniforms
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

out vec2 fragTexCoord;
out vec3 fragPosition;

void main() {
    gl_Position = mvp * vec4(vertexPosition, 1.0);
    fragPosition = vec3(matNormal * vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
}
