#include "imgGenerator.h"

#define RES 1024
#define PI 3.14159f

float hashNoise(float x, float y) {

	float n = sin(dot(glm::vec2(x, y), glm::vec2(12.9898f, 78.233f))) * 43758.5453f;
	n = n - floor(n);
	//return 0.5f + 0.5f * n;
	//std::cout << n << '\n';
	return n;
}

glm::vec2 hashVector(float x, float y) {

	float r = hashNoise(x, y);
	r *= 8.0f;
	r = floor(r);
	return glm::vec2(cosf(r * PI * 0.25f), sinf(r * PI * 0.25f));
}

float mix(float a, float b, float t) {
	return (1.0f - t) * a + t * b;
}

float valueNoise(float x, float y, float freq) {
	x *= freq; y *= freq;
	glm::vec2 f = glm::vec2(floor(x), floor(y));
	glm::vec2 r = glm::vec2(x - f[0], y - f[1]);
	glm::vec2 u = r*r*r*(r*(r*6.0f - glm::vec2(15.0f)) + glm::vec2(10.0f));

	// force tiling boundaries
	glm::vec2 f1 = glm::vec2(f) + glm::vec2(1, 1);
	if (fabs(f1[0] - freq) < 0.001f) f1[0] = 0.0f;
	if (fabs(f1[1] - freq) < 0.001f) f1[1] = 0.0f;

	float a = hashNoise(f[0], f[1]);
	float b = hashNoise(f1[0], f[1]);
	float c = hashNoise(f[0], f1[1]);
	float d = hashNoise(f1[0], f1[1]);

	a = mix(a, b, u[0]);
	c = mix(c, d, u[0]);

	return mix(a, c, u[1]);
}


float perlinNoise(float x, float y, float freq, float shift) {
	x *= freq; y *= freq;
	glm::vec2 f = glm::vec2(floor(x), floor(y));
	glm::vec2 r = glm::vec2(x - f[0], y - f[1]);
	glm::vec2 u = r*r*r*(r*(r*6.0f - glm::vec2(15.0f)) + glm::vec2(10.0f));

	// force tiling boundaries
	glm::vec2 f1 = glm::vec2(f) + glm::vec2(1, 1);
	if (fabs(f1[0] - freq * (shift + 1.0)) < 0.001f) f1[0] = freq * shift;
	if (fabs(f1[1] - freq * (shift + 1.0)) < 0.001f) f1[1] = freq * shift;

	float a = glm::dot(hashVector(f[0], f[1]), r);
	float b = glm::dot(hashVector(f1[0], f[1]), -glm::vec2(1, 0) + r);
	float c = glm::dot(hashVector(f[0], f1[1]), -glm::vec2(0, 1) + r);
	float d = glm::dot(hashVector(f1[0], f1[1]), -glm::vec2(1, 1) + r);

	//std::cout << hashVector(f[0], f[1]).x << '\n';

	a = mix(a, b, u[0]);
	c = mix(c, d, u[0]);

	return mix(a, c, u[1]);
}


float fbm(float x, float y) {

	float freq = 16.0f;
	float ampl = 0.5f;
	float n = 0;
	for (int i = 0; i < 7; i++) {
		n += ampl * valueNoise(x, y, freq);
		freq *= 2.0f;
		ampl *= 0.5f;
	}
	return n;
}


float fbmPerlin(float x, float y, float shift) {

	float freq = 8.0f;//16.0f;
	float ampl = 0.5f;
	float n = 0;
	for (int i = 0; i < 7; i++) {
		n += ampl * perlinNoise(x, y, freq, shift);
		freq *= 2.0f;
		ampl *= 0.5f;
	}
	return 0.5 + 0.5*n;
}


GLuint* generateMap() {
	GLuint * map = new GLuint[RES * RES];

	for (int row = 0; row < RES; row++) {
		for (int col = 0; col < RES; col++) {
			GLuint color = row ^ col;
			GLuint color1 = color;
			GLuint color2 = color;

			color = (color << 0) | (color1 << 8) | (color2 << 16) | (255 << 24);
			map[row * RES + col] = color;
		}
	}

	return map;
}



GLuint* generateMapPerlin() {
	GLuint * map = new GLuint[RES * RES];
	GLuint maxColor = 0;
	GLuint minColor = 255;

	for (int row = 0; row < RES; row++) {
		for (int col = 0; col < RES; col++) {
			GLuint color = (int)(255 * fbmPerlin((float)row / RES, (float)col / RES, 0.0f));
			if (color > maxColor) maxColor = color;
			else if (color < minColor) minColor = color;

			map[row * RES + col] = color;
		}
	}

	float contrast = 255.0f / (maxColor - minColor);
	// stretch contrast
	for (int row = 0; row < RES; row++) {
		for (int col = 0; col < RES; col++) {
			GLuint color = map[row * RES + col];
			color -= minColor;
			color = (GLuint)(contrast * color);
			GLuint color1 = (int)(255 * fbmPerlin((float)row / RES + 1.0f, (float)col / RES + 1.0f, 1.0f));
			color1 -= minColor;
			color1 = (GLuint)(contrast * color1);
			//color1 = (color1 < 0 ? 0 : color1) > 255 ? 255 : color1;
			GLuint color2 = (int)(255 * fbmPerlin((float)row / RES + 2.0f, (float)col / RES + 2.0f, 2.0f));
			color2 -= minColor;
			color2 = (GLuint) (contrast * color2);
			//color2 = (color2 < 0 ? 0 : color2) > 255 ? 255 : color2;
			color = (color << 0) | (color1 << 8) | (color2 << 16) | (255 << 24);
			map[row * RES + col] = color;
		}
	}

	return map;
}

GLuint* generateStarfield(GLuint* bg) {
	GLuint* map = new GLuint[RES * RES];
	std::copy(bg, bg + RES * RES, map);
	
	// tone map this background
	for (int row = 0; row < RES; row++) {
		for (int col = 0; col < RES; col++) {
			GLuint color = map[row * RES + col];
			color = color & 0xFF;
			color = (GLuint)(0.2f * color) | ((GLuint)(0.16f * color) << 8) | 
				((GLuint)(0.4f * color) << 16) | (255 << 24);
			map[row * RES + col] = color;
		}

	}

	// stratify some stars
	

	for (int layers = 0; layers < 4; layers++) {
		for (int x = 0; x < 16; x++) {
			for (int y = 0; y < 16; y++) {
				float hx = hashNoise((float)(layers + 1) * x, (float)(layers + 1) * y);
				float hy = hashNoise((float)(layers + 2) * y, (float)(layers + 2) * x);
				
				int centerX = (int)((x + hx)/ 16.0f * RES);
				int centerY = (int)((y + hy) / 16.0f * RES);
				glm::vec3 rgb = glm::vec3(0.2f * hashNoise((float) x*x, (float)  y*y) + 0.8f,
					0.5f * hashNoise((float) x*y, (float) x*y) + 0.5f, 
					0.1f * hashNoise((float) y * y, (float) x * x) + 0.9f);

				int radius = (int)(5.0 * hashNoise((float)centerX, (float)centerY));
				centerX = (centerX - radius) < 0 ? centerX + RES : centerX;
				centerY = (centerY - radius) < 0 ? centerY + RES : centerY;
				for (int sX = centerX - radius; sX <= centerX + radius; sX++) {
					for (int sY = centerY - radius; sY <= centerY + radius; sY++) {
						float dist = sqrt((sY-centerY) * (sY - centerY) + (sX-centerX) * (sX-centerX));
						dist = std::max((radius - dist) / (float) radius, 0.0f);
						
						GLuint val = (GLuint)(dist * 255.0f * (layers + 1) / 4.0f);
						
						GLuint color = map[(sY % RES) * RES + (sX % RES)];

						GLuint r = ((color & 0xFF) + (GLuint)(rgb[0] * val));
						r = r > 255 ? 255 : r;
						GLuint g = (((color & 0xFF00) >> 8) + (GLuint)(rgb[1] * val));
						g = g > 255 ? 255 : g;
						GLuint b = (((color & 0xFF0000) >> 16) + (GLuint)(rgb[2] * val));
						b = b > 255 ? 255 : b;

						map[(sY % RES) * RES + (sX % RES)] = r | (g << 8) | (b << 16) | (255 << 24);
					}
				}

				//map[centerY * RES + centerX] = 255 | (255 << 8) | (255 << 16) | (255 << 24);
			}
		}
	}

	return map;
}


imgGenerator::imgGenerator()
{
}


imgGenerator::~imgGenerator()
{
}
