#version 410 core

in vec2 uv;
uniform sampler2D tex;
uniform vec4 color;
out vec4 fragColor;

void main(void)
{
	fragColor = vec4(1, 1, 1, texture(tex, uv).r) * color;
}