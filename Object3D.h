#ifndef OPENGL_OBJECT3D_H
#define OPENGL_OBJECT3D_H

#include <string>
#include <iostream>
#include <OpenGL/gl3.h>
#include <armadillo>
#include "stb_image.h"

#include "Configuration.h"

using namespace std;
using namespace arma;

/**
 * This class holds rendering information for a 3D model loaded from an .obj file.
 */
class Object3D
{
private:
	string kind;							// Object type (should be unique for multiple kinds of objects in a scene).
	GLuint bufferID;						// Buffer ID given by OpenGL.
	GLuint textureID;						// Texture ID is user creates object with a texture.
	GLsizei verticesCount;					// Number of vertices stored in buffer.
	bool withTexture;						// Does the object have an enabled texture?

	GLsizei getData( const vector<vec3>& inVs, const vector<vec2>& inUVs, const vector<vec3>& inNs, vector<float>& outVs, vector<float>& outUVs, vector<float>& outNs ) const;

public:
	Object3D();
	Object3D( const char* type, const char* filename, const char* textureFilename = nullptr );
	void loadOBJ( const char* filename, vector<vec3 >& outVertices, vector<vec2>& outUVs, vector<vec3>& outNormals ) const;
	GLuint getBufferID() const;
	GLsizei getVerticesCount() const;
	GLuint getTextureID() const;
	bool hasTexture() const;
};


#endif //OPENGL_OBJECT3D_H
