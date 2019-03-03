#version 410 core

in vec3 aPosition;
in vec3 aNormal;
in vec2 aTexCoords;										// Object model texture coordinates.

uniform mat4 Model;										// Model transform takes points from model into world coordinates.
uniform mat4 View;										// View matrix takes points from world into camera coordinates.
uniform mat4 Projection;
uniform mat4 LightSpaceMatrix;                          // Project world-space points into light space projective space.
uniform mat3 InvTransModelView;							// Inverse-transposed 3x3 principal submatrix of ModelView matrix.
uniform float pointSize;

out vec3 oGPosition;
out vec2 oTexCoords;
out vec3 oGNormal;
out vec4 oGPosLightSpace;
out vec3 oGVPosition;									// Position and normal in view coordinates.
out vec3 oGVNormal;

void main()
{
	mat3 normalMatrix = transpose( inverse ( mat3( Model ) ) );
	vec4 p = Model * vec4( aPosition.xyz, 1.0 );		// Vertex position in world coordinates.
	vec4 vP = View * p;									// Vertex position in view coordinates.
	gl_Position = Projection * vP;						// Usual projection.

	oGPosition = p.xyz;									// World space position.
	oGNormal = normalMatrix * aNormal;					// World space normal vector.
	oGPosLightSpace = LightSpaceMatrix * p;             // Vertex position in projective light space.
	
	oGVPosition = vP.xyz;								// Position and normal in view space.
	oGVNormal = InvTransModelView * aNormal;

	gl_PointSize = pointSize;
	oTexCoords = aTexCoords;
}
