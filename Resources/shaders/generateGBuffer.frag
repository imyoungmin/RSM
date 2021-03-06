#version 410 core

layout (location = 0) out vec3 TexGPosition;			// Output to attachments for positions and normals in world space.
layout (location = 1) out vec3 TexGNormal;
layout (location = 2) out vec4 TexGAlbedoSpecular;		// Output to attachment for albedo and specular (RGB -> diffuse component, A -> shininess component).
layout (location = 3) out vec4 TexGPosLightSpace;       // Output to attachment for position in projective light space: RGB map to position, A maps to 0 -> flat shading, 1 -> Blinn-Phong reflectance model.

in vec3 oGPosition;
in vec2 oTexCoords;
in vec3 oGNormal;
in vec4 oGPosLightSpace;

uniform vec4 diffuse;									// The [r,g,b,a] object albedo (ambient = 0.1 * diffuse, and specular = [0.8, 0.8, 0.8]).
uniform float shininess;
uniform bool useTexture;								// Shall we use texture instead of plain color?
uniform bool drawPoint;									// Drawing rounded points?
uniform bool useBlinnPhong;								// Use Blinn-Phong reflectance model?

uniform sampler2D objectTexture;						// 3D object texture in case albedo is given there.

void main()
{
	// Store the fragment position vector in the first G buffer texture.
	TexGPosition = oGPosition;

	// Store the per-fragment normal into second G buffer texture.
	// If useBlinnPhong is false, this normal vector is meaningless and won't be used in rendering.
	TexGNormal = normalize( oGNormal );

	// Store the fragment position in projective light space, now in normalized coordinates [0, 1].
	vec3 projFrag = oGPosLightSpace.xyz / oGPosLightSpace.w;				// Perspective division: fragment is in [-1, +1].
	projFrag = projFrag * 0.5 + 0.5;										// Normalize fragment position to [0, 1].
	TexGPosLightSpace = vec4( projFrag, ( useBlinnPhong )? 1: 0 );			// The Alpha channel will be used to store the useBlinnPhong flag.

	// Store [R,G,B,A] = Diffuse RGB color, shininess.
	// Ignore alpha or transparency.
	vec4 color;
	color.rgb = ((useTexture)? texture( objectTexture, oTexCoords ).rgb * diffuse.rgb : diffuse.rgb);
	color.a = shininess;

	if( drawPoint )
	{
		if( dot( gl_PointCoord - 0.5, gl_PointCoord - 0.5 ) > 0.25 )		// For rounded points.
			discard;
		else
			TexGAlbedoSpecular = color;
	}
	else
		TexGAlbedoSpecular = color;

	// The depth is writtin automatically.
	// gl_FragDepth = gl_FragCoord.z;
}
