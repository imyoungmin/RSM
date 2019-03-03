#version 410 core

// Reflective shadow maps constants.
const uint N_SAMPLES = 151;
const float R_MAX = 0.07;							// Maximum sampling radius.
const float RSM_INTENSITY = 0.6;

// Percentage closer soft shadow constants.
const uint PCSS_SAMPLES = 33;
const float NEAR_PLANE = 0.01;
const float LIGHT_WORLD_SIZE = 2.0;
const float LIGHT_FRUSTUM_WIDTH = 20.0;
const float LIGHT_SIZE_UV = (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH);	// Assuming that LIGHT_FRUSTUM_WIDTH = LIGHT_FRUSTUM_HEIGHT.

// Screen Space Ambient Occlusion constants.
const float SSAO_AMBIENT_WEIGHT = 0.25;

// Shader variables.
uniform vec2 RSMSamplePositions[N_SAMPLES];			// Array of uniformly-distributed sampling positions in a unit disk.

uniform sampler2D sRSMPosition;						// Reflective shadow map textures: positions.
uniform sampler2D sRSMNormal;						// Normals.
uniform sampler2D sRSMFlux;							// Flux.
uniform sampler2D sRSMDepth;						// Depths.

uniform sampler2D sGPosition;						// G-Buffer textures: positions.
uniform sampler2D sGNormal;							// Normals.
uniform sampler2D sGAlbedoSpecular;					// Object's RGB albedo + specular shininess.
uniform sampler2D sGPosLightSpace;					// Fragment position in normalized reflective light space + using Blinn-Phong shading flag.
uniform sampler2D sGDepth;							// Depth buffer texture.

uniform sampler2D sSSAOFactor;						// SSAO occlusion factor sampler.

in vec2 oTexCoords;									// NDC quad texture coordinates.

uniform vec4 lightPosition;							// In world space.
uniform vec3 lightColor;							// Only RGB.
uniform vec3 eyePosition;							// Viewer position in world space.

uniform bool enableSSAO;							// Use or not SSAO.

// Used for searching and filtering the depth/shadow map.
const vec2 poissonDisk[PCSS_SAMPLES] = vec2[PCSS_SAMPLES](
	vec2(-0.17232697437460032, -0.5062881718328716), vec2(-0.294162930913042, -0.8107894897875367), vec2(-0.019831996686628273, -0.935165017572383), vec2(0.24271866619042637, -0.5106033293811425),
	vec2(-0.5395439431143971, -0.7651503978096444), vec2(0.07389515110145806, -0.7012223077921668), vec2(0.08780299948354009, -0.14727029215602516), vec2(0.47655543531189926, -0.4407089253864036),
	vec2(0.502639069753168, -0.7324190784846516), vec2(-0.5416255049871092, -0.48581401377682243), vec2(0.34139565919314796, -0.19305756133365937), vec2(-0.18275878403669044, -0.14268591370966954),
	vec2(-0.9842915819814386, -0.03681602905392933), vec2(-0.7399111319357496, -0.2868575213448644), vec2(0.7283424328534942, -0.28861487434709754), vec2(-0.3186284178614639, 0.18467631504722215),
	vec2(-0.5131076967664215, -0.11779853725441991), vec2(-0.5914496428377283, 0.13717144734270348), vec2(0.775378879856607, 0.11622481346074376), vec2(0.5908709361594684, 0.38952087361743426),
	vec2(-0.8277838430235871, 0.2119381439781829), vec2(-0.46220197836090016, 0.3932213768459396), vec2(-0.3012476690126118, 0.5996622534406295), vec2(-0.04652760771971687, 0.5935589013065825),
	vec2(0.7986665781534596, 0.584016274681399), vec2(0.3322325098601957, 0.45927262078368414), vec2(0.15110118164212016, 0.1220800298717366), vec2(-0.6040595292023718, 0.6787992564550387),
	vec2(0.49039889863745545, 0.017502013993166088), vec2(-0.014882147350587793, 0.32762762866048045), vec2(0.3725777142294926, 0.7107790265580047), vec2(0.07493684595473016, 0.8008472791079064),
	vec2(-0.20100611752963116, 0.8623389977253264)
);

out vec4 color;

///////////////////////////////////////// Reflective shadow maps functions /////////////////////////////////////////////

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
		vec2 uv = uvFrag + R_MAX * RSMSamplePositions[i];
		vec3 flux = texture( sRSMFlux, uv ).rgb;		// Collect components from corresponding RSM textures.
		vec3 x_p = texture( sRSMPosition, uv ).xyz;		// Position (x_p) and normal (n_p) are in world coordinates too.
		vec3 n_p = texture( sRSMNormal, uv ).xyz;

		// Irradiance at current fragment w.r.t. pixel light at uv.
		vec3 r = x - x_p;								// Difference vector.
		float d2 = dot( r, r );							// Square distance.
		vec3 E_p = flux * ( max( 0.0, dot( n_p, r ) ) * max( 0.0, dot( n, -r ) ) );
		E_p *= RSMSamplePositions[i].x * RSMSamplePositions[i].x / ( d2 * d2 );				// Weighting contribution and normalizing.

		rsmShading += E_p;								// Accumulate.
	}

	return rsmShading * RSM_INTENSITY;					// Modulate result with some intensity value.
}

///////////////////////////////////// Percentage closer soft shadows functions /////////////////////////////////////////

/**
 * Parallel plane estimatino of penumbra size.
 * @param zReceiver Current fragment depth in normalized coordinates [0, 1].
 * @param zBlocker Average blocker depth.
 * @return Triangle similarity estimation of proportion for penumbra size.
 */
float penumbraSize( float zReceiver, float zBlocker )
{
	return ( zReceiver - zBlocker ) / zBlocker;			//Parallel plane estimation
}

/**
 * Get the average blocker depth values that are closer to the light than current depth.
 * @param uv Fragment position in normalized coordinates [0, 1].
 * @param zReceiver Depth of current fragment in normalized coordinates [0, 1].
 * @param bias If given, evaluation bias to prevent 'depth' acne.
 * @return Average blocker depth or -1 if no blockers were found.
 */
float findBlockerDepth( vec2 uv, float zReceiver, float bias )
{
	// Uses similar triangles to compute what area of the shadow map we should search.
	float searchWidth = LIGHT_SIZE_UV * ( zReceiver - NEAR_PLANE );
	float blockerSum = 0, numBlockers = 0;

	for( int i = 0; i < PCSS_SAMPLES; i++ )
	{
		float shadowMapDepth = texture( sRSMDepth, uv + poissonDisk[i] * searchWidth ).r;
		if( zReceiver - shadowMapDepth > 0 )				// A blocker? Closer to light.
		{
			blockerSum += shadowMapDepth;					// Accumulate blockers depth.
			numBlockers++;
		}
	}

	return ( numBlockers > 0 )? blockerSum / numBlockers : -1.0;
}

/**
 * Apply percentage closer filter to a set of samples around the current fragment (in the shadow map).
 * @param uv Fragment position in normalized coordinates [0 ,1] with respect to light projected space.
 * @param zReceiver Fragment's depth value in light projected space.
 * @param filterRadiusUV Sampling radius around the fragment's position in shadow map.
 * @param bias Evaluation bias to prevent shadow acne.
 * @return Percentage of shadow to be assigned to fragment.
 */
float applyPCFilter( vec2 uv, float zReceiver, float filterRadiusUV, float bias )
{
	float shadow = 0;
	for( int i = 0; i < PCSS_SAMPLES; i++ )
	{
		vec2 offset = poissonDisk[i] * max( bias, filterRadiusUV );
		float pcfDepth = texture( sRSMDepth, uv + offset ).r;
		shadow += ( zReceiver - pcfDepth > bias )? 1.0 : 0.0;
	}
	return shadow / PCSS_SAMPLES;
}

/**
 * Percentage closer soft shadow method.
 * @param projFrag Fragment position in normalized projected light space.
 * @param incidence Dot product of light and normal vectors at fragment to be rendered.
 * @return Shadow percentage for fragment (1: Completely in shadow, 0: Completely lit).
 */
float pcss( vec3 projFrag, float incidence )
{
	vec2 uv = projFrag.xy;
	float zReceiver = projFrag.z;

	if( zReceiver > 1.0 )							// Anything farther than the light frustrum should be lit.
		return 0;

	float bias = max( 0.001 * ( 1.0 - incidence ), 0.002 );

	// Step 1: Blocker search.
	float avgBlockerDepth = findBlockerDepth( uv, zReceiver, 0.0 );
	if( avgBlockerDepth < 0 )						// There are no occluders so early out (this saves filtering).
		return 0;

	// Step 2: Penumbra size.
	float penumbraRatio = penumbraSize( zReceiver, avgBlockerDepth );
	float filterRadiusUV = penumbraRatio * LIGHT_SIZE_UV * NEAR_PLANE / zReceiver;

	// Step 3: Filtering.
	return applyPCFilter( uv, zReceiver, filterRadiusUV, bias );
}

////////////////////////////////////////////////// Shading functions ///////////////////////////////////////////////////

/**
 * Main function.
 */
void main( void )
{
	// Retrieve data from G-Buffer textures.
	vec3 diffuseColor = texture( sGAlbedoSpecular, oTexCoords ).rgb;
	
	if( texture( sGDepth, oTexCoords ).r < 1.0 )		// Perform calculations for fragments not in the far plane (depth = 1).
	{
		diffuseColor *= lightColor;
		vec3 specularColor = vec3( 0.8, 0.8, 0.8 );
		float shininess = texture( sGAlbedoSpecular, oTexCoords ).a;
		vec3 position = texture( sGPosition, oTexCoords ).rgb;
		vec3 projFrag = texture( sGPosLightSpace, oTexCoords ).rgb;
		bool useBlinnPhong = texture( sGPosLightSpace, oTexCoords ).a != 0.0;
		
		// Retrieve data from the SSAO occlusion sampler if it is enabled.
		float ambientOcclusion = 0.0;
		vec3 ambientColor = diffuseColor * 0.1;
		if( enableSSAO )
		{
			ambientOcclusion = texture( sSSAOFactor, oTexCoords ).r;
			ambientColor = diffuseColor * ambientOcclusion * SSAO_AMBIENT_WEIGHT;
		}
		
		float shadow = 0;												// PCSS shadow result for this fragment.
		vec3 eColor = vec3( 0 );										// Indirect lighting works only when normals are given.
		if( useBlinnPhong )												// Use Blinn-Phong reflectance model?
		{
			vec3 N = texture( sGNormal, oTexCoords ).rgb;
			vec3 E = normalize( eyePosition - position );				// View direction.
			vec3 L = normalize( lightPosition.xyz - position );			// Light direction.
			vec3 H = normalize( L + E );								// Half vector.
			float incidence = dot( N, L );
			
			// Diffuse component.
			float cDiff = max( incidence, 0.0 );
			diffuseColor = cDiff * diffuseColor;
			
			// Specular component.
			if( incidence > 0 && shininess > 0.0 )						// Negative shininess turns off specular component.
			{
				float cSpec = pow( max( dot( N, H ), 0.0 ), shininess );
				specularColor = cSpec * specularColor;
			}
			else
				specularColor = vec3( 0.0, 0.0, 0.0 );
			shadow = pcss( projFrag, incidence );
			
			// Calculate indirect lighting using the reflective shadow map.
			eColor = indirectLighting( projFrag.xy, N, position );
		}
		else
		{
			specularColor = vec3( 0.0, 0.0, 0.0 );
			shadow = pcss( projFrag, 1 );
		}
		
		// Fragment color.
		color = vec4( ambientColor + ( 1.0 - shadow ) * ( diffuseColor + specularColor + eColor ) * ( 1.0 - ( enableSSAO ? SSAO_AMBIENT_WEIGHT : 0.0 ) ), 1.0 );
	}
	else
		color = vec4( diffuseColor, 1.0 );
}
