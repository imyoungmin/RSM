//
//  Light.h
//  RTRendering
//
//  Created by Im YoungMin on 1/29/19.
//  Copyright © 2019 Im YoungMin. All rights reserved.
//

#ifndef Light_h
#define Light_h

#include <armadillo>
#include <OpenGL/gl3.h>

using namespace arma;

/**
 * Light object and related properties.
 */
class Light
{
private:
	float lY;					// Starting height.
	float lXZRadius;			// Pole distance of light on the xz-plane.
	float lAngle;				// Angle with respect to +z in the xz-plane.
	int lUnit;					// Unique unit index associated with its entries in the shader.
	
public:
	vec3 position;				// 3D world light location.
	vec3 color;					// Color in RGB.
	mat44 Projection;			// Projection matrix.
	mat44 SpaceMatrix;			// Product of Light Projection * Light View.
	
	GLuint shadowMapFBO;		// OpenGL shading objects for the shadow map (a.k.a. depth map).
	GLuint shadowMapTextureID;	// Texture ID associated with shadow map.
	GLint shadowMapLocation;	// Location of shadow map 2D samples in fragment shader.
	
	Light( const vec3& p, const vec3& c, const mat44& P, int unit );
	void rotateBy( float angle );
	int getUnit() const;
};

#endif /* Light_h */
