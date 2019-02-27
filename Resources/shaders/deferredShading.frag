#version 410 core

// Shader variables.
out vec4 color;

in vec2 oTexCoords;

uniform sampler2D sGPosition;
uniform sampler2D sGNormal;
uniform sampler2D sGAlbedoSpecular;
uniform sampler2D sGPosLightSpace;

uniform vec4 lightPosition;								// In world space.
uniform vec3 lightColor;								// Only RGB.
uniform vec3 eyePosition;								// Viewer position in world space.

void main()
{
	// Retrieve data from G-Buffer textures.
	vec3 diffuseColor = texture( sGAlbedoSpecular, oTexCoords ).rgb,
		 specularColor = vec3( 0.8, 0.8, 0.8 );
	vec3 ambientColor = diffuseColor * 0.1;
	float shininess = texture( sGAlbedoSpecular, oTexCoords ).a;
	vec3 position = texture( sGPosition, oTexCoords ).rgb;
	vec3 projFrag = texture( sGPosLightSpace, oTexCoords ).rgb;
	bool useBlinnPhong = texture( sGPosLightSpace, oTexCoords ).a != 0.0;

	if( useBlinnPhong )												// Use Blinn-Phong reflectance model?
	{
		vec3 N = normalize( texture( sGNormal, oTexCoords ).rgb );
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
	}
	else
		specularColor = vec3( 0.0, 0.0, 0.0 );

	// Fragment color.
	color = vec4( ambientColor + ( diffuseColor + specularColor ) * lightColor, 1.0 );
}
