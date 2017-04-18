#include "./common.h"
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "core/log.h"
#include <vector>
#include <iostream>
#include <fstream>

#define DEBUG true

// GLSL parser and debug printer by Mariano Merchante
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

int main() {

	// create the window
	sf::Window window(sf::VideoMode(640, 480, 32), "Spaceflight");
	window.setVerticalSyncEnabled(true);

	// activate the window
	window.setActive(true);

	// Load GLEW
	GLenum err = glewInit();

	// Show this before, just in case there's an error
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

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// raymarch me bro
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		window.display();
	}


	return 0;
}