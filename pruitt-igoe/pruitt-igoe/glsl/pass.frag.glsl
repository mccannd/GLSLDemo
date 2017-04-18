#version 150

in vec2 fuv;
out vec4 out_Col;

void main() {
    out_Col = vec4(fuv, 0, 1);
}