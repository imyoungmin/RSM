#ifndef OpenGLGeometry_h
#define OpenGLGeometry_h

#include <vector>
#include <armadillo>
#include "Transformations.h"

using namespace std;
using namespace arma;

class OpenGLGeometry
{
private:
	vector<vec3> points;			// Store 3D coordinates of geometry vertices.
	vector<vec3> normals;			// Store 3D coordinates of normal vectors to each vertex.
	
	void registerTriangle( const vec3& a, const vec3& b, const vec3& c, const vec3& na, const vec3& nb, const vec3& nc );
	void divideTriangle( const vec3& a, const vec3& b, const vec3& c, int n );
	
public:
	
	unsigned int getData( vector<float>& vertices, vector<float>& normals ) const;
	void createCube( double side = 1.0 );
	void createSphere( int n = 6 );
	void createCylinder( double radius = 1.0, double length = 1.0 );
	void createPrism( double radius = 1.0, double length = 1, double bases = 0.3 );
};

#endif /* OpenGLGeometry_h */
