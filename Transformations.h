#ifndef Transformations_h
#define Transformations_h

#include <armadillo>

using namespace arma;

class Tx
{
public:
	
	static const vec3 X_AXIS;
	static const vec3 Y_AXIS;
	static const vec3 Z_AXIS;
	
	static mat44 translate( double x, double y, double z );
	static mat44 translate( const vec3& v );
	static mat44 scale( double x, double y, double z );
	static mat44 scale( const vec3& v );
	static mat44 scale( double s );
	static mat44 rotate( double theta, const vec3& axis );
	static mat44 lookAt( const vec3& e, const vec3& p, const vec3& u );
	static mat44 frustrum( double left, double right, double bottom, double top, double near, double far );
	static mat44 perspective( double fovy, double ratio, double near, double far );
	static mat44 ortographic( double left, double right, double bottom, double top, double near, double far );
	static void toOpenGLMatrix( float* destination, const mat& source );
	static mat33 getInvTransModelView( const mat44& MV, bool uniformTransform = true );
};

#endif /* Transformations_h */






