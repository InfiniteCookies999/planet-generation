#version 330

in vec3 fragNormal;

out vec4 finalColor;

uniform vec3 u_direction_to_star;

void main() {

    vec4 albedo = vec4(1.0, 1.0, 1.0, 1.0);

    float diffuse = max(dot(fragNormal, normalize(u_direction_to_star)), 0.0);
    float ambient = 0.3;

    diffuse *= (1.0 - ambient);

    vec3 color = albedo.rgb * (ambient + diffuse);
    finalColor = vec4(color, albedo.a);
}
