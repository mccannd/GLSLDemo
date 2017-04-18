#version 150

in vec4 vertexPosition;
out vec2 fuv;

void main() {
	fuv = 0.5 * vertexPosition.xy + vec2(0.5, 0.5);
    gl_Position = vec4(vertexPosition.x, vertexPosition.y, .5, 1.0);
}