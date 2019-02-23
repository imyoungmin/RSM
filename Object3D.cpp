#include "Object3D.h"

/**
 * Default constructor.
 */
Object3D::Object3D() = default;

/**
 * 3D model constructor.
 * @param type Unique kind name for this model.
 * @param filename OBJ filename.
 * @param textureFilename Texture image file name: nullptr to not use texture.
 */
Object3D::Object3D( const char* type, const char* filename, const char* textureFilename )
{
	kind = string( type );
	withTexture = false;

	// Load the 3D model from the provided filename.
	cout << "Loading 3D model \"" << kind << "\" from file: \"" << filename << "\"... " << endl;
	vector<vec3> vertices, normals;		// Output vectors.
	vector<vec2> uvs;
	loadOBJ( filename, vertices, uvs, normals );

	// Allocate a buffer and load data into it.
	glGenBuffers( 1, &(bufferID) );
	glBindBuffer( GL_ARRAY_BUFFER, bufferID );
	vector<float> vertexPositions;
	vector<float> textureCoordinates;
	vector<float> normalComponents;
	verticesCount = getData( vertices, uvs, normals, vertexPositions, textureCoordinates, normalComponents );

	// Allocate space for vertex and texture coordinates.
	const size_t size3D = sizeof(float) * vertexPositions.size();							// Size of positions arrays in bytes.
	const size_t sizeUV = sizeof(float) * textureCoordinates.size();
	glBufferData( GL_ARRAY_BUFFER, 2 * size3D + sizeUV, nullptr, GL_STATIC_DRAW );
	glBufferSubData( GL_ARRAY_BUFFER, 0, size3D, vertexPositions.data() );					// Copy positions.
	glBufferSubData( GL_ARRAY_BUFFER, size3D, size3D, normalComponents.data() );			// Copy normals.
	if( textureFilename != nullptr )
	{
		glBufferSubData( GL_ARRAY_BUFFER, 2 * size3D, sizeUV, textureCoordinates.data() );	// Copy texture coords.
		
		// Create texture, which will be attached to unit GL_TEXTURE1.
		glGenTextures( 1, &textureID );
		glBindTexture( GL_TEXTURE_2D, textureID );
		
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );				// Set the texture wrapping/filtering options (on the currently bound texture object).
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		int width, height, nrChannels;														// Load and generate the texture.
		string fullTextureFilename = conf::OBJECTS_FOLDER + string( textureFilename );
		stbi_set_flip_vertically_on_load( true );											// Flip the y-axis.
		unsigned char *data = stbi_load( fullTextureFilename.c_str(), &width, &height, &nrChannels, 0 );
		if( data )
		{
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, (nrChannels == 4)? GL_RGBA: GL_RGB, GL_UNSIGNED_BYTE, data );		// This works for PNG and JPEG textures.
			glGenerateMipmap( GL_TEXTURE_2D );
			withTexture = true;																// Object can now use texture for rendering.
		}
		else
		{
			cerr << "Failed to load texture for object " << kind << endl;
			exit( EXIT_FAILURE );
		}
		stbi_image_free( data );
		cout << "Finished loading " << kind << "'s texture!" << endl;
	}
}

/**
 * Read the 3D object vertices, uv coordinates, and vector normals.
 * @param filename 3D object filename.
 * @param outVertices Vector of 3D vertices.
 * @param outUVs Vector of 2D texture coordinates.
 * @param outNormals Vector of 3D normals.
 */
void Object3D::loadOBJ( const char* filename, vector<vec3 >& outVertices, vector<vec2>& outUVs, vector<vec3>& outNormals ) const
{
	vector<int> vertexIndices, uvIndices, normalIndices;	// Auxiliary variables.
	vector<vec3> temp_vertices;
	vector<vec2> temp_uvs;
	vector<vec3> temp_normals;
	size_t nFaces = 0;
	
	string fullFileName = conf::OBJECTS_FOLDER + string( filename );
	FILE* file = fopen( fullFileName.c_str(), "r" );
	if( file == NULL )
	{
		cerr << "Unable to open file " << filename << endl;
		exit( EXIT_FAILURE );
	}
	
	while( true )
	{
		char lineHeader[512];
		char buffer[512];
		
		// Read the first word of the line.
		int res = fscanf( file, "%s", lineHeader );
		if( res == EOF )
			break; 			// EOF = End Of File. Quit the loop.
		
		// Else: Parse lineHeader.
		float x, y, z;
		if( strcmp( lineHeader, "v" ) == 0 )				// A vertex? v -1.000000 1.000000 -1.000000
		{
			fscanf( file, "%f %f %f\n", &x, &y, &z );
			vec3 vertex = { x, y, z };
			temp_vertices.push_back( vertex );
			
		}
		else if( strcmp( lineHeader, "vt" ) == 0 )			// Texture coordinate? vt 0.748953 0.250920
		{
			fscanf( file, "%f %f\n", &x, &y );
			vec2 uv = { x, y };
			temp_uvs.push_back( uv );
			
		}
		else if( strcmp( lineHeader, "vn" ) == 0 )			// A normal vector? vn -0.000000 -1.000000 0.000000
		{
			fscanf( file, "%f %f %f\n", &x, &y, &z );
			vec3 normal = { x, y, z };
			temp_normals.push_back(normal);
			
		}
		else if( strcmp( lineHeader, "f" ) == 0 ) 			// A face? f 5/1/1 1/2/1 4/3/1
		{
			string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches;
			if( !temp_uvs.empty() )		// Textures provided?
				matches = fscanf( file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
								 &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2] );
			else
				matches = fscanf( file, "%d//%d %d//%d %d//%d\n",
								 &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2] );
			
			if( ( matches != 9 && !temp_uvs.empty() ) || ( matches != 6 && temp_uvs.empty() ) )
			{
				cerr << "File can't be read by our simple parser: Try exporting with other options" << endl;
				exit( EXIT_FAILURE );
			}
			
			vertexIndices.push_back( vertexIndex[0] );		// Loading vertex information.
			vertexIndices.push_back( vertexIndex[1] );
			vertexIndices.push_back( vertexIndex[2] );
			if( !temp_uvs.empty() )
			{
				uvIndices.push_back( uvIndex[0] );			// Loading texture coordinates (if given).
				uvIndices.push_back( uvIndex[1] );
				uvIndices.push_back( uvIndex[2] );
			}
			normalIndices.push_back( normalIndex[0] );		// Loading normals information.
			normalIndices.push_back( normalIndex[1] );
			normalIndices.push_back( normalIndex[2] );
			
			nFaces++;			
		}
		else
		{
			if( !fgets( buffer, 128, file ) )				// No matching headliner?  Finish reading the whole line.
				break;										// Reached end of file?
		}
	}
	
	fclose( file );
	
	if( uvIndices.size() != nFaces * 3 )					// Did we read an inconsistent number of UV texture indices?
	{
		cout << "WARNING! The UV information is incomplete or missing -- it'll be ignored" << endl;
		uvIndices.clear();
		temp_uvs.clear();
	}
		
	// For each vertex of each triangle
	for( unsigned int i = 0; i < vertexIndices.size(); i++ )
	{
		unsigned int vertexIndex = vertexIndices[i];		// Vertices.
		vec3 vertex = temp_vertices[vertexIndex - 1];
		outVertices.push_back( vertex );
		
		if( !temp_uvs.empty() )
		{
			unsigned int uvIndex = uvIndices[i];			// UV coordinates.
			vec2 uv = temp_uvs[uvIndex - 1];
			outUVs.push_back( uv );
		}
		
		unsigned int normalIndex = normalIndices[i];		// Normals.
		vec3 normal = temp_normals[normalIndex - 1];
		outNormals.push_back( normal );
	}
	
	cout << "Finished loading " << nFaces << " triangles!" << endl;
}

/**
 * Retrieve the buffer ID, which contains the rendering information for this kind of 3D object model.
 * @return OpenGL Buffer ID.
 */
GLuint Object3D::getBufferID() const
{
	return bufferID;
}

/**
 * Retrieve the texture ID.
 * @return OpengGL texture ID.
 */
GLuint Object3D::getTextureID() const
{
	return textureID;
}

/**
 * Retrieve the number of vertices for this 3D object model.
 * @return Number of vertices.
 */
GLsizei Object3D::getVerticesCount() const
{
	return verticesCount;
}

/**
 * Does the object have a texture?
 * @return True if a texture exists for this object, false otherwise.
 */
bool Object3D::hasTexture() const
{
	return withTexture;
}

/**
 * Collect the vertex, uv, and normal coordinates into linear vectors of scalars.
 * @param inVs Input 3D vertex positions.
 * @param inUVs Input 2D texture coordinates.
 * @param inNs Input 3D vertex normals.
 * @param outVs Flat x, y, and z vertex coordinates.
 * @param outUVs Flat u and v texture coordinates per vertex.
 * @param outNs Flat x, y, and z components of the normal vectors.
 * @return Number of processed vertices.
 */
GLsizei Object3D::getData( const vector<vec3>& inVs, const vector<vec2>& inUVs, const vector<vec3>& inNs, vector<float>& outVs, vector<float>& outUVs, vector<float>& outNs ) const
{
	GLsizei N = static_cast<GLsizei>( inVs.size() );

	for( int i = 0; i < N; i++ )
	{
		// Vertices.
		outVs.push_back( inVs[i][0] );			// X-coordinate.
		outVs.push_back( inVs[i][1] );			// Y-coordinate.
		outVs.push_back( inVs[i][2] );			// Z-coordinate.

		// Texture coordinates (if existent).
		if( !inUVs.empty() )
		{
			outUVs.push_back( inUVs[i][0] );	// U-coordinate.
			outUVs.push_back( inUVs[i][1] );	// V-coordinate.
		}

		// Normals.
		outNs.push_back( inNs[i][0] );			// X-coordinate.
		outNs.push_back( inNs[i][1] );			// Y-coordinate.
		outNs.push_back( inNs[i][2] );			// Z-coordinate.
	}

	return N;
}
