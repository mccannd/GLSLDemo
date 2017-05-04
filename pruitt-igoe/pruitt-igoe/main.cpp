
#include "./imgGenerator.h"
#include <SFML/Window.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/System/Clock.hpp>
#include "core/log.h"
#include <vector>
#include <iostream>
#include <fstream>

#define DEBUG true


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
	glm::vec3 pos = glm::vec3(1.8 * sin(0.5 * time), -0.3f, 1.8 * cos(0.5 * time));
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
	int pxWidth = 1024;// 640;
	int pxHeight = 768;// 480;

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
	
	GLuint* noiseTexture = generateMapPerlin();
	GLint imageAdd = glGetUniformLocation(shaderProgram, "texture0");
	glUniform1i(imageAdd, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, imageAdd);
	// set necessary texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, noiseTexture);
	
	
	GLuint* noiseTexture2 = generateStarfield(noiseTexture);//generateMap();
	GLint imageAdd2 = glGetUniformLocation(shaderProgram, "texture1");
	glUniform1i(imageAdd2, 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, imageAdd2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// allocate memory and set texture data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, noiseTexture2);
	
	sf::Music song;
	if (!song.openFromFile("Faunts - Das Malefitz.ogg"))
	{
		std::cout << "Failed to open song\n";
	}
	else {
		song.setLoop(true);
		song.play();
	}
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

	//delete[] noiseTexture;
	delete[] noiseTexture2;
	return 0;
}