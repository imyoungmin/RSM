//
//  Light.cpp
//  RTRendering
//
//  Created by Im YoungMin on 1/29/19.
//  Copyright © 2019 Im YoungMin. All rights reserved.
//

#include "Light.h"

/**
 * Light constructor.
 * @param p Position.
 * @param c Color triplet -- RGB.
 * @param P The 4x4 light projection matrix.
 * @param unit Shadow map index unit (for texture).
 */
Light::Light( const vec3& p, const vec3& c, const mat44& P, int unit )
{
	position = vec3( p );
	lY = position[1];																				// Build light components from its initial value.
	lXZRadius = sqrt( position[0]*position[0] + position[2]*position[2] );
	lAngle = atan2( position[0], position[2] );
	color = { fmax(0.0, fmin(c[0], 1.0)), fmax(0.0, fmin(c[1], 1.0)), fmax(0.0, fmin(c[2], 1.0)) };	// Check color components.
	Projection = mat44( P );
	lUnit = unit;
}

/**
 * Rotate light around y-axis.
 * @param angle Amount of rotation in radians.
 */
void Light::rotateBy( float angle )
{
	lAngle += angle;
	position = { lXZRadius * sin( lAngle ), lY, lXZRadius * cos( lAngle ) };						// New position.
}

/**
 * Get shader unique unit index (or suffix).
 * @return Unit.
 */
int Light::getUnit() const
{
	return lUnit;
}
