#version 410 core

in vec3 aPosition;			// Quad vertex.
in vec2 aTexCoords;			// Texture coordinates for quad vertex.

out vec2 oTexCoords;

void main()
{
	// Basically, render buffers into a quad in normalized device coordinates.
    oTexCoords = aTexCoords;
    gl_Position = vec4( aPosition, 1.0 );
}
