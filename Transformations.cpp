#include "Transformations.h"

/////////////////////////////////////////////////////// Constants //////////////////////////////////////////////////////

const vec3 Tx::X_AXIS = { 1, 0, 0 };
const vec3 Tx::Y_AXIS = { 0, 1, 0 };
const vec3 Tx::Z_AXIS = { 0, 0, 1 };

/**
 * Translation, scalar version.
 */
mat44 Tx::translate( double x, double y, double z )
{
	mat44 T = eye< mat >( 4, 4 );
	T(0,3) = x;
	T(1,3) = y;
	T(2,3) = z;
	
	return T;
}

/**
 * Translation, vector version.
 */
mat44 Tx::translate( const vec3& v )
{
	return translate( v[0], v[1], v[2] );
}

/**
 * Scaling, scalars version.
 */
mat44 Tx::scale( double x, double y, double z )
{
	mat44 S = eye< mat >( 4, 4 );
	S(0,0) = x;
	S(1,1) = y;
	S(2,2) = z;
	
	return S;
}

/**
 * Scaling, vector version.
 */
mat44 Tx::scale( const vec3& v )
{
	return scale( v[0], v[1], v[2] );
}

/**
 * Scaling, one-scalar version.
 */
mat44 Tx::scale( double s )
{
	return scale( s, s, s );
}

/**
 * Rotation, axis-angle, vec3 version.
 */
mat44 Tx::rotate( double theta, const vec3& axis )
{
	vec3 u = normalise( axis );				// Normalize rotation axis.
	const double cosTheta = cos( theta );
	const double sinTheta = sin( theta );
	
	double x = u[0];
	double y = u[1];
	double z = u[2];
	
	// Cross-product matrix.
	mat33 C = { {  0, -z,  y },
		{  z,  0, -x },
		{ -y,  x,  0 } };
	
	// Tensor-product matrix.
	mat33 T = { { x*x, x*y, x*z },
		{ x*y, y*y, y*z },
		{ x*z, y*z, z*z } };
	
	mat R = cosTheta*eye< mat >(3,3) + sinTheta*C + (1-cosTheta)*T;
	
	mat44 RR = eye< mat >(4,4);
	RR( span(0,2), span(0,2) ) = R;			// Set upper 3x3 matrix with the rotation matrix.
	
	return RR;
}

/**
 * View matrix: Look at.
 * @param e Viewer's eye position.
 * @param p Point of interest.
 * @param u Up vector
 */
mat44 Tx::lookAt( const vec3& e, const vec3& p, const vec3& u )
{
	vec3 z = normalise( e - p );			// Forward vector.
	vec3 x = normalise( cross( u, z ) );	// Sideways vector.
	vec3 y = cross( z, x );					// Normalized up vector.
	
	mat44 M = { { x[0], y[0], z[0], 0.0 },
		{ x[1], y[1], z[1], 0.0 },
		{ x[2], y[2], z[2], 0.0 },
		{ -dot(x,e), -dot(y,e), -dot(z,e), 1.0 } };
	
	return M.t();
}

/**
 * Perspective matrix: frustrum.
 */
mat44 Tx::frustrum( double left, double right, double bottom, double top, double near, double far )
{
	if( right == left || top == bottom || near == far || near < 0.0 || far < 0.0 )
		return eye<mat>( 4,4 );
	
	mat44 M = { { 2.0*near/(right-left),          0.0,          (right+left)/(right-left),           0.0           },
		{         0.0,           2.0*near/(top-bottom), (top+bottom)/(top-bottom),           0.0           },
		{         0.0,                    0.0,            (near+far)/(near-far),   2.0*near*far/(near-far) },
		{         0.0,                    0.0,                    -1.0,                       0.0          } };
	
	return M;
}

/**
 * Perspective matrix: symmetric frustrum.
 */
mat44 Tx::perspective( double fovy, double ratio, double near, double far )
{
	double q =  1.0/( fovy/2.0 );
	double a = q / ratio;
	double b = far/(near-far);
	double c = near*far/(near-far);
	
	mat44 M = { {  a,  0.0,  0.0, 0.0 },
		{ 0.0,  q,   0.0, 0.0 },
		{ 0.0, 0.0,   b,   c  },
		{ 0.0, 0.0, -1.0, 0.0 } };
	
	return M;
}

/**
 * Orthographic projection.
 */
mat44 Tx::ortographic( double left, double right, double bottom, double top, double near, double far )
{
	if( right == left || top == bottom || near == far || near < 0.0 || far < 0.0 )
		return eye<mat>( 4,4 );
	
	mat44 M = { { 2.0/(right-left),       0.0,              0.0,      -(left+right)/(right-left) },
				{      0.0,         2.0/(top-bottom),       0.0,      -(bottom+top)/(top-bottom) },
				{      0.0,               0.0,        -2.0/(far-near),   -(far+near)/(far-near)  },
				{      0.0,               0.0,              0.0,                  1.0            } };
	
	return M;
}

/**
 * To OpenGL matrix form.
 */
void Tx::toOpenGLMatrix( float* destination, const mat& source )
{
	for( int c = 0; c < source.n_cols; c++ )
		for( int r = 0; r < source.n_rows; r++ )
		{
			*destination = static_cast<float>(source(r,c));
			destination++;
		}
}

/**
 * Get the inverse transpose of the 3x3 principal submatrix of the model view matrix.
 * @param MV The model-view matrix.
 * @return Desired inverse transpose.
 */
mat33 Tx::getInvTransModelView( const mat44& MV, bool uniformTransform )
{
	mat33 Upper3x3( MV.submat( 0, 0, size( 3, 3 ) ) );
	if( uniformTransform )
		return Upper3x3;
	mat33 Q, R;
 	qr( Q, R, Upper3x3 );
	return Q * inv( R ).t();
}












