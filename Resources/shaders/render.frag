#version 410 core

// Reflective shadow maps constants.
const uint N_SAMPLES = 150;
const float R_MAX = 0.09;							// Maximum sampling radius.
const float RSM_INTENSITY = 0.4;

// Percentage closer soft shadow constants.
const uint PCSS_SAMPLES = 31;
const float NEAR_PLANE = 0.01;
const float LIGHT_WORLD_SIZE = 2.0;
const float LIGHT_FRUSTUM_WIDTH = 20.0;
const float LIGHT_SIZE_UV = (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH);	// Assuming that LIGHT_FRUSTUM_WIDTH = LIGHT_FRUSTUM_HEIGHT.

// Screen Space Ambient Occlusion constants.
const float SSAO_AMBIENT_WEIGHT = 0.3;

// Light attenuation constants.
const float LIGHT_CONSTANT_PARAM = 1.0;
const float LIGHT_LINEAR_PARAM = 0.027;
const float LIGHT_QUADRATIC_PARAM = 0.0028;

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

uniform sampler2D sSSAOFactor;						// SSAO occlusion factor sampler.

in vec2 oTexCoords;									// NDC quad texture coordinates.

uniform vec4 lightPosition;							// In world space.
uniform vec3 lightColor;							// Only RGB.
uniform vec3 eyePosition;							// Viewer position in world space.

uniform bool enableSSAO;							// Use or not SSAO.

// Used for searching and filtering the depth/shadow map.
const vec2 poissonDisk[PCSS_SAMPLES] = vec2[PCSS_SAMPLES](
	vec2(0.19483898, -0.04906884), vec2(-0.20130208, -0.1598451), vec2(-0.17657379, 0.27214397), vec2(-0.18434988, 0.71673575),
	vec2(0.19165408, 0.89863354), vec2(-0.58109718, 0.63340388), vec2(0.05128605, 0.51784401), vec2(0.63354365, 0.5275925),
	vec2(-0.51379029, 0.1419501), vec2(0.69485186, 0.13043893), vec2(-0.88038342, 0.3311689), vec2(0.10940117, -0.63964106),
	vec2(0.34065359,  0.36201536), vec2(-0.85766868, -0.19484638), vec2(0.68075919,  0.9383508 ), vec2(0.98666111,  0.70115573),
	vec2(0.93154108,  0.40278772), vec2(0.50832938, -0.35158726), vec2(-0.55546935, -0.39379395), vec2(-0.38763822,  0.96256016),
	vec2(-0.81259892, -0.80838567), vec2(-0.99363224,  0.79686303), vec2(-0.4778399 , -0.95310737), vec2(-0.75429473,  0.98810816),
	vec2(0.81623723, -0.72246694), vec2(0.05209179, -0.96911855), vec2(0.10548147, -0.33657712), vec2(-0.20686522, -0.49231428),
	vec2(0.97070736, -0.24451268), vec2(0.44274798, -0.77235927), vec2(-0.83493721, -0.50384213)
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
		vec2 offset = poissonDisk[i] * max( bias/3.0, filterRadiusUV );
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

	float bias = max( 0.004 * ( 1.0 - incidence ), 0.0045 );

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
	vec3 diffuseColor = texture( sGAlbedoSpecular, oTexCoords ).rgb * lightColor,
		 specularColor = vec3( 0.8, 0.8, 0.8 );
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

	// Light attenuation.
	float lDistance = length( lightPosition.xyz - position );
	float lAttenuation = 1.0 / ( LIGHT_CONSTANT_PARAM + LIGHT_LINEAR_PARAM * lDistance + LIGHT_QUADRATIC_PARAM * lDistance * lDistance );

	// Fragment color.
	color = vec4( ambientColor + ( 1.0 - shadow ) * ( diffuseColor + specularColor + eColor ) * ( 1.0 - ( enableSSAO ? SSAO_AMBIENT_WEIGHT : 0.0 ) ) * lAttenuation, 1.0 );
}
