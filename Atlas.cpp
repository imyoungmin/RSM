#include "Atlas.h"

/**
 * Text characters constructor.
 * @param face Freetype face object pointer.
 * @param height Character desirable height.
 * @param uniform_tex Shader texture sampler uniform location ID.
 * @param attribute_coord Shader texture coordinates location ID.
 * @param uniform_color Shader text color location ID.
 */
Atlas::Atlas( FT_Face face, GLuint height, GLint uniform_tex, GLint attribute_coord, GLint uniform_color )
{
	FT_Set_Pixel_Sizes( face, 0, height );		// Character width is calculated automatically.
	FT_GlyphSlot g = face->glyph;

	GLuint roww = 0;
	GLuint rowh = 0;
	w = 0;
	h = 0;

	// Must be initialized beforehand in caller.
	uniform_tex_loc = uniform_tex;				// Shader locations.
	attribute_coord_loc = static_cast<GLuint>( attribute_coord );
	uniform_color_loc = uniform_color;

	memset( c, 0, sizeof( c ) );

	// Find minimum size for a texture holding all ATLAS_SIZE characters.
	for( GLuint i = 32; i < ATLAS_SIZE; i++ )
	{
		if( FT_Load_Char( face, i, FT_LOAD_RENDER ) )
		{
			fprintf( stderr, "Loading character %c failed!\n", i );
			continue;
		}
		if( roww + g->bitmap.width + 1 >= MAXWIDTH )
		{
			w = max( w, roww );
			h += rowh;
			roww = 0;
			rowh = 0;
		}
		roww += g->bitmap.width + 1;
		rowh = max( rowh, g->bitmap.rows );
	}

	w = max( w, roww );
	h += rowh;

	// Create a texture that will be used to hold all glyphs.
	glGenTextures( 1, &tex );
	glBindTexture( GL_TEXTURE_2D, tex );

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr );

	// We require 1 byte alignment when uploading texture data.
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

	// Clamping to edges is important to prevent artifacts when scaling.
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	// Linear filtering usually looks best for text.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Paste all glyph bitmaps into the texture, remembering the offset.
	int ox = 0;
	int oy = 0;

	rowh = 0;

	for( GLuint i = 32; i < ATLAS_SIZE; i++ )
	{
		if( FT_Load_Char( face, i, FT_LOAD_RENDER ) )
		{
			fprintf( stderr, "Loading character %c failed!\n", i );
			continue;
		}

		if( ox + g->bitmap.width + 1 >= MAXWIDTH )
		{
			oy += rowh;
			rowh = 0;
			ox = 0;
		}

		glTexSubImage2D( GL_TEXTURE_2D, 0, ox, oy, g->bitmap.width, g->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer );
		c[i].ax = g->advance.x >> 6;
		c[i].ay = g->advance.y >> 6;

		c[i].bw = g->bitmap.width;
		c[i].bh = g->bitmap.rows;

		c[i].bl = g->bitmap_left;
		c[i].bt = g->bitmap_top;

		c[i].tx = ox / (float)w;
		c[i].ty = oy / (float)h;

		rowh = max(rowh, g->bitmap.rows);
		ox += g->bitmap.width + 1;
	}

	fprintf( stdout, "Generated a %d x %d (%.2f kb) texture atlas\n", w, h, w * h / 1024.0 );
}

/**
 * Destructor.
 */
Atlas::~Atlas()
{
	glDeleteTextures( 1, &tex );
}
