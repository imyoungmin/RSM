#include "Shaders.h"

/**
 * Read shader file, line by line.
 * @param fname Shader file name, with relative path.
 */
string Shaders::read( const string& fname )
{
	string content;
	ifstream sFile( fname );
	
	if( sFile.is_open() )
	{
		string line;
		while( getline( sFile, line ) )
			content += line + '\n';
		
		sFile.close();
	}
	else
	{
		cerr << "Unable to open file " << fname << endl;
		exit( EXIT_FAILURE );
	}
	
	return content;
}

/**
 * Creates a program from the vertex and fragment shaders provided.
 * @param fvert Vertex shader file name, with relative path.
 * @param ffrag Fragment shader file name, with relative parth.
 * @return A shading program, otherwise, it exits the application with an error.
 */
GLuint Shaders::compile( const string& fvert, const string& ffrag )
{
	const GLint MAXLENGTH = 500;
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint program;
	GLint compileParam;
	GLchar compileInfoLog[MAXLENGTH+1];
	GLint compileInfoLength;
	
	// Source code for vertex shader.
	string s = read( fvert );
	const GLchar* vertexShaderSource = s.c_str();
	
	// Source code for fragment shader.
	string t = read( ffrag );
	const GLchar* fragmentShaderSource = t.c_str();
	
	// Create and compile verter shader.
	vertexShader = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( vertexShader, 1, &vertexShaderSource, NULL );
	glCompileShader( vertexShader );
	glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &compileParam );
	if( compileParam == GL_FALSE )
	{
		glGetShaderInfoLog( vertexShader, MAXLENGTH, &compileInfoLength, compileInfoLog );
		cerr << compileInfoLog << endl;
		exit( EXIT_FAILURE );
	}
	
	// Create and compile fragment shader.
	fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( fragmentShader, 1, &fragmentShaderSource, NULL );
	glCompileShader( fragmentShader );
	glGetShaderiv( fragmentShader, GL_COMPILE_STATUS, &compileParam );
	if( compileParam == GL_FALSE )
	{
		glGetShaderInfoLog( fragmentShader, MAXLENGTH, &compileInfoLength, compileInfoLog );
		cerr << compileInfoLog << endl;
		exit( EXIT_FAILURE );
	}
	
	// Create program, attach shaders to it, and link it.
	program = glCreateProgram();
	glAttachShader( program, vertexShader );
	glAttachShader( program, fragmentShader );
	glLinkProgram( program );
	
	GLint linkParam;
	GLint linkInfoLogLength;
	GLchar linkInfoLog[MAXLENGTH+1];
	glGetProgramiv( program, GL_LINK_STATUS, &linkParam );
	if( linkParam == GL_FALSE )
	{
		glGetProgramInfoLog( program, MAXLENGTH, &linkInfoLogLength, linkInfoLog );
		cerr << linkInfoLog << endl;
		exit( EXIT_FAILURE );
	}
	
	// Delete shaders since the program has them all now.
	glDeleteShader( vertexShader );
	glDeleteShader( fragmentShader );
	
	return program;
}
