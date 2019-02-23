#version 410 core

uniform bool drawPoint;

void main( void )
{
	if( drawPoint )
	{
		if( dot( gl_PointCoord-0.5, gl_PointCoord - 0.5 ) > 0.25 )		// For rounded points.
			discard;
	}
	
	// gl_FragDepth = gl_FragCoord.z;
}
