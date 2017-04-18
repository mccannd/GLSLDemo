#version 150

in vec4 vertexPosition;
out vec2 fuv;




void main() {
	fuv = vertexPosition.xy;
    gl_Position = vec4(vertexPosition.x, vertexPosition.y, .5, 1.0);
}