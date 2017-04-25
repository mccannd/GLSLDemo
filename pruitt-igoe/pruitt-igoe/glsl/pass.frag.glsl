#version 150
#define EPS 0.000005

uniform float u_time;
uniform mat3 u_cam_proj;
uniform vec3 u_cam_pos;
uniform int res_width;
uniform int res_height;

uniform sampler2D texture0;

in vec2 fuv;
out vec4 out_Col;

float rhythm(float time) {
	//return 1.0;
	float r = smoothstep(-1, 1, sin(6.263 * time));
	r = smoothstep(0, 1, r);
	return 0.8 + 0.2 * smoothstep(0, 1, r);
	
}

float bias(in float t, in float b) {
	return (t / ((((1.0 / b) - 2.0) * (1.0 - t)) + 1.0));
}

float gain(in float t, in float g)
{
	if (t < 0.5) return bias(t * 2.0, g) / 2.0;
	else return bias(t * 2.0 - 1.0, 1.0 - g) / 2.0 + 0.5;
}

float atan2(in float a, in float b) {
	float angle = 0.3183 * atan(abs(b), a);
	// correct for bad glsl arctangent
	angle = 0.5 * mod((b < 0 ? (2.0 - angle) : angle), 2.0);
	return angle;
}

// Typical IQ Palette
vec3 palette(in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d)
{
	return a + b*cos(6.28318*(c*t + d));
}

float getBump(in vec3 pos) {
	vec2 v1 = pos.xz;
	vec2 v2 = pos.zy;
	vec2 v3 = pos.yx;
	float s1 = texture2D(texture0, 0.5 * v1 + 0.5).x;
	float s2 = texture2D(texture0, 0.5 * v2 + 0.5).y;
	float s3 = texture2D(texture0, 0.5 * v3 + 0.5).z;
	return bias(gain((s1 + s2 + s3) / 2.6, 0.33), 0.33);
}
float getBump2(in vec3 pos, in float w) {
	vec2 v1 = mod(pos.xz + vec2(w * 0.10 * u_time), vec2(1.0));
	vec2 v2 = mod(pos.zy + vec2(w * 0.08 * u_time), vec2(1.0));
	vec2 v3 = mod(pos.yx + vec2(w * 0.12 * u_time), vec2(1.0));
	float s1 = texture2D(texture0, v1).x;
	float s2 = texture2D(texture0, v2).y;
	float s3 = texture2D(texture0, v3).z;
	return bias(gain((s1 + s2 + s3) / 2.6, 0.33), 0.33);
}

float getPlanetRadius(in vec3 pos) {

	float ech = sin(0.25 * 3.1416 * (u_time + 16.0 * pos.y)) * 0.2 + 0.3;
	return 0.98 - ech;
}

float map(in vec3 pos) {
	float bump = getBump(pos);
	float r = rhythm(u_time);
	float height = clamp(r * bump,  0.4, 1.0);
	float r2 = getPlanetRadius(pos);
	return length(pos) - r2 - (0.98 - r2) * height + 0.05;
}


vec3 getNormal(in vec3 pos) {
	vec2 d = vec2(EPS, 0.0);
	return normalize(vec3(map(pos + d.xyy) - map(pos - d.xyy),
		map(pos + d.yxy) - map(pos - d.yxy),
		map(pos + d.yyx) - map(pos - d.yyx)));
}

float sphereMarch(in vec3 ro, in vec3 rd) {
	float t = 0.0;
	for (int i = 0; i < 120; i++) {
		vec3 pos = t * rd + ro;
		float h = map(pos);
		if (h < 0.001) return t;
		t += max(h, 0.002);
	}
	return -1.0;
}

float rayMarch(in vec3 ro, in vec3 rd) {
	float t = 0.0;

	for (int i = 1; i < 120; i++) {
		vec3 pos = t * rd + ro;
		float h = map(pos);

		if (h < 0.001) return t;

		t += min(h, 0.008);
	}
	return -1.0;
}

vec4 marchVolume(in vec3 ro, in vec3 rd, in float maxDist, in vec3 background) {
	
	vec4 sum = vec4(0.5, 0.5, 0.5, 0.0);
	vec3 sunPos = vec3(10.0, 7.0, 10.0);


	float t = 0.0;
	float st = sin(0.5 * u_time);
	float ct = cos(0.5 * u_time);

	for (int i = 1; i < 80; i++) {
		vec3 pos = ro + t * rd;
		vec3 p2 = vec3(ct * pos.x + st * pos.y, ct * pos.y - st*pos.x, pos.z) + vec3(1.0);


		for (int j = 1; j < 3; j++) {
			p2 *= 0.5;
			float w = 0.5 * 1.0 / float(j);
			vec3 pos2 = p2 - w * 0.16 * normalize(vec3(10.0, 7.0, 10.0) - p2);
			float dense = getBump2(p2, w);
			dense *= dense;
			float dense2 = getBump2(pos2, w);
			dense2 *= dense2;
			dense2 = clamp(1.67 * (dense - dense2), -0.7, 0.7);
			vec3 col = vec3(dense2 * 0.5);

			float co = clamp(1.0 - 6.67 * abs(length(pos) - 0.85), 0.0, 1.0);
			//float prad = getPlanetRadius(pos);
			//co = clamp(prad - 10.0 * prad * abs(length(pos) - prad + 0.1 * prad), 0.0, 1.0);
			co = smoothstep(0.0, 1.0, co);
			//float op = dense > 0.5? co * 2.0 * (dense + 0.1): 0.0;
			float op = pow(dense * co * 1.0, 2.5 - co);
			op = clamp(op, 0.0, 1.0);
			op = 2.0 * op * op;
			sum.w += w*op;
			sum.w = clamp(sum.w, 0.0, 1.0);
			//col = mix(col, background, 1.0 - exp(-0.003*t*t));
			sum.xyz = sum.xyz + (1.0 - sum.w)*op* col;

			
		}

		if (sum.w > 0.99) break;
		if (t > maxDist) break; 

		t += max(0.005, 0.02 * t);
	}
	vec3 mid = ro + (t * 0.5 + 0.05) * rd;
	float alignment = dot(normalize(sunPos - mid), normalize(mid));
	vec3 spherelit = mix(vec3(0.2, 0.3, 0.5), vec3(1.3, 1.2, 1.1), alignment * 0.5 + 0.5);
	return vec4(spherelit * sum.xyz, sum.w);
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

	// make tunnel effect
	vec2 centerOffset = vec2(0.2 *  sin(0.75 * u_time), 0.2 *  cos(1.25 * u_time));
	vec2 tuv = vec2(aspect, 1.0) * fuv + centerOffset;

	float angle = 0.3183 * atan(abs(tuv.y), tuv.x);
	// correct for bad glsl arctangent
	angle = 0.5 * mod((tuv.y < 0 ? (2.0 - angle) : angle) + 0.25 * u_time, 2.0);
	
	float dist = mod(length(tuv) - 1.0 * u_time, 1.0);
	vec3 d2 = vec3(clamp(length(tuv) - 0.2, 0.0, 1.0));
	out_Col = pow(texture2D(texture0, vec2(angle, dist)).xxxw * d2.xxxz, vec4(3.0, 3.0, 3.0, 1.0));

	vec3 ro = u_cam_pos;
	vec3 F = u_cam_proj[0];
	vec3 R = u_cam_proj[1];
	vec3 U = u_cam_proj[2];

	vec3 ref = u_cam_pos + 0.1 * F;
    float len = 0.1;
    vec3 p = ref + aspect * fuv.x * len * 1.0 * R + fuv.y * len * 1.0 * U;	
    vec3 rd = normalize(p - u_cam_pos);


	float sun = dot(normalize(vec3(10.0, 7.0, 10.0) - ro), rd);
	sun = clamp(sun, 0.0, 1.0);
	sun = sun * sun;
	sun = sun * sun;
	sun = sun * sun * sun;
	out_Col.xyz = mix(out_Col.xyz, vec3(1.0, 0.8, 0.6), sun);
	out_Col.xyz += (sun * sun * sun) * vec3(1.0, 0.8, 0.6);
	// raytrace the bounding sphere
	float t = raySphere(ro, rd, vec4(0, 0, 0, 1));
	if (t > 0.0) {

		// raymarch the planet
		vec3 pos = ro + t * rd;
		float tM = rayMarch(pos, rd);

		if (tM > 0.0) {
			// shade this planet
			pos += tM * rd;
			vec3 nor = getNormal(pos);
			float bump = getBump(pos);
			
			vec3 light_col = vec3(1.0, 0.8, 0.6);
			vec3 light_amb = vec3(0.05, 0.1, 0.2) * (1.0 - abs(dot(nor, F)));
			vec3 light_disp = vec3(10.0, 7.0, 10.0) - pos;
			vec3 light_dir = normalize(light_disp);
			float light_dist = length(light_disp);

			float facing = clamp(dot(light_dir, nor), 0.0, 1.0);
			float intensity = 200.0 / (light_dist * light_dist);
			float rhythm = rhythm(u_time);

			vec3 mat; 
			
			float spec = abs(dot(normalize(light_dir - rd), nor));
			if (rhythm * bump > 0.4) {
				mat = palette(rhythm * bump, vec3(0.298, 0.498, -0.102), vec3(0.268, 0.298, 0.798),
					vec3(0.638, 0.508, 0.338), vec3(0.308, 1.308, 1.578));
				spec *= 0.0001 * spec;
			}
			else {
				mat = mix(vec3(0.05, 0.1, 0.3), vec3(0.1, 0.3, 0.4), 
					clamp(dot(reflect(rd, nor), light_dir) - 0.5 * bump, 0.0, 1.0));
				spec = 0.5 * pow(spec, 64.0 * (1.0 - bump));
			}
			vec3 col = intensity * facing * light_col + light_amb;
			col *= mat;
			col += vec3(spec);
			vec3 radial = normalize(pos);

			// atmospheric glow
			float atmo = 1.0 - clamp(dot(radial, -F), 0.0, 1.0);
			atmo = pow(atmo, 1.4);
			vec3 atmoCol = vec3(0.05, 0.1, 0.25);

			atmoCol = mix(atmoCol, vec3(1.0, 0.9, 0.7), sun);
			atmoCol = mix(atmoCol, vec3(0.3, 0.6, 0.8), clamp(dot(radial, light_dir), 0.0, 1.0));

			col = mix(col, atmoCol, atmo);
			col = clamp(col, vec3(0.0), vec3(1.0));

			out_Col = vec4(pow(col, vec3(0.4545)), 1.0);
			
		}
		//out_Col.xyz = clamp(out_Col.xyz, vec3(0.0), vec3(1.0));
		vec4 cloud = marchVolume(ro + t * rd, rd, tM > 0.0 ? tM : 2.0, out_Col.xyz);
		
		//out_Col = vec4(mix(out_Col.xyz, cloud.xyz, 0.5), 1.0);
		out_Col = vec4(mix(out_Col.xyz, clamp(cloud.xyz, vec3(0.0), vec3(1.0)), cloud.w), 1.0);
		//out_Col = vec4(vec3(cloud.w), 1.0);
		//out_Col.xyz += cloud.w * cloud.xyz;
	}

	// vignetting effect
	out_Col *= vec4(vec3(1.0 - clamp(length(0.75 * fuv) - 0.2, 0.0, 1.0)), 1.0);

}