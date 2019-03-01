#version 410 core

layout (location = 0) out float TexSSAOFactor;	// Outputting to a single render attachment, which is the NON occlusion factor.

const int KERNEL_SIZE = 32;
const float HEMISPHERE_RADIUS = 0.45;
const float BIAS = 0.05;

in vec2 oTexCoords;

uniform sampler2D sGPosition;
uniform sampler2D sGNormal;
uniform sampler2D sSSAONoiseTexture;	// The 4x4 noise texture.

uniform vec3 ssaoSamples[KERNEL_SIZE];	// Normal hemisphere samples.
uniform mat4 View;						// View matrix to take points/normals from G-Buffer into camera coordinates.
uniform mat4 Projection;

uniform float frameBufferWidth;			// Effective width and height of framebuffer of OpenGL window.
uniform float frameBufferHeight;

void main()
{
	vec2 noiseScale = vec2( frameBufferWidth/4.0, frameBufferHeight/4.0 );							// Tile noise texture over screen.

    // Collect position, normal, and random noise from G-Buffer and noise sampler.
	vec3 vPosition = ( View * vec4( texture( sGPosition, oTexCoords ).xyz, 1.0 ) ).xyz;				// Position in camera space.
	vec3 vNormal = normalize( ( View * vec4( texture( sGNormal, oTexCoords ).xyz, 1.0 ) ).xyz );	// Normal in camera space.
	vec3 randomVector = texture( sSSAONoiseTexture, oTexCoords * noiseScale ).xyz;					// Pick a unit random vector from noise texture in the xy plane.

	// Create a transform matrix that takes points from tangent space to view space (since normal is in camera space already).
	// Gramm-Schmidt process.
	vec3 tangent = normalize( randomVector - vNormal * dot( randomVector, vNormal ) );
	vec3 bitangent = cross( vNormal, tangent );
	mat3 T = mat3( tangent, bitangent, vNormal );		// Orhonormal basis matrix.

	// Iterate over the samples kernel and calculate occlusion factor.
    float occlusion = 0.0;
	for( int i = 0; i < KERNEL_SIZE; ++i )
	{
		vec3 s = T * ssaoSamples[i]; 					// Pick sample and transform it to view space.
		s = vPosition + s * HEMISPHERE_RADIUS;			// Place sample around the current fragment.

		// Project sample position (to sample texture) to get position on screen/texture.
		vec4 offset = vec4( s, 1.0 );
		offset = Projection * offset; 					// From view to [-1, +1].
		offset.xyz /= offset.w; 						// Perspective division.
		offset.xyz = offset.xyz * 0.5 + 0.5; 			// From [-1, +1] to [0, 1].

		// Get viewing depth stored at the G-buffer gPosition texture at the location given by current sample.
		float vDepth = ( View * vec4( texture( sGPosition, offset.xy ).xyz, 1.0 ) ).z;

		// Range check and accumulate.
		float rangeCheck = smoothstep( 0.0, 1.0, HEMISPHERE_RADIUS / abs( vPosition.z - vDepth ) );
		occlusion += ( vDepth >= s.z + BIAS ? 1.0 : 0.0 ) * rangeCheck;
    }
	TexSSAOFactor = pow( 1.0 - ( occlusion / KERNEL_SIZE ), 7.0 );
}
