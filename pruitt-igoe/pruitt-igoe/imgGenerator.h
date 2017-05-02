#pragma once
#include "./common.h"
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>


GLuint* generateMap();
GLuint* generateMapPerlin();
GLuint* generateStarfield(GLuint* bg);
class imgGenerator
{
public:
	imgGenerator();
	~imgGenerator();
};

