#version 410 core

const uint N_SAMPLES = 100;
const float R_MAX = 0.085;								// Maximum sampling radius.
const float RSM_INTENSITY = 0.36;

uniform vec4 lightPosition;								// In camera coordinates.
uniform vec3 lightColor;								// Only RGB.

uniform vec4 ambient, diffuse, specular;				// The [r,g,b,a] ambient, diffuse, and specular material properties, respectively.
uniform float shininess;
uniform bool useBlinnPhong;
uniform bool useTexture;
uniform bool drawPoint;

uniform vec2 rsmSamplePositions[N_SAMPLES];				// Array of uniformly-distributed sampling positions in a unit disk.

uniform sampler2D rsmPosition;							// Reflective shadow map textures: positions.
uniform sampler2D rsmNormal;							// Normals.
uniform sampler2D rsmFlux;								// Flux.
uniform sampler2D rsmDepth;								// Depths.
uniform sampler2D objectTexture;						// 3D object texture.

in vec3 vPosition;										// Position in view (camera) coordinates.
in vec3 vNormal;										// Normal vector in view coordinates.
in vec2 oTexCoords;

in vec3 gPosition;										// Position and normal at this fragment in world space coordinates.
in vec3 gNormal;

in vec4 fragPosLightSpace;								// Position of fragment in light space (need w component for manual perspective division).

out vec4 color;

/**
 * Compute indirect lighting from pixels in the surroundings of current fragment.
 * @param uvFrag Current fragment's projected coordinates in light space (texture).
 * @param n Normalized normal vector to current fragment in world space coordinates.
 * @param x Fragment position in world space coordinates.
 */
vec3 indirectLighting( vec2 uvFrag, vec3 n, vec3 x )
{
	vec3 rsmShading = vec3( 0 );
	for( int i = 0; i < N_SAMPLES; i++ )				// Sum contributions of sampling locations.
	{
		vec2 uv = uvFrag + R_MAX * rsmSamplePositions[i];
		vec3 flux = texture( rsmFlux, uv ).rgb;			// Collect components from corresponding RSM textures.
		vec3 x_p = texture( rsmPosition, uv ).xyz;
		vec3 n_p = texture( rsmNormal, uv ).xyz;

		// Irradiance at current fragment w.r.t. pixel light at uv.
		vec3 r = x - x_p;								// Difference vector.
		float d2 = dot( r, r );							// Square distance.
		vec3 E_p = flux * ( max( 0.0, dot( n_p, r ) ) * max( 0.0, dot( n, -r ) ) );
		E_p *= rsmSamplePositions[i].x * rsmSamplePositions[i].x / ( d2 * d2 );				// Weighting contribution and normalizing.

		rsmShading += E_p;								// Accumulate.
	}

	return rsmShading * RSM_INTENSITY;					// Modulate result with some intensity value.
}

/**
 * Compute shadow for a given fragment.
 * @param projFrag Fragment position in normalized projected light space.
 * @param incidence Dot product of light and normal vectors at fragment to be rendered.
 * @return Shadow for fragment (1: Completely in shadow, 0: Completely lit).
 */
float computeShadow( vec3 projFrag, float incidence )
{
	vec2 uv = projFrag.xy;
	float zReceiver = projFrag.z;
	
	if( zReceiver > 1.0 )							// Anything farther than the light frustrum should be lit.
		return 0;
	
	float bias = max( 0.004 * ( 1.0 - incidence ), 0.005 );
	float depth = texture( rsmDepth, uv ).r;
	return ( zReceiver - depth > bias )? 1.0 : 0.0;
}

/**
 * Apply color given a selected light and shadow map.
 * @param projFrag Fragment position in normalized projected light space.
 * @param N Normalized normal vector to current fragment (if using Blinn-Phong shading) in camera coordinates.
 * @param E Normalized view direction (if using Blinn-Phong shading) in camera coordinates.
 * @param gN Normalized normal vector in global coordinates.
 * @param gP Position in global coordinates.
 * @return Fragment color (minus ambient component).
 */
vec3 shade( vec3 projFrag, vec3 N, vec3 E, vec3 gN, vec3 gP )
{
	vec3 diffuseColor = diffuse.rgb,
		 specularColor = specular.rgb;
	float shadow;

	if( useBlinnPhong )
	{
		vec3 L = normalize( lightPosition.xyz - vPosition );
		
		vec3 H = normalize( L + E );
		float incidence = dot( N, L );
		
		// Diffuse component.
		float cDiff = max( incidence, 0.0 );
		diffuseColor = cDiff * ( (useTexture)? texture( objectTexture, oTexCoords ).rgb * diffuseColor : diffuseColor );
		
		// Specular component.
		if( incidence > 0 && shininess > 0.0 )		// Negative shininess turns off specular component.
		{
			float cSpec = pow( max( dot( N, H ), 0.0 ), shininess );
			specularColor = cSpec * specularColor;
		}
		else
			specularColor = vec3( 0.0, 0.0, 0.0 );
		
		shadow = computeShadow( projFrag, incidence );
	}
	else
	{
		specularColor = vec3( 0.0, 0.0, 0.0 );
		shadow = computeShadow( projFrag, 1 );
	}

	// Calculate indirect lighting using the reflective shadow map.
	vec3 eColor = indirectLighting( projFrag.xy, gN, gP );
	
	// Fragment color with respect to this light (excluding ambient component).
	return ( 1.0 /*- shadow*/ ) * ( diffuseColor + specularColor + eColor ) * lightColor;
}

/**
 * Main function.
 */
void main( void )
{
	vec3 ambientColor = ambient.rgb;		// Ambient component is constant across lights.
    float alpha = ambient.a;
	vec3 N, E;								// Unit-length normal and eye direction (only necessary for shading with Blinn-Phong reflectance model).
	
	if( useBlinnPhong )
	{
		N = normalize( vNormal );
		E = normalize( -vPosition );
	}

	// Normalize coordinates of fragment in projected light space.
	vec3 projFrag = fragPosLightSpace.xyz / fragPosLightSpace.w;	// Perspective division: fragment is in [-1, +1].
    projFrag = projFrag * 0.5 + 0.5;								// Normalize fragment position to [0, 1].

    vec3 gN = normalize( gNormal );
    vec3 gP = gPosition/* - 0.0001 * gN*/;								// Avoid the illumination integral singularity (at the joint of walls).
	
    // Final fragment color is the sum of light contributions.
	vec3 totalColor = ambientColor + shade( projFrag, N, E, gN, gP );

    if( drawPoint )
    {
        if( dot( gl_PointCoord - 0.5, gl_PointCoord - 0.5 ) > 0.25 )		// For rounded points.
        	discard;
        else
            color = vec4( totalColor, alpha );
    }
    else
        color = vec4( totalColor, alpha );
}
