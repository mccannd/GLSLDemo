#include "./common.h"
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System/Clock.hpp>
#include "core/log.h"
#include <vector>
#include <iostream>
#include <fstream>

#define DEBUG true
#define RES 1024

float hashNoise(float x, float y) {

	float n = sin(dot(glm::vec2(x, y), glm::vec2(12.9898f, 78.233f))) * 43758.5453f;
	n = n - floor(n);
	//return 0.5f + 0.5f * n;
	return n;
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
	if (fabs(f1[0] - freq) < 0.001) f1[0] = 0.0;
	if (fabs(f1[1] - freq) < 0.001) f1[1] = 0.0;

	float a = hashNoise(f[0], f[1]);
	float b = hashNoise(f1[0], f[1]);
	float c = hashNoise(f[0], f1[1]);
	float d = hashNoise(f1[0], f1[1]);

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

// to be moved to a separate class soon

GLuint* generateMap() {
	GLuint * map = new GLuint[RES * RES];

	for (int row = 0; row < RES; row++) {
		for (int col = 0; col < RES; col++) {
			GLuint color = (int)(255 * fbm((float) row / RES, (float) col / RES));
			GLuint color1 = (int)(255 * fbm((float)row / RES + 1.0f, (float)col / RES + 1.0f));
			GLuint color2 = (int)(255 * fbm((float)row / RES + 2.0f, (float)col / RES + 2.0f));

			color = (color << 0) | (color1 << 8) | (color2 << 16) | (255 << 24);
			map[row * RES + col] = color;
		}
	}

	return map;
}


// GLSL parser by Mariano Merchante
std::string ReadFile(const std::string& filename)
{
	std::ifstream in(filename, std::ios::in);

	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}

	return "";
}

// GLSL debug printer by Mariano Merchante
void PrintShaderInfoLog(int shader)
{
	int infoLogLen = 0;
	int charsWritten = 0;
	GLchar * infoLog;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

	if (infoLogLen > 0)
	{
		infoLog = new GLchar[infoLogLen + 1];
		glGetShaderInfoLog(shader, infoLogLen, &charsWritten, infoLog);
		std::cout << "ShaderInfoLog:" << std::endl << infoLog << std::endl;
		delete[] infoLog;
	}
}

void update(GLuint& shader, float time) {
	GLint timeUniform = glGetUniformLocation(shader, "u_time");
	glUniform1f(timeUniform, time);

	glm::vec3 tgt = glm::vec3(0, 0, 0);
	glm::vec3 pos = glm::vec3(1.5 * sin(0.5 * time), -0.1f, 1.5 * cos(0.5 * time));
	glm::vec3 F = glm::normalize(tgt - pos);
	glm::vec3 R = glm::normalize(glm::cross(F, glm::vec3(0, 1, 0)));
	glm::vec3 U = glm::normalize(glm::cross(R, F));

	glm::mat3 FRU = glm::mat3(F, R, U);
	GLint projUniform = glGetUniformLocation(shader, "u_cam_proj");
	glUniformMatrix3fv(projUniform, 1, GL_FALSE, glm::value_ptr(FRU));
	GLint camUniform = glGetUniformLocation(shader, "u_cam_pos");
	glUniform3f(camUniform, pos[0], pos[1], pos[2]);

	return;
}


int main() {
	int pxWidth = 640;
	int pxHeight = 480;

	// create the window
	sf::Window window(sf::VideoMode(pxWidth, pxHeight, 32), "Spaceflight");
	window.setVerticalSyncEnabled(true);

	// activate the window
	window.setActive(true);

	// Load GLEW
	GLenum err = glewInit();

	std::cout << "--------------------------------------" << std::endl;
	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;;
	std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "--------------------------------------" << std::endl;

	if (err != GLEW_OK) {
		std::cerr << "glewInit failed: " << glewGetErrorString(err) << std::endl;
		exit(1);
	}

	if (!GLEW_VERSION_3_0)
	{
		exit(1);
	}

	glEnable(GL_TEXTURE_2D);

	// create a quad to be drawn across the entire screen
	glm::vec4 vert[] = {
		glm::vec4(-1, -1, 0, 1.0f),
		glm::vec4(1, -1, 0, 1.0f),
		glm::vec4(1, 1, 0, 1.0f),
		glm::vec4(-1, 1, 0, 1.0f),
	};

	GLuint indices[] = {
		0, 1, 2, 2, 3, 0
	};

	GLuint idx;
	glGenBuffers(1, &idx);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), indices, GL_STATIC_DRAW);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vert), vert, GL_STATIC_DRAW);

	// Generate the shaders for a fullscreen quad
	GLuint shaderProgram = glCreateProgram();
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	std::string vertexSource = ReadFile("./glsl/pass.vert.glsl");
	std::string fragmentSource = ReadFile("./glsl/pass.frag.glsl");
	const char * vertSource = vertexSource.c_str();
	const char * fragSource = fragmentSource.c_str();
	glShaderSource(vertShader, 1, &vertSource, 0);
	glShaderSource(fragShader, 1, &fragSource, 0);
	glCompileShader(vertShader);
	glCompileShader(fragShader);

	// error check w/ Mariano's code
	GLint compiled;
	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		PrintShaderInfoLog(vertShader);
	}

	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		PrintShaderInfoLog(fragShader);
	}

	// combine vertex and frag shaders
	glAttachShader(shaderProgram, vertShader);
	glAttachShader(shaderProgram, fragShader);

	glBindFragDataLocation(shaderProgram, 0, "out_Col");

	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	// begin linking uniforms
	GLint posAttrib = glGetAttribLocation(shaderProgram, "vertexPosition");
	glVertexAttribPointer(posAttrib, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);

	GLint resw = glGetUniformLocation(shaderProgram, "res_width");
	glUniform1i(resw, pxWidth);
	GLint resh = glGetUniformLocation(shaderProgram, "res_height");
	glUniform1i(resh, pxHeight);

	GLuint* noiseTexture = generateMap();
	GLint imageAdd = glGetUniformLocation(shaderProgram, "texture0");
	glUniform1i(imageAdd, 0);
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, imageAdd);

		// set necessary texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// allocate memory and set texture data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, RES, RES, 0, GL_RGBA, GL_UNSIGNED_BYTE, noiseTexture);
	sf::Clock clock = sf::Clock();

	// run the main loop
	bool running = true;
	while (running) {
		// handle events
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed) {
				// end the program
				running = false;
			}
		}

		update(shaderProgram, clock.getElapsedTime().asSeconds());

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// raymarch me bro
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		window.display();
	}

	delete[] noiseTexture;
	return 0;
}