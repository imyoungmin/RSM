#version 410 core

in vec3 aPosition;
in vec3 aNormal;
in vec2 aTexCoords;

uniform mat4 Model;										// Model transform takes points from model into world coordinates.
uniform mat4 View;										// View matrix takes points from world into camera coordinates.
uniform mat3 InvTransModelView;							// Inverse-transposed 3x3 principal submatrix of ModelView matrix.
uniform mat4 Projection;
uniform float pointSize;
uniform bool useBlinnPhong;

uniform mat4 LightSpaceMatrix;							// Takes world to light space coordinates (= Proj_light * View_light).

out vec3 vPosition;										// Position in view (camera) coordinates.
out vec3 vNormal;										// Normal vector in view coordinates.
out vec2 oTexCoords;									// Interpolate texture coordinates into fragment shader.

out vec3 oPosition;										// Position and normal in world space coordinates.
out vec3 oNormal;

out vec4 fragPosLightSpace;								// Position of fragment in light space (need w component for manual perspective division).

void main( void )
{
	mat3 normalMatrix = transpose( inverse ( mat3( Model ) ) );

	vec4 p = Model * vec4( aPosition.xyz, 1.0 );		// Vertex in world coordinates.
	gl_Position = Projection * View * p;

	if( useBlinnPhong )
	{
		vPosition = (View * p).xyz;						// Send vertex and normal to fragment shader in camera coodinates.
		vNormal = InvTransModelView * aNormal;
	}

	gl_PointSize = pointSize;
	oTexCoords = aTexCoords;
	
	fragPosLightSpace = LightSpaceMatrix * p;			// Send vertex position in light space projected coordinates.

	oPosition = p.xyz;									// Send vertex position and normal in global coordinates.
	oNormal = normalMatrix * aNormal;
}
