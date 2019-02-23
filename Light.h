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
	
public:
	vec3 position;				// 3D world light location.
	vec3 color;					// Color in RGB.
	mat44 Projection;			// Projection matrix.
	mat44 SpaceMatrix;			// Product of Light Projection * Light View.
	
	GLuint rsmFBO;				// OpenGL frame buffer object for the reflective shadow map.
	GLuint rsmPosition;			// Texture ID for world space positions.
	GLuint rsmNormal;			// Texture ID for world space normals.
	GLuint rsmFlux;				// Texture ID for the flux (= material's albedo times light color).
	GLuint rsmDepth;			// Texture ID for depth (= same used in shadow mapping).

	Light();
	Light( const vec3& p, const vec3& c, const mat44& P );
	void rotateBy( float angle );
};

#endif /* Light_h */
