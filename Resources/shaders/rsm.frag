#version 410 core

// Reflective shadow maps constants.

const uint N_SAMPLES = 100;
const float R_MAX = 0.09;								// Maximum sampling radius.
const float RSM_INTENSITY = 0.55;

// Percentage closer soft shadow constants.
const uint PCSS_SAMPLES = 31;
const float NEAR_PLANE = 0.01;
const float LIGHT_WORLD_SIZE = 2.0;
const float LIGHT_FRUSTUM_WIDTH = 20.0;
const float LIGHT_SIZE_UV = (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH);	// Assuming that LIGHT_FRUSTUM_WIDTH = LIGHT_FRUSTUM_HEIGHT.

// Shader variables.
uniform vec4 lightPosition;								// In camera coordinates.
uniform vec3 lightColor;								// Only RGB.

uniform vec4 ambient, diffuse, specular;				// The [r,g,b,a] ambient, diffuse, and specular material properties, respectively.
uniform float shininess;
uniform bool useBlinnPhong;
uniform bool useTexture;
uniform bool drawPoint;

uniform vec2 RSMSamplePositions[N_SAMPLES];				// Array of uniformly-distributed sampling positions in a unit disk.

uniform sampler2D sRSMPosition;							// Reflective shadow map textures: positions.
uniform sampler2D sRSMNormal;							// Normals.
uniform sampler2D sRSMFlux;								// Flux.
uniform sampler2D sRSMDepth;							// Depths.
uniform sampler2D objectTexture;						// 3D object texture.

in vec3 vPosition;										// Position in view (camera) coordinates.
in vec3 vNormal;										// Normal vector in view coordinates.
in vec2 oTexCoords;

in vec3 oPosition;										// Position and normal at this fragment in world space coordinates.
in vec3 oNormal;

in vec4 fragPosLightSpace;								// Position of fragment in light space (need w component for manual perspective division).

out vec4 color;

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
		vec3 x_p = texture( sRSMPosition, uv ).xyz;
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
		
		shadow = pcss( projFrag, incidence );
	}
	else
	{
		specularColor = vec3( 0.0, 0.0, 0.0 );
		shadow = pcss( projFrag, 1 );
	}

	// Calculate indirect lighting using the reflective shadow map.
	vec3 eColor = indirectLighting( projFrag.xy, gN, gP );
	
	// Fragment color with respect to this light (excluding ambient component).
	return ( 1.0 - shadow ) * ( diffuseColor + specularColor + eColor ) * lightColor;
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

    vec3 oN = normalize( oNormal );
    vec3 oP = oPosition/* - 0.0001 * gN*/;								// Avoid the illumination integral singularity (at the joint of walls).
	
    // Final fragment color is the sum of light contributions.
	vec3 totalColor = ambientColor + shade( projFrag, N, E, oN, oP );

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
