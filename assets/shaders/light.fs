#version 330

in vec3 fragPosition;
in vec3 fragNormal;

out vec4 finalColor;

void main() {

    vec4 albedo = vec4(1.0, 1.0, 1.0, 1.0);

    vec3 sunDirection = vec3(-0.5, -1, -0.5);

    float diffuse = max(dot(fragNormal, normalize(-sunDirection)), 0.0);
    float ambient = 0.3;

    diffuse *= (1.0 - ambient);

    vec3 color = albedo.rgb * (ambient + diffuse);
    finalColor = vec4(color, albedo.a);
}
