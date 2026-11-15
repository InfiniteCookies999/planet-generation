#version 330

// raylib standard attributes
in vec3 vertexPosition;

// raylib standard uniforms
uniform mat4 mvp;
uniform mat4 matNormal;
uniform mat4 matModel;

out vec3 fragPosition;
out vec3 fragWorldPosition;

void main() {
    gl_Position = mvp * vec4(vertexPosition, 1.0);
    fragPosition = vertexPosition;
    fragWorldPosition = vec3(matModel * vec4(vertexPosition, 1.0));
}
