#include "OpenGLGeometry.h"

/**
 * Register group of three points as vertices for a triangle and its normals.
 * The parameters must be given in right-hand order, so that CCW culling can be used in OpenGL.
 * @param a First point.
 * @param b Second point.
 * @param c Third point.
 * @param na First normal.
 * @param nb Second normal.
 * @param nc Third normal.
 */
void OpenGLGeometry::registerTriangle( const vec3& a, const vec3& b, const vec3& c, const vec3& na, const vec3& nb, const vec3& nc )
{
	// Vertices.
	points.push_back( a );
	points.push_back( b );
	points.push_back( c );
	
	// Normals.
	normals.push_back( na );
	normals.push_back( nb );
	normals.push_back( nc );
}

/**
 * Get all vertices coordinates and normals.
 * @param vertices[out] An empty vector to allocate the x, y, and z vertices information.
 * @param normals[out] An empty vector to allocate the x, y, and z normals information.
 * @return Number of 3D points/vertices that were processed.
 */
unsigned int OpenGLGeometry::getData( vector<float>& vertices, vector<float>& normals ) const
{
	size_t N = points.size();
	
	for( int i = 0; i < N; i++ )
	{
		// Vertices.
		vertices.push_back( points[i][0] );			// X-coordinate.
		vertices.push_back( points[i][1] );			// Y-coordinate.
		vertices.push_back( points[i][2] );			// Z-coordinate.
		
		// Normals.
		normals.push_back( this->normals[i][0] );	// X-coordinate.
		normals.push_back( this->normals[i][1] );	// Y-coordinate.
		normals.push_back( this->normals[i][2] );	// Z-coordinate.
	}
	
	return static_cast<unsigned int>(N);
}

/**
 * Builds a cube centered at the origin.
 * @param side Cube side metric.
 */
void OpenGLGeometry::createCube( double side )
{
	double s = side/2.0;
	
	// Front face.
	vec3 p0 = { -s, -s,  s };					//      p7----------p5
	vec3 p1 = {  s, -s,  s };					//      /|          /|
	vec3 p2 = {  s,  s,  s };					//     /           / |
	vec3 p3 = { -s,  s,  s };					//    p3-+--------p2 |
												//    |           |  |
	// Back face.								//    |  |        |  |
	vec3 p4 = {  s, -s, -s };					//    | p6- - - - +-p4
	vec3 p5 = {  s,  s, -s };					//    | /         | /
	vec3 p6 = { -s, -s, -s };					//    |/          |/
	vec3 p7 = { -s,  s, -s };					//    p0----------p1
	
	// Register all vertices in triangles.
	registerTriangle( p0, p1, p2, Tx::Z_AXIS, Tx::Z_AXIS, Tx::Z_AXIS );				// Front face.
	registerTriangle( p2, p3, p0, Tx::Z_AXIS, Tx::Z_AXIS, Tx::Z_AXIS );
	registerTriangle( p1, p4, p2, Tx::X_AXIS, Tx::X_AXIS, Tx::X_AXIS );				// Right face.
	registerTriangle( p4, p5, p2, Tx::X_AXIS, Tx::X_AXIS, Tx::X_AXIS );
	registerTriangle( p4, p6, p5, -Tx::Z_AXIS, -Tx::Z_AXIS, -Tx::Z_AXIS );			// Back face.
	registerTriangle( p6, p7, p5, -Tx::Z_AXIS, -Tx::Z_AXIS, -Tx::Z_AXIS );
	registerTriangle( p0, p3, p7, -Tx::X_AXIS, -Tx::X_AXIS, -Tx::X_AXIS );			// Left face.
	registerTriangle( p0, p7, p6, -Tx::X_AXIS, -Tx::X_AXIS, -Tx::X_AXIS );
	registerTriangle( p2, p5, p7, Tx::Y_AXIS, Tx::Y_AXIS, Tx::Y_AXIS );				// Top face.
	registerTriangle( p7, p3, p2, Tx::Y_AXIS, Tx::Y_AXIS, Tx::Y_AXIS );
	registerTriangle( p1, p6, p4, -Tx::Y_AXIS, -Tx::Y_AXIS, -Tx::Y_AXIS );			// Bottom face.
	registerTriangle( p1, p0, p6, -Tx::Y_AXIS, -Tx::Y_AXIS, -Tx::Y_AXIS );
}

/**
 * Create a unit sphere centered at the origin.
 * @param n Number of recursion levels to approximate the sphere.
 */
void OpenGLGeometry::createSphere( int n )
{
	// Points for the starting tetrahedron.
	vec3 v[4] = {
		vec3( { 0.0, 0.0, 1.0 } ),
		vec3( { 0.0, 0.942809, -0.333333 } ),
		vec3( { -0.816497, -0.471405, -0.333333 } ),
		vec3( { 0.816497, -0.471405, -0.333333 } ) };
	
	divideTriangle( v[0], v[1], v[2], n );
	divideTriangle( v[3], v[2], v[1], n );
	divideTriangle( v[0], v[3], v[1], n );
	divideTriangle( v[0], v[2], v[3], n );
}

/**
 * Recursively, divde a equilateral triangle into four inner triangles.
 * @param a First triangle vertex.
 * @param b Second triangle vertex.
 * @param c Third triangle vertex.
 * @param n recursive level.
 */
void OpenGLGeometry::divideTriangle( const vec3& a, const vec3& b, const vec3& c, int n )
{
	if( n > 0 )
	{
		vec3 v1 = normalise( a + b );
		vec3 v2 = normalise( a + c );
		vec3 v3 = normalise( b + c );
		divideTriangle( a, v2, v1, n-1 );
		divideTriangle( c, v3, v2, n-1 );
		divideTriangle( b, v1, v3, n-1 );
		divideTriangle( v1, v2, v3, n-1 );
	}
	else
		registerTriangle( a, b, c, a, b, c );		// Normals are the same as vertex locations.
}

/**
 * Create a cylinder along the Z axis.
 * The cylinder is created so that its base is located on the XY plane, and it grows along the +Z axis.
 * @param radius The cylinder radius (must be positive).
 * @param length The cylinder length along the +Z axis (must be positive).
 */
void OpenGLGeometry::createCylinder( double radius, double length )
{
	if( radius < 0 )					// Check for correct input parameters.
		radius = 1.0;
	
	if( length < 0 )
		length = 1.0;
	
	const int N = 50;					// Resolution (number of sides to approximate top and bottom circles).
	const double step = 2.0*M_PI/N;
	vec3 P0 = { 0, 0, 0 };
	vec3 P0L = { 0, 0, length };
	vec3 P1 = { radius, 0, 0 };			// Need this point to start the triangle.
	vec3 P1L = P1 + Tx::Z_AXIS*length;	// P1 moved to the other face of the cylinder.
	
	for( int I = 1; I <= N; I++ )
	{
		double angle = I*step;
		vec3 P2 = { radius*cos( angle ), radius*sin( angle ), 0  };
		
		// Register XY0 triangle (order of points is changed to keep the right-hand rule).
		registerTriangle( P0, P2, P1, -Tx::Z_AXIS, -Tx::Z_AXIS, -Tx::Z_AXIS );
		
		vec3 P2L = P2 + Tx::Z_AXIS*length;	// P2 moved to the other face of the cylinder.
		
		// Register XYlength triangle.
		registerTriangle( P0L, P1L, P2L, Tx::Z_AXIS, Tx::Z_AXIS, Tx::Z_AXIS );
		
		// Register a side of the cylinder.
		registerTriangle( P2L, P1L, P1, P2, P1, P1 );		// Lower triangle.
		registerTriangle( P1, P2, P2L, P1, P2, P2 );		// Upper triangle.
		
		P1 = P2;
		P1L = P2L;
	}
}

/**
 * Create a prism along the Z-axis.
 * The prism contains two square pyramids whose bases are glued, perpendicular to Z-axis. Their
 * bases are located within a distance from the origina, along the Z-axis. Thus, the first
 * pyramid's apex is at the origin, and the second pyramid's apex is at the length of the geom, on the +Z axis.
 * @param radius Bases radius for both pyramids.
 * @param length Prism's length along the +Z axis.
 * @param bases Bases position expressed in a percentage value in the range (0,1).
 */
void OpenGLGeometry::createPrism( double radius, double length, double bases )
{
	if( length < 0 )				// Fix input parameters if they are invalid.
		length = 1;
	
	if( radius < 0 )
		radius = 0.5;
	
	if( !(bases > 0 && bases < 1) )
		bases = 0.3;
	
	bases *= length;				// Change bases to something in (0, length).
	
	vec3 PA1 = { 0, 0, 0 };			// Apex for first pyramid.
	vec3 PA2 = { 0, 0, length };	// Apex for second pyramid.
	
	const int N = 4;
	const double step = M_PI/2.0;	// Four sides for each pyramid.
	double angle = -M_PI/4.0;		// Start below the X axis.
	vec3 normal;
	
	vec3 P1 = { radius*cos(angle), radius*sin(angle), bases };
	for( int I = 1; I <= N; I++ )
	{
		angle += step;
		
		vec3 P2 = { radius*cos(angle), radius*sin(angle), bases };
		
		// Register triangle for first pyramid.
		normal = cross( P1-PA1, PA1-P2 );
		registerTriangle( P1, PA1, P2, normal, normal, normal );
		
		// Register triangle for second pyramid.
		normal = cross( P1-P2, P2-PA2 );
		registerTriangle( P1, P2, PA2, normal, normal, normal );
		
		P1 = P2;
		
	}
}