#version 410 core

out vec4 color;

in vec2 oTexCoords;

void main()
{
    color = vec4( oTexCoords, 0.0, 1.0 );
}
