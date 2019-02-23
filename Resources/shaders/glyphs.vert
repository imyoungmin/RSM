#version 410 core

in vec4 coord;
out vec2 uv;

void main(void)
{
	gl_Position = vec4(coord.xy, 0, 1);
  	uv = coord.zw;
}