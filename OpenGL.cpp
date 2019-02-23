#include "OpenGL.h"

/**
 * Constructor.
 */
OpenGL::OpenGL(){}

/**
 * Release resources.
 */
OpenGL::~OpenGL()
{
	glDeleteVertexArrays( 1, &vao );
	glDeleteProgram( glyphsProgram );
}

/**
 * Initialize the OpenGL object.
 */
void OpenGL::init()
{
	// Create vertex array object.
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );
	
	// Initialize glyphs via FreeType.
	initGlyphs();
}

/**
 * Initialize OpenGL objects for glyphs.
 */
void OpenGL::initGlyphs()
{
	// Initialize the FreeType2 library.
	if( FT_Init_FreeType( &ft ) )
	{
		cerr << "Could not init FreeType Library" << endl;
		exit( EXIT_FAILURE );
	}

	// Create the font face object.
	FT_Error ft_error = FT_New_Face( ft, string(conf::FONTS_FOLDER + "ubuntumonob.ttf").c_str(), 0, &face );
	if( ft_error != FT_Err_Ok )
	{
		cerr << "Could not open font!" << endl;
		exit( EXIT_FAILURE );
	}

	// Initialize shaders for glyphs drawing program.
	Shaders shaders;
	cout << "Initializing glyph shaders... " << endl;
	glyphsProgram = shaders.compile( conf::SHADERS_FOLDER + "glyphs.vert", conf::SHADERS_FOLDER + "glyphs.frag" );
	if( glyphsProgram == 0 )
	{
		cerr << "Failed to compile glyphs shaders proglram!" << endl;
		exit( EXIT_FAILURE );
	}

	GLint attribute_coord = glGetAttribLocation( glyphsProgram, "coord" );	// Reading locations off the shader.
	GLint uniform_tex = glGetUniformLocation( glyphsProgram, "tex" );
	GLint uniform_color = glGetUniformLocation( glyphsProgram, "color" );

	if( attribute_coord == -1 || uniform_tex == -1 || uniform_color == -1 )
	{
		cerr << "Failed to read off shader locations!" << endl;
		exit( EXIT_FAILURE );
	}

	glGenBuffers( 1, &glyphsBufferID );		// Create the vertex buffer object

	// Create texture atlasses for several font sizes.
	atlas48 = new Atlas( face, 48, uniform_color, attribute_coord, uniform_color );
	atlas24 = new Atlas( face, 24, uniform_color, attribute_coord, uniform_color );
	atlas12 = new Atlas( face, 12, uniform_color, attribute_coord, uniform_color );

	// Done with freetype: free resources.
	FT_Done_Face( face );
	FT_Done_FreeType( ft );

	cout << "Done!" << endl;
}

/**
 * Change material color.
 * @param r Red component in [0,1].
 * @param g Green component in [0,1].
 * @param b Blue component in [0,1].
 * @param a Alpha value in [0,1].
 * @param shininess Power value for specular component: negative to turn specular off; value in [-inf, 128].
 */
void OpenGL::setColor( float r, float g, float b, float a, float shininess )
{
	// Clamp components to valid values.
	r = fmax( 0.0f, fmin( r, 1.0f ) );
	g = fmax( 0.0f, fmin( g, 1.0f ) );
	b = fmax( 0.0f, fmin( b, 1.0f ) );
	a = fmax( 0.0f, fmin( a, 1.0f ) );

	material.diffuse = { r, g, b, a };
	material.ambient = material.diffuse * 0.1;
	material.specular[3] = material.ambient[3] = a;
	material.shininess = fmin( shininess, 128.0f );
}

/**
 * Draw a unit cube at the origin.
 * Creates a unit cube at the origin. If the cube is not yet created, it fills out a buffer and
 * leaves its vertex and normal data there for future calls, making it drawing in OpenGL more efficient.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera transformation matrix.
 * @param Model The 4x4 model transformation matrix.
 */
void OpenGL::drawCube( const mat44& Projection, const mat44& Camera, const mat44& Model )
{
	drawGeom( Projection, Camera, Model, &cube, CUBE );
}

/**
 * Draw a unit sphere at the origin.
 * Creates a unit sphere at the origin. If the sphere is not yet created, it fills out a buffer and
 * leaves its vertex and normal data there for future calls, making it drawing in OpenGL more efficient.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera transformation matrix.
 * @param Model The 4x4 model transformation matrix.
 */
void OpenGL::drawSphere( const mat44& Projection, const mat44& Camera, const mat44& Model )
{
	drawGeom( Projection, Camera, Model, &sphere, SPHERE );
}

/**
 * Draw a unit-length cylinder, with unit radius, from z=0 to z=1.
 * Creates a unit cylinder. If the cylinder is not yet created, it fills out a buffer and
 * leaves its vertex and normal data there for future calls, making it drawing in OpenGL more efficient.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera transformation matrix.
 * @param Model The 4x4 model transformation matrix.
 */
void OpenGL::drawCylinder( const mat44& Projection, const mat44& Camera, const mat44& Model )
{
	drawGeom( Projection, Camera, Model, &cylinder, CYLINDER );
}

/**
 * Draw a unit prism.
 * Creates a 8-sided prism whose first apex is at the orgin, and the second apex is at (0,0,1).
 * The pyramid bases join at the plane z=0.3 and consist of a square inscribed in
 * a circle of unit radius. If the prism is not yet created, it fills out a buffer and leaves
 * its vertex and normal data there for future calls, making it drawing in OpenGL more efficient.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera transformation matrix.
 * @param Model The 4x4 model transformation matrix.
 */
void OpenGL::drawPrism( const mat44& Projection, const mat44& Camera, const mat44& Model )
{
	drawGeom( Projection, Camera, Model, &prism, PRISM );
}

/**
 * Draw an open path.
 * This function combines the functionality of the private function drawGeometry with the public draw* functions,
 * but for a path, which doesn't consider the normals.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera matrix.
 * @param Model The 4x4 model transformation matrix.
 * @param vertices A vector of vec3 elements containing position information.
 */
void OpenGL::drawPath( const mat44& Projection, const mat44& Camera, const mat44& Model, const vector<vec3>& vertices )
{
	if( material.ambient[3] < 1.0 )		// If alpha channel in current material color is not fully opaque, enable blending for transparency.
	{
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}

	auto posL = setSequenceInformation( Projection, Camera, Model, vertices );		// Prepare drawing by sending shading information to shaders.

	// Draw connected line segments.
	if( posL >= 0 )
	{
		glDrawArrays( GL_LINE_STRIP, 0, path->verticesCount );
		
		// Disable vertex attribute array position we sent in the setSequenceInformation function.
		glDisableVertexAttribArray( posL );
	}

	if( material.ambient[3] < 1.0 )		// Restore blending if necessary.
		glDisable( GL_BLEND );
};

/**
 * Draw a sequence of points.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera matrix.
 * @param Model The 4x4 model transformation matrix.
 * @param vertices A vector of vec3 elements containing vertex positions.
 * @param size Pixel size for points.
 */
void OpenGL::drawPoints( const mat44& Projection, const mat44& Camera, const mat44& Model, const vector<vec3>& vertices, float size )
{
	if( size < 0 )
		size = 10.0;

	if( material.ambient[3] < 1.0 )		// If alpha channel in current material color is not fully opaque, enable blending for transparency.
	{
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}

	auto posL = setSequenceInformation( Projection, Camera, Model, vertices );		// Prepare drawing by sending shading information to shaders.
	if( posL >= 0 )
	{
		// Overriding the point size set by the sendShadingInformation() function in vertex shader.
		int pointSize_location = glGetUniformLocation( renderingProgram, "pointSize" );
		if( pointSize_location >= 0 )
			glUniform1f( pointSize_location, size );
		
		// Specify we are drawing a point --setSequenceInformation (via sendShadingInformation) sent a false, but here we'll override it with a 1.
		int drawPoint_location = glGetUniformLocation( renderingProgram, "drawPoint" );
		if( drawPoint_location >= 0 )
			glUniform1i( drawPoint_location, true );
		
		glEnable( GL_PROGRAM_POINT_SIZE );
		glDrawArrays( GL_POINTS, 0, path->verticesCount );
		glDisable( GL_PROGRAM_POINT_SIZE );
		
		// Disable vertex attribute array position we sent in the setSequenceInformation function.
		glDisableVertexAttribArray( posL );
	}

	if( material.ambient[3] < 1.0 )		// Restore blending mode.
		glDisable( GL_BLEND );
};

/**
 * Auxiliary function to draw any geometry.
 * This function considers the right geometry buffer has been bound (active) and executes all
 * necessary commands to finish drawing the active geometry.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera matrix.
 * @param Model The 4x4 model transformation matrix.
 * @param G A pointer to the geometry data structure.
 * @param t Type of geometry to be drawn.
 */
void OpenGL::drawGeom( const mat44& Projection, const mat44& Camera, const mat44& Model, GeometryBuffer** G, GeometryTypes t )
{
	if( material.ambient[3] < 1.0 )		// If alpha channel in current material color is not fully opaque, enable blending.
	{
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}

	if( *G == nullptr )					// No data yet loaded into the buffer?
	{
		*G = new GeometryBuffer();
		glGenBuffers( 1, &((*G)->bufferID) );
		glBindBuffer( GL_ARRAY_BUFFER, (*G)->bufferID );
		
		OpenGLGeometry geom;
		switch( t )						// Create a geometry vertices and normals according to requested type.
		{
			case CUBE: geom.createCube(); break;
			case SPHERE: geom.createSphere(); break;
			case CYLINDER: geom.createCylinder(); break;
			case PRISM: geom.createPrism(); break;
		}

		vector<float> vertexPositions;
		vector<float> normals;
		(*G)->verticesCount = geom.getData( vertexPositions, normals );
		
		// Allocate space for the buffer.
		const size_t size = sizeof(float) * vertexPositions.size();				// Size of arrays in bytes.
		glBufferData( GL_ARRAY_BUFFER, 2 * size, nullptr, GL_STATIC_DRAW );
		glBufferSubData( GL_ARRAY_BUFFER, 0, size, vertexPositions.data() );	// Copy actual position and normal data.
		glBufferSubData( GL_ARRAY_BUFFER, size, size, normals.data() );
	}
	else									// Data is already there; just make geom bufferID the active buffer.
		glBindBuffer( GL_ARRAY_BUFFER, (*G)->bufferID );
	
	// Set up our vertex attributes.
	int position_location = glGetAttribLocation( renderingProgram, "position" );
	int normal_location = glGetAttribLocation( renderingProgram, "normal" );
	if( position_location >= 0 )
	{
		glEnableVertexAttribArray( position_location );
		glVertexAttribPointer( position_location, ELEMENTS_PER_VERTEX, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET( 0 ) );
		
		if( normal_location >= 0 )
		{
			glEnableVertexAttribArray( normal_location );
			size_t offset = sizeof(float) * (*G)->verticesCount * ELEMENTS_PER_VERTEX;
			glVertexAttribPointer( normal_location, ELEMENTS_PER_VERTEX, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET( offset ) );
		}
		
		sendShadingInformation( Projection, Camera, Model, true );
		
		// Draw triangles.
		glDrawArrays( GL_TRIANGLES, 0, (*G)->verticesCount );
		
		// Disable attributes.
		glDisableVertexAttribArray( position_location );
		if( normal_location >= 0 )
			glDisableVertexAttribArray( normal_location );
	}

	if( material.ambient[3] < 1.0 )
		glDisable( GL_BLEND );
}

/**
 * Send shading information to GPU.
 * @param Projection 4x4 Projection matrix.
 * @param Camera 4x4 Camera matrix.
 * @param Model 4x4 Model matrix.
 * @param usingBlinnPhong Whether use phong model of flat coloring of geoms.
 * @param usingTexture Whether to render with just colors or with a loaded texture (usually for 3D object models).
 */
void OpenGL::sendShadingInformation( const mat44& Projection, const mat44& Camera, const mat44& Model, bool usingBlinnPhong, bool usingTexture )
{
	// Send the model, view, projection, and light space matrices (if they exist).
	int model_location = glGetUniformLocation( renderingProgram, "Model" );
	int view_location = glGetUniformLocation( renderingProgram, "View");
	int proj_location = glGetUniformLocation( renderingProgram, "Projection" );
	int itmv_location = glGetUniformLocation( renderingProgram, "InvTransModelView" );
	
	if( model_location >= 0 )			// Send model matrix only if shaders have corresponding receptor.
	{
		float model_matrix[ELEMENTS_PER_MATRIX];
		Tx::toOpenGLMatrix( model_matrix, Model );
		glUniformMatrix4fv( model_location, 1, GL_FALSE, model_matrix );
	}
	
	if( view_location >= 0 )			// Send view matrix only if shaders have corresponding receptor.
	{
		float view_matrix[ELEMENTS_PER_MATRIX];
		Tx::toOpenGLMatrix( view_matrix, Camera );
		glUniformMatrix4fv( view_location, 1, GL_FALSE, view_matrix );
	}
	
	if( proj_location >= 0 )			// Send projection matrix only if shaders have corresponding receptor.
	{
		float proj_matrix[ELEMENTS_PER_MATRIX];
		Tx::toOpenGLMatrix( proj_matrix, Projection );
		glUniformMatrix4fv( proj_location, 1, GL_FALSE, proj_matrix );
	}

	if( usingBlinnPhong && itmv_location >= 0 )
	{
		float itmv_matrix[9];
		mat33 InvTransMV = Tx::getInvTransModelView( Camera * Model, usingUniformScaling );		// The inverse transpose of the upper left 3x3 matrix in the Model View matrix.
		Tx::toOpenGLMatrix( itmv_matrix, InvTransMV );
		glUniformMatrix3fv( itmv_location, 1, GL_FALSE, itmv_matrix );
	}

	// Specify if we will use phong lighting model.
	int useBlinnPhong_location = glGetUniformLocation( renderingProgram, "useBlinnPhong" );
	if( useBlinnPhong_location >= 0 )
		glUniform1i( useBlinnPhong_location, usingBlinnPhong );

	// Specify we are not drawing points.
	int drawPoint_location = glGetUniformLocation( renderingProgram, "drawPoint" );
	if( drawPoint_location >= 0 )
		glUniform1i( drawPoint_location, false );
	
	// Specify if we'll use texture as diffuse component in fragment shader.
	int useTexture_location = glGetUniformLocation( renderingProgram, "useTexture" );
	if( useTexture_location != -1 )
		glUniform1i( useTexture_location, usingTexture );

	// Set up material shading.
	int shininess_location = glGetUniformLocation( renderingProgram, "shininess" );
	if( shininess_location >= 0 )
		glUniform1f( shininess_location, material.shininess );

	int ambient_location = glGetUniformLocation( renderingProgram, "ambient" );
	int diffuse_location = glGetUniformLocation( renderingProgram, "diffuse" );
	int specular_location = glGetUniformLocation( renderingProgram, "specular" );
	
	if( ambient_location >= 0 )
	{
		float ambient_vector[HOMOGENEOUS_VECTOR_SIZE];
		Tx::toOpenGLMatrix( ambient_vector, material.ambient );
		glUniform4fv( ambient_location, 1, ambient_vector );
	}

	if( diffuse_location >= 0 )
	{
		float diffuse_vector[HOMOGENEOUS_VECTOR_SIZE];
		Tx::toOpenGLMatrix( diffuse_vector, material.diffuse );
		glUniform4fv( diffuse_location, 1, diffuse_vector );
	}
	
	if( specular_location >= 0 )
	{
		float specular_vector[HOMOGENEOUS_VECTOR_SIZE];
		Tx::toOpenGLMatrix( specular_vector, material.specular );
		glUniform4fv( specular_location, 1, specular_vector );
	}
}

/**
 * Set sequence of vertices information for a path.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera matrix.
 * @param Model The 4x4 model transformation matrix.
 * @param vertices A vector of 3D vertices.
 * @return The position attribute location in shader, so that the pointer can be disabled in the caller.
 */
GLint OpenGL::setSequenceInformation( const mat44& Projection, const mat44& Camera, const mat44& Model, const vector<vec3>& vertices )
{
	if( path == nullptr )									// We haven't used this buffer before? Create it.
	{
		path = new GeometryBuffer;
		glGenBuffers( 1, &(path->bufferID) );
	}
	glBindBuffer( GL_ARRAY_BUFFER, path->bufferID );		// Make path buffer the current one.

	// Load vertices and (virtually no) normals.
	path->verticesCount = static_cast<GLuint>( vertices.size() );
	const int totalElements = ELEMENTS_PER_VERTEX * path->verticesCount;
	float vertexPositions[totalElements];
	for( int i = 0; i < path->verticesCount; i++ )
	{
		for( int j = 0; j < ELEMENTS_PER_VERTEX; j++ )
			vertexPositions[ELEMENTS_PER_VERTEX * i + j] = vertices[i][j];
	}

	// Allocate space for the buffer.
	const size_t size = sizeof(float) * totalElements;		// Size of arrays in bytes.
	glBufferData( GL_ARRAY_BUFFER, size, vertexPositions, GL_DYNAMIC_DRAW );

	// Set up our vertex attributes (no normals needed).
	int position_location = glGetAttribLocation( renderingProgram, "position" );
	if( position_location >= 0 )
	{
		glEnableVertexAttribArray( position_location );
		glVertexAttribPointer( position_location, ELEMENTS_PER_VERTEX, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET( 0 ) );
		
		sendShadingInformation( Projection, Camera, Model, false );			// Without using phong model.
	}

	return position_location;
}

/**
 * Render a 3D object model of a selected type.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera matrix.
 * @param Model The 4x4 model transformation matrix.
 * @param objectType Type of object to be rendered.
 * @param useTexture Whether or not use texture loaded for object.
 * @param textureUnit Which texture unit activate for sampling in shader.
 */
void OpenGL::render3DObject( const mat44& Projection, const mat44& Camera, const mat44& Model, const char* objectType, bool useTexture, int textureUnit )
{
	try
	{
		Object3D o = objectModels.at( string( objectType ) );	// Retrieve object.

		if( material.ambient[3] < 1.0 )		// If alpha channel in current material color is not fully opaque, enable blending.
		{
			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		}

		glBindBuffer( GL_ARRAY_BUFFER, o.getBufferID() );

		// Set up our vertex (and texture) attributes.
		GLint position_location = glGetAttribLocation( renderingProgram, "position" );
		GLint normal_location = glGetAttribLocation( renderingProgram, "normal" );
		GLint texCoords_location = glGetAttribLocation( renderingProgram, "texCoords" );
		if( position_location != -1 )		// Need to have at least the vertices positions to render.
		{
			glEnableVertexAttribArray( position_location );
			glVertexAttribPointer( position_location, ELEMENTS_PER_VERTEX, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET( 0 ) );
			
			size_t offset = sizeof(float) * o.getVerticesCount() * ELEMENTS_PER_VERTEX;
			
			if( normal_location != -1 )		// Do we need normals?
			{
				glEnableVertexAttribArray( normal_location );
				glVertexAttribPointer( normal_location, ELEMENTS_PER_VERTEX, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET( offset ) );
			}
			
			if( texCoords_location != -1 && useTexture && o.hasTexture() )			// Do we want to render with texture instead of color?
			{
				glEnableVertexAttribArray( texCoords_location );
				glVertexAttribPointer( texCoords_location, TEX_ELEMENTS_PER_VERTEX, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET( offset * 2 ) );
				
				// Enable texture rendering.
				glActiveTexture( GL_TEXTURE0 + textureUnit );												// Recall for objects we assigned texture unit after all lights.
				glBindTexture( GL_TEXTURE_2D, o.getTextureID() );
				glUniform1i( glGetUniformLocation( renderingProgram, "objectTexture" ), textureUnit );		// And tell OpenGL so.
			}
			else
				useTexture = false;
			
			sendShadingInformation( Projection, Camera, Model, true, useTexture );	// Indicate we are using texture if the above condition holds.
			
			// Draw triangles.
			glDrawArrays( GL_TRIANGLES, 0, o.getVerticesCount() );
			
			// Disable attribute arrays for position and normals.
			glDisableVertexAttribArray( position_location );
			if( normal_location != -1 )
				glDisableVertexAttribArray( normal_location );
			if( texCoords_location != -1 )
				glDisableVertexAttribArray( texCoords_location );
		}

		if( material.ambient[3] < 1.0 )
			glDisable( GL_BLEND );

	}
	catch( const out_of_range& oor )
	{
		cerr << "Attempting to render a nonexistent type of 3D object model!" << endl;
	}
}

/**
 * Render text using the currently loaded font and currently set font size.
 * Rendering starts at coordinates (x, y), z is always 0.
 * The pixel coordinates that the FreeType2 library uses are scaled by (sx, sy).
 * @param text Text to render.
 * @param a Glyphs atlas object.
 * @param x Start x-coordinate.
 * @param y Start y-coordinate.
 * @param sx Horizontal scaling.
 * @param sy Vertical scaling.
 * @param color Text color as 4-tuple [r,g,b,a].
 */
void OpenGL::renderText( const char* text, const Atlas* a, float x, float y, float sx, float sy, const float* color )
{
	const uint8_t *p;

	// Use the texture containing the atlas.
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, a->tex );
	glUniform1i( a->uniform_tex_loc, 0 );			// We are using here the unit 0 for the text sampler.

	// Set up the VBO for our vertex data.
	glEnableVertexAttribArray( a->attribute_coord_loc );
	glBindBuffer( GL_ARRAY_BUFFER, glyphsBufferID );
	glVertexAttribPointer( a->attribute_coord_loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

	// Set text color.
	glUniform4fv( a->uniform_color_loc, 1, color );

	GlyphPoint coords[6 * strlen(text)];
	int c = 0;

	// Loop through all characters.
	for( p = (const uint8_t *)text; *p; p++ )
	{
		// Calculate the vertex and texture coordinates.
		float x2 = x + a->c[*p].bl * sx;
		float y2 = -y - a->c[*p].bt * sy;
		float w = a->c[*p].bw * sx;
		float h = a->c[*p].bh * sy;

		// Advance the cursor to the start of the next character.
		x += a->c[*p].ax * sx;
		y += a->c[*p].ay * sy;

		// Skip glyphs that have no pixels.
		if( w <= 0 || h <= 0 )
			continue;

		coords[c++] = (GlyphPoint) { x2, -y2, a->c[*p].tx, a->c[*p].ty };
		coords[c++] = (GlyphPoint) { x2 + w, -y2, a->c[*p].tx + a->c[*p].bw / a->w, a->c[*p].ty };
		coords[c++] = (GlyphPoint) { x2, -y2 - h, a->c[*p].tx, a->c[*p].ty + a->c[*p].bh / a->h };
		coords[c++] = (GlyphPoint) { x2 + w, -y2, a->c[*p].tx + a->c[*p].bw / a->w, a->c[*p].ty };
		coords[c++] = (GlyphPoint) { x2, -y2 - h, a->c[*p].tx, a->c[*p].ty + a->c[*p].bh / a->h };
		coords[c++] = (GlyphPoint) { x2 + w, -y2 - h, a->c[*p].tx + a->c[*p].bw / a->w, a->c[*p].ty + a->c[*p].bh / a->h };
	}

	// Draw all the characters on the screen in one go.
	glBufferData( GL_ARRAY_BUFFER, sizeof( coords ), coords, GL_DYNAMIC_DRAW );
	glDrawArrays( GL_TRIANGLES, 0, c );

	glDisableVertexAttribArray( a->attribute_coord_loc );
}

/**
 * Get the glyphs program ID.
 * @return OpenGL program ID.
 */
GLuint OpenGL::getGlyphsProgram()
{
	return glyphsProgram;
}

/**
 * Set the uniform scaling flag to avoid computing inverses of the 3x3 principal submatrix of the model view matrix.
 * @param u True for using uniform scaling.
 */
void OpenGL::setUsingUniformScaling( bool u )
{
	usingUniformScaling = u;
}

/**
 * Load a new type of 3D object and allocate its necessary OpenGL rendering objects.
 * @param name User-defined object type name.
 * @param filename *.obj filename that contains the 3D triangular mesh.
 * @param textureFilename Object's texture if needed.
 */
void OpenGL::create3DObject( const char* name, const char* filename, const char* textureFilename )
{
	// Check if the object we want to create already exists.  If so, empty its buffer and recreate it.
	string sName = string( name );
	auto it = objectModels.find( sName );
	if( it != objectModels.end() )					// Element found?
	{
		Object3D o = it->second;
		cout << "WARNING!  You are attempting to create a new type of 3D object with an existing name.  The old one will be replaced!" << endl;
		GLuint bufferID = o.getBufferID();
		GLuint textureID = o.getTextureID();
		glDeleteBuffers( 1, &bufferID );			// Empty buffer and texture.
		if( o.hasTexture() && glIsTexture( textureID ) )
			glDeleteTextures( 1, &textureID );
	}

	objectModels[sName] = Object3D( name, filename, textureFilename );
	cout << "The 3D object of kind \"" << name << "\" has been successfully allocated!" << endl;
}

/**
 * Set the rendering program and start using it.
 * @param program OpenGL program ID.
 */
void OpenGL::useProgram( GLuint program )
{
	renderingProgram = program;
	glUseProgram( renderingProgram );
}

/**
 * Set and send the lighting properties to shaders attached to current rendering program.
 * @param light Light object.
 * @param View The 4x4 view transformation matrix (usually the camera matrix).
 * @param useUnitSuffix Wheter attach light index as suffix to shader uniform variables.
 */
void OpenGL::setLighting( const Light& light, const mat44& View, bool useUnitSuffix )
{
	string lightSpaceMatrixStr = "LightSpaceMatrix";
	string lightPositionStr = "lightPosition";
	string lightColorStr = "lightColor";
	
	if( useUnitSuffix )		// Does the shader have light names with suffix corresponding to unit?
	{
		lightSpaceMatrixStr += to_string( light.getUnit() );
		lightPositionStr += to_string( light.getUnit() );
		lightColorStr += to_string( light.getUnit() );
	}
	
	// Send light space matrix transform if shaders have corresponding receptor.
	int lsm_location = glGetUniformLocation( renderingProgram, lightSpaceMatrixStr.c_str() );
	if( lsm_location >= 0 )
	{
		float lsm_matrix[ELEMENTS_PER_MATRIX];
		Tx::toOpenGLMatrix( lsm_matrix, light.SpaceMatrix );
		glUniformMatrix4fv( lsm_location, 1, GL_FALSE, lsm_matrix );
	}
	
	// Light position.
	int lightSource_location = glGetUniformLocation( renderingProgram, lightPositionStr.c_str() );
	if( lightSource_location >= 0 )
	{
		float ls_vector[HOMOGENEOUS_VECTOR_SIZE];
		Tx::toOpenGLMatrix( ls_vector, View * vec4{ light.position[0], light.position[1], light.position[2], 1.0 } );		// We must send the light position in view coordinates.
		glUniform4fv( lightSource_location, 1, ls_vector );
	}
	
	// Light color.
	int lightColor_location = glGetUniformLocation( renderingProgram, lightColorStr.c_str() );
	if( lightColor_location >= 0 )
	{
		float lightColor_vector[VECTOR_SIZE_3D];
		Tx::toOpenGLMatrix( lightColor_vector, light.color );
		glUniform3fv( lightColor_location, 1, lightColor_vector );
	}
}












