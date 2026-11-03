#version 330

in vec3 fragPosition;
in float fragZ;

out vec4 finalColor;

uniform vec3 u_center;
uniform float u_thickness;
uniform float u_semi_major_axis;
uniform float u_semi_minor_axis;
uniform float u_planar_forward_dot;
uniform float u_znear;
uniform float u_zfar;
// uniform float u_eccentricity;

float linear_depth(float depth) {
    float z = depth * 2.0 - 1.0; // Convert back to NDC  (0 to 1)
    return (2.0 * u_znear * u_zfar) / (u_zfar + u_znear - z * (u_zfar - u_znear));
}

void main() {

    float ratio = u_semi_major_axis / u_semi_minor_axis;

    vec2 d = fragPosition.xz - u_center.xz;
    d = vec2(d.x, d.y * ratio);

    float dist = length(d);

    float local_dist = abs(dist - u_semi_major_axis);

    // Adjust the thickness of the linears based on camera angle and distance from camera
    // to help avoid having the lines disspear farther away when the camera is tilted down.
    float depth = (linear_depth(gl_FragCoord.z) / u_zfar) + 0.2;
    depth = pow(depth, 0.8);
    float impact = max(0.0, 1.0 - u_planar_forward_dot);
    impact = min(impact, 0.5);
    float added_thickness = depth * 3.0 * impact;
    // added_thickness = pow(added_thickness, 0.5);
    float radius = u_thickness + added_thickness;

    float t = local_dist / radius;
    t = 1.0 - t * t;
    vec3 c = mix(vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), depth);
    finalColor = vec4(c, max(0.0, t - 0.3));
}
