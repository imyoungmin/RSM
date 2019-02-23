#ifndef OPENGL_ATLAS_H
#define OPENGL_ATLAS_H

#include <iostream>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <OpenGL/gl3.h>

#define MAXWIDTH 1024 		// Maximum texture width for text atlas.
#define ATLAS_SIZE 128		// Number of characters held in atals texture.

using namespace std;

/**
 * The Atlas class holds a texture that contains the visible US-ASCII characters
 * of a certain font rendered with a certain character height.
 * It also contains an array that contains all the information necessary to
 * generate the appropriate vertex and texture coordinates for each character.
 *
 * After the constructor is run, you don't need to use any FreeType functions anymore.
 */
class Atlas
{
public:
	GLuint tex;					// Texture object ID.
	GLint uniform_tex_loc;		// Sampler ID in fragment shader.
	GLuint attribute_coord_loc;	// Vertex shader location for position.
	GLint uniform_color_loc;	// Characters color shader location.

	GLuint w;				// Width of texture in pixels.
	GLuint h;				// Height of texture in pixels.

	struct {
		float ax;			// advance.x
		float ay;			// advance.y

		float bw;			// bitmap.width;
		float bh;			// bitmap.height;

		float bl;			// bitmap_left;
		float bt;			// bitmap_top;

		float tx;			// x offset of glyph in texture coordinates
		float ty;			// y offset of glyph in texture coordinates
	} c[ATLAS_SIZE];		// Character information

	Atlas( FT_Face face, GLuint height, GLint uniform_tex, GLint attribute_coord, GLint uniform_color );
	~Atlas();
};


#endif //OPENGL_ATLAS_H
