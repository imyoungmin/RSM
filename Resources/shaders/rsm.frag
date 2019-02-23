#version 410 core

uniform vec4 lightPosition;								// In camera coordinates.
uniform vec3 lightColor;								// Only RGB.

uniform vec4 ambient, diffuse, specular;				// The [r,g,b,a] ambient, diffuse, and specular material properties, respectively.
uniform float shininess;
uniform bool useBlinnPhong;
uniform bool useTexture;
uniform bool drawPoint;

uniform sampler2D rsmPosition;							// Reflective shadow map textures: positions.
uniform sampler2D rsmNormal;							// Normals.
uniform sampler2D rsmFlux;								// Flux.
uniform sampler2D rsmDepth;								// Depths.
uniform sampler2D objectTexture;						// 3D object texture.

in vec3 vPosition;										// Position in view (camera) coordinates.
in vec3 vNormal;										// Normal vector in view coordinates.
in vec2 oTexCoords;

in vec4 fragPosLightSpace;								// Position of fragment in light space (need w component for manual perspective division).

out vec4 color;

/**
 * Compute shadow for a given fragment.
 * @param shadowMap Shadow map texture sampler to use.
 * @param coords Fragment 3D position in projected light space.
 * @param incidence Dot product of light and normal vectors at fragment to be rendered.
 * @return Shadow for fragment (1: Completely in shadow, 0: Completely lit).
 */
float computeShadow( sampler2D shadowMap, vec4 coords, float incidence )
{
	vec3 projFrag = coords.xyz / coords.w;			// Perspective division: fragment is in [-1, +1].
	projFrag = projFrag * 0.5 + 0.5;				// Normalize fragment position to [0, 1].
	
	vec2 uv = projFrag.xy;
	float zReceiver = projFrag.z;
	
	if( zReceiver > 1.0 )							// Anything farther than the light frustrum should be lit.
		return 0;
	
	float bias = max( 0.004 * ( 1.0 - incidence ), 0.005 );
	float depth = texture( shadowMap, uv ).r;
	return ( zReceiver - depth > bias )? 1.0 : 0.0;
}

/**
 * Apply color given a selected light and shadow map.
 * @param shadowMap Shadow map sampler to read depth values from.
 * @param fragPosLightSpace Fragment position in selected projected light space coordinates.
 * @param lightColor RGB color of light source.
 * @param lightPosition 3D coordinates of light source with respect to the camera.
 * @param N Normalized normal vector to current fragment (if using Blinn-Phong shading) in camera coordinates.
 * @param E Normalized view direction (if using Blinn-Phong shading) in camera coordinates.
 * @return Fragment color (minus ambient component).
 */
vec3 shade( sampler2D shadowMap, vec4 fragPosLightSpace, vec3 lightColor, vec3 lightPosition, vec3 N, vec3 E )
{
	vec3 diffuseColor = diffuse.rgb,
		 specularColor = specular.rgb;
	float shadow;

	if( useBlinnPhong )
	{
		vec3 L = normalize( lightPosition - vPosition );
		
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
		
		shadow = computeShadow( shadowMap, fragPosLightSpace, incidence );
	}
	else
	{
		specularColor = vec3( 0.0, 0.0, 0.0 );
		shadow = computeShadow( shadowMap, fragPosLightSpace, 1 );
	}
	
	// Fragment color with respect to this light (excluding ambient component).
	return ( 1.0 - shadow ) * ( diffuseColor + specularColor ) * lightColor;
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
	
    // Final fragment color is the sum of light contributions.
	vec3 totalColor = ambientColor + shade( rsmDepth, fragPosLightSpace, lightColor, lightPosition.xyz, N, E );
//	vec3 projFrag = fragPosLightSpace.xyz / fragPosLightSpace.w;			// Perspective division: fragment is in [-1, +1].
//    	projFrag = projFrag * 0.5 + 0.5;				// Normalize fragment position to [0, 1].
//	vec2 uv = projFrag.xy;
//	vec3 totalColor = texture( rsmPosition, uv ).rgb;
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
