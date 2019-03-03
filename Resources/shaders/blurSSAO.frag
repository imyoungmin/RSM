#version 410 core

layout (location = 0) out float TexSSAOFactor;	// Outputting to a single render attachment, which is the NON occlusion factor.

in vec2 oTexCoords;

uniform sampler2D sSSAOFactor;					// SSAO occlusion factor sampler.

void main()
{
    vec2 texelSize = 1.0 / vec2( textureSize( sSSAOFactor, 0 ) );
    float result = 0.0;
    for( int x = -2; x < 2; x++ )				// Average samples around current texel.
    {
        for( int y = -2; y < 2; y++ )
        {
            vec2 offset = vec2( float(x), float(y) ) * texelSize;
            result += texture( sSSAOFactor, oTexCoords + offset ).r;
        }
    }
    TexSSAOFactor = result / 16.0;
}
