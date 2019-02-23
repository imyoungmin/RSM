#version 410 core

layout (location = 0) out vec3 oPosition;		// Output to attachements for positions and normals in world space.
layout (location = 1) out vec3 oNormal;
layout (location = 2) out vec3 oFlux;			// Output to attachement for flux.

uniform bool drawPoint;

void main( void )
{
	if( drawPoint )
	{
		if( dot( gl_PointCoord-0.5, gl_PointCoord - 0.5 ) > 0.25 )		// For rounded points.
			discard;
	}

	// Store the fragment position vector in the first RSM buffer texture.
	oPosition = vec3( 0.0, 0.0, 1.0 );

	// Store the per-fragment normal into the RSM buffer texture.
	oNormal = vec3( 0.0, 1.0, 0.0 );

    // And the flux.
	oFlux = vec3( 1.0, 0.0, 0.0 );

	// The depth is writtin automatically.
	// gl_FragDepth = gl_FragCoord.z;
}
