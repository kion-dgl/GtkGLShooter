#version 130

attribute vec2 coord2d;
uniform mat4 ortho, mvp;

void main(void) {

	gl_Position = ortho * mvp * vec4(coord2d, 0.0, 1.0);

}
