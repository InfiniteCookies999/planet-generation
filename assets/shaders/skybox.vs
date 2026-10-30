#version 330

// raylib standard attributes
in vec3 vertexPosition;

// raylib standard uniforms
uniform mat4 matView;
uniform mat4 matProjection;

out vec3 fragPosition;

void main() {

    // remove translation from the view matrix.
    mat4 rotView = mat4(mat3(matView));
    gl_Position = matProjection * rotView * vec4(vertexPosition, 1.0);
    
    fragPosition = vertexPosition;
}
