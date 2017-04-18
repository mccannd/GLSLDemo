#version 150
#define EPS 0.00001

uniform float u_time;
uniform mat3 u_cam_proj;
uniform vec3 u_cam_pos;
uniform int res_width;
uniform int res_height;

uniform sampler2D texture0;

in vec2 fuv;
out vec4 out_Col;

float rhythm(float time) {
	float r = smoothstep(-1, 1, sin(6.263 * time));
	r = smoothstep(0, 1, r);
	return smoothstep(0, 1, r);;
}

float bias(in float t, in float b) {
	return (t / ((((1.0 / b) - 2.0) * (1.0 - t)) + 1.0));
}

float gain(in float t, in float g)
{
	if (t < 0.5) return bias(t * 2.0, g) / 2.0;
	else return bias(t * 2.0 - 1.0, 1.0 - g) / 2.0 + 0.5;
}

float map(in vec3 pos) {
	float s1 = texture2D(texture0, 0.5 * pos.xz + 0.5).x;
	float s2 = texture2D(texture0, 0.5 * pos.zy + 0.5).y;
	float s3 = texture2D(texture0, 0.5 * pos.xy + 0.5).z;
	float bump = bias(gain((s1 + s2 + s3) / 2.6, 0.33), 0.33);
	float r = rhythm(u_time);
	float height = clamp(r * bump,  0.4, 1.0);
	return length(pos) - 0.9 - 0.1 * height;
	//return max(length(pos) - 1.0, bump);
}


vec3 getNormal(in vec3 pos) {
	vec2 d = vec2(EPS, 0.0);
	return normalize(vec3(map(pos + d.xyy) - map(pos - d.xyy),
		map(pos + d.yxy) - map(pos - d.yxy),
		map(pos + d.yyx) - map(pos - d.yyx)));
}

float sphereMarch(in vec3 ro, in vec3 rd) {
	float t = 0.0;
	for (int i = 0; i < 100; i++) {
		vec3 pos = t * rd + ro;
		float h = map(pos);
		if (h < 0.001) return t;
		t += max(h, 0.002);
	}
	return -1.0;
}

float rayMarch(in vec3 ro, in vec3 rd) {
	float t = 0.0;
	for (int i = 0; i < 120; i++) {
		vec3 pos = t * rd + ro;
		float h = map(pos);
		if (h < 0.001) return t;
		t += min(h, 0.004);
	}
	return -1.0;
}

float raySphere(in vec3 ro, in vec3 rd, in vec4 sph) {
	float r = sph.w;
	vec3 disp = ro - sph.xyz;
	float b = dot(disp, rd);
	float c = dot(disp, disp) - r*r;
	float h = b*b - c;
	if(h>0.0) h = -b -sqrt(h);
	return h;
}


void main() {
    vec2 uv = 0.5 * fuv + vec2(0.5, 0.5);
	float aspect = float(res_width) / float(res_height);
	// for drawing a texture at its native resolution

	vec2 centerOffset = vec2(0.4 *  sin(0.75 * u_time), 0.4 *  cos(1.25 * u_time));

	vec2 tuv = vec2(aspect, 1.0) * fuv + centerOffset;
	float angle = 0.3183 * atan(abs(tuv.y), tuv.x);
	angle = 0.5 * mod((tuv.y < 0 ? (2.0 - angle) : angle) + 0.25 * u_time, 2.0);
	float dist = mod(length(tuv) + u_time, 1.0);

	vec2 texUV = vec2(float(res_width) / 1024.0, float(res_height) / 1024.0) * uv;

	float st = sin(u_time);
	float ct = cos(u_time);

	out_Col = vec4(uv, 0.5 + 0.5 * st, 1);
	vec3 ro = u_cam_pos;
	vec3 F = u_cam_proj[0];
	vec3 R = u_cam_proj[1];
	vec3 U = u_cam_proj[2];

	vec3 ref = u_cam_pos + 0.1 * F;
    float len = 0.1;
    vec3 p = ref + aspect * fuv.x * len * 1.619 * R + fuv.y * len * 1.619 * U;
	
    vec3 rd = normalize(p - u_cam_pos);


    out_Col = texture2D(texture0, vec2(angle, dist)).xxxw;
	//out_Col = vec4(angle, angle, angle, 1.0);

	//float t = sphereMarch(ro, rd);
	float t = raySphere(ro, rd, vec4(0, 0, 0, 1));


	if (t > 0.0) {


		vec3 pos = ro + t * rd;

		float tM = sphereMarch(pos, rd);

		if (tM > 0.0) {
			pos += tM * rd;
			vec3 nor = getNormal(pos);

			vec3 mat = vec3(0.2);

			vec3 light_col = vec3(1.0, 0.8, 0.6);
			vec3 light_amb = vec3(0.05, 0.1, 0.2) * (1.0 - abs(dot(nor, F)));
			vec3 light_disp = vec3(10.0, 7.0, 10.0) - pos;
			vec3 light_dir = normalize(light_disp);
			float light_dist = length(light_disp);

			float facing = clamp(dot(light_dir, nor), 0.0, 1.0);
			float intensity = 100.0 / (light_dist * light_dist);
			vec3 col = intensity * facing * light_col + light_amb;

			out_Col = vec4(pow(col, vec3(0.4545)), 1.0);
		}

	}
}