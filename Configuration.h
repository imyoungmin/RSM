#ifndef OPENGL_CONFIGURATION_H
#define OPENGL_CONFIGURATION_H

#include <string>

using namespace std;

/**
 * Defining configuration parameters used accross the application.
 */
namespace conf
{
	const string RESOURCES_FOLDER	= "/Users/youngmin/Documents/CS/Real-Time High Quality Rendering/Projects/RSM/Resources/";
	const string SHADERS_FOLDER 	= RESOURCES_FOLDER + "shaders/";
	const string FONTS_FOLDER 		= RESOURCES_FOLDER + "fonts/";
	const string OBJECTS_FOLDER 	= RESOURCES_FOLDER + "objects/";
}

#endif //OPENGL_CONFIGURATION_H
