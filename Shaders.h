#ifndef Shaders_h
#define Shaders_h

#include <iostream>
#include <fstream>
#include <string>
#include <OpenGL/gl3.h>

using namespace std;

class Shaders
{
private:
	string read( const string& fname );
	
public:
	GLuint compile( const string& fvert, const string& ffrag );
};

#endif /* shaders_h */
