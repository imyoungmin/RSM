#version 410 core

in vec3 aPosition;
in vec3 aNormal;
in vec2 aTexCoords;

uniform mat4 Model;										// Model transform takes points from model into world coordinates.
uniform mat4 LightSpaceMatrix;							// Takes world to light space coordinates (= Proj_light * View_light).
uniform float pointSize;

out vec2 oTexCoords;									// Interpolate texture coordinates into fragment shader.

out vec3 oRSMPosition;									// Outputs into fragment shader in world space to be stored in RSM buffer.
out vec3 oRSMNormal;

void main( void )
{
	mat3 normalMatrix = transpose( inverse ( mat3( Model ) ) );
	vec4 p = Model * vec4( aPosition.xyz, 1.0 );		// Vertex position in world coordinates.
	gl_Position = LightSpaceMatrix * p;					// Projecting to light space.

	oRSMPosition = p.xyz;								// World space position.
	oRSMNormal = normalMatrix * aNormal;				// World space normal vector.

	gl_PointSize = pointSize;
	oTexCoords = aTexCoords;
}
