#version 410 core

layout (location = 0) out vec3 TexRSMPosition;			// Output to attachements for positions and normals in world space.
layout (location = 1) out vec3 TexRSMNormal;
layout (location = 2) out vec3 TexRSMFlux;				// Output to attachement for flux.

// We need almost all variables from normal shading to calculate the flux.
uniform vec3 lightColor;								// Only RGB.
uniform vec4 diffuse;									// The [r,g,b,a] material's diffuse color/albedo.
uniform bool useTexture;
uniform bool drawPoint;

uniform sampler2D objectTexture;						// 3D object texture (must be a blurred texture)
in vec2 oTexCoords;

in vec3 oRSMPosition;									// Inputs from vertex shader, in world space.
in vec3 oRSMNormal;

void main( void )
{
	// Store the fragment position vector in the first RSM buffer texture.
	TexRSMPosition = oRSMPosition;

	// Store the per-fragment normal into second RSM buffer texture.
	TexRSMNormal = normalize( oRSMNormal );

    // Determining the flux: it's the product of color light with material's albedo (i.e. diffuse component).
    // Ignore alpha or transparency.
    vec3 color = ((useTexture)? texture( objectTexture, oTexCoords ).rgb * diffuse.rgb : diffuse.rgb) * lightColor;

	if( drawPoint )
	{
		if( dot( gl_PointCoord - 0.5, gl_PointCoord - 0.5 ) > 0.25 )		// For rounded points.
			discard;
		else
			TexRSMFlux = color;
	}
	else
		TexRSMFlux = color;

	// The depth is writtin automatically.
	// gl_FragDepth = gl_FragCoord.z;
}
