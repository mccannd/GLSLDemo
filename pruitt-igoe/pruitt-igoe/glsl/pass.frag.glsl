#version 150

uniform float u_time;
uniform mat3 u_cam_proj;
uniform vec3 u_cam_pos;
uniform int res_width;
uniform int res_height;

in vec2 fuv;
out vec4 out_Col;

float map(in vec3 pos) {
	return length(pos) - 1.0;
}

float march(in vec3 ro, in vec3 rd) {
	float t = 0.0;
	for (int i = 0; i < 100; i++) {
		vec3 pos = t * rd + ro;
		float h = map(pos);
		if (h < 0.001) return t;
		t += max(h, 0.002);
	}
	return -1.0;
}

void main() {
    vec2 uv = 0.5 * fuv + vec2(0.5, 0.5);
	out_Col = vec4(uv, 0.5 + 0.5 * sin(u_time), 1);
	vec3 ro = u_cam_pos;
	vec3 F = u_cam_proj[0];
	vec3 R = u_cam_proj[1];
	vec3 U = u_cam_proj[2];

	float aspect = float(res_width) / float(res_height);

	vec3 ref = u_cam_pos + 0.1 * F;
    float len = 0.1;
    vec3 p = ref + aspect * fuv.x * len * 1.619 * R + fuv.y * len * 1.619 * U;
	
    vec3 rd = normalize(p - u_cam_pos);
    out_Col = vec4(rd, 1.0);
	float t = march(ro, rd);
	if (t > 0.0) {
		out_Col = vec4(1.0);
	}
}