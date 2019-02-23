#version 410 core

in vec3 position;

uniform mat4 Model;										// Model transform takes points from model into world coordinates.
uniform mat4 LightSpaceMatrix;							// Takes world to light space coordinates (= Proj_light * View_light).

uniform float pointSize;

void main( void )
{
	gl_Position = LightSpaceMatrix * Model * vec4( position, 1.0 );		// Transforming all scene vertices to light space.
	gl_PointSize = pointSize;
}
