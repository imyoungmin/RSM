/**
 * OpenGL main application.
 */

#include <iostream>
#include <armadillo>
#include <OpenGL/gl3.h>
#include <string>
#include "GLFW/glfw3.h"
#include "ArcBall/Ball.h"
#include "OpenGL.h"
#include "Transformations.h"

using namespace std;
using namespace std::chrono;
using namespace arma;

// Perspective projection matrix.
mat44 Proj;

// Text scaling.
float gTextScaleX;
float gTextScaleY;

// Camera controls globals.
vec3 gPointOfInterest;
vec3 gEye;
vec3 gUp;

bool gLocked;							// Track if mouse button is pressed down.
bool gUsingArrowKey;					// Track if we are using the arrow keys for rotating scene.
bool gRotatingLights;					// Enable/disable rotating lights about the scene.
bool gRotatingCamera;					// Enable/disable rotating camera.
float gZoom;							// Camera zoom.
const float ZOOM_IN = 1.015;
const float ZOOM_OUT = 0.985;
BallData* gArcBall;						// Arc ball.

// Framebuffer size metrics.
int fbWidth;
int fbHeight;
float gRetinaRatio;						// How many screen dots exist per OpenGL pixel.

OpenGL ogl;								// Initialize application OpenGL.

// Lights.
vector<Light> gLights;					// Light source objects.
int gLightsCount = 0;

// Frame rate variables and functions.
static const int NUM_FPS_SAMPLES = 64;
float gFpsSamples[NUM_FPS_SAMPLES];
unsigned char gCurrentSample = 0;		// Should start storing from gCurrentSample >= 1.

/**
 * Calculate the number of frames per second using a window.
 * @param dt Amount of seconds for current frame.
 * @return Frames per second.
 */
float calculateFPS( float dt )
{
	gCurrentSample++;
	gCurrentSample = static_cast<unsigned char>( max( 1, static_cast<int>( gCurrentSample ) ) );
	if( dt <= 0 )
		cout << "error" << endl;
	gFpsSamples[(gCurrentSample - 1) % NUM_FPS_SAMPLES] = 1.0f / dt;
	float fps = 0;
	int i = 0;
	for( i = 0; i < min( NUM_FPS_SAMPLES, static_cast<int>( gCurrentSample ) ); i++ )
		fps += gFpsSamples[i];
	fps /= i;
	return fps;
}

/**
 * Reset rotation and zoom.
 */
void resetArcBall()
{
	Ball_Init( gArcBall );
	Ball_Place( gArcBall, qOne, 0.75 );
}

/**
 * GLFW error callback.
 * @param error Error code.
 * @param description Error description.
 */
void errorCallback( int error, const char* description )
{
	cerr << error << ": " << description << endl;
}

/**
 * Rotate scene in x or y direction.
 * @param x Rotation amount in x direction, usually in the range [-1,1].
 * @param y Rotation amount in y direction, usually in the range [-1,1].
 */
void rotateWithArrowKey( float x, float y )
{
	if( gLocked )		// Do not rotate scene with arrow key if it's currently rotating with mouse.
		return;

	gUsingArrowKey = true;						// Start blocking rotation with mouse button.
	HVect arcballCoordsStart, arcballCoordsEnd;

	arcballCoordsStart.x = 0.0;					// Start rotation step.
	arcballCoordsStart.y = 0.0;
	Ball_Mouse( gArcBall, arcballCoordsStart );
	Ball_BeginDrag( gArcBall );

	arcballCoordsEnd.x = x;						// End rotation step.
	arcballCoordsEnd.y = y;
	Ball_Mouse( gArcBall, arcballCoordsEnd );
	Ball_Update( gArcBall );
	Ball_EndDrag( gArcBall );
	gUsingArrowKey = false;						// Exiting conflicting block for rotating with arrow keys.
}

/**
 * GLFW keypress callback.
 * @param window GLFW window.
 * @param key Which key was pressed.
 * @param scancode Unique code for key.
 * @param action Key action: pressed, etc.
 * @param mods Modifier bits: shift, ctrl, alt, super.
 */
void keyCallback( GLFWwindow* window, int key, int scancode, int action, int mods )
{
	if( action != GLFW_PRESS && action != GLFW_REPEAT )
		return;
	
	const float rotationStep = 0.0025;
	
	switch( key )
	{
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose( window, GL_TRUE );
			break;
		case GLFW_KEY_LEFT:
			rotateWithArrowKey( -rotationStep, 0 );
			break;
		case GLFW_KEY_RIGHT:
			rotateWithArrowKey( +rotationStep, 0 );
			break;
		case GLFW_KEY_UP:
			rotateWithArrowKey( 0, +rotationStep );
			break;
		case GLFW_KEY_DOWN:
			rotateWithArrowKey( 0, -rotationStep );
			break;
		case GLFW_KEY_R:
			resetArcBall();
			gZoom = 1.0;
			break;
		case GLFW_KEY_L:
			if( !gRotatingCamera )				// Avoid rotating camera and lights at the same time.
				gRotatingLights = !gRotatingLights;
			break;
		case GLFW_KEY_C:
			if( !gRotatingLights )
				gRotatingCamera = !gRotatingCamera;
			break;
		default: return;
	}
}

/**
 * GLFW mouse button callback.
 * @param window GLFW window.
 * @param button Which mouse button has been actioned.
 * @param action Mouse action: pressed or released.
 * @param mode Modifier bits: shift, ctrl, alt, supper.
 */
void mouseButtonCallback( GLFWwindow* window, int button, int action, int mode )
{
	if( button != GLFW_MOUSE_BUTTON_LEFT )		// Ignore mouse button other than left one.
		return;

	if( gUsingArrowKey )						// Wait for arrow keys to stop being used as rotation mechanism.
		return;

	if( action == GLFW_PRESS )
	{
		//glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
		HVect arcballCoords;
		double x, y;
		int w, h;
		glfwGetWindowSize( window, &w, &h );
		glfwGetCursorPos( window, &x, &y );
		arcballCoords.x = static_cast<float>( 2.0*x/static_cast<float>(w) - 1.0 );
		arcballCoords.y = static_cast<float>( -2.0*y/static_cast<float>(h) + 1.0 );
		Ball_Mouse( gArcBall, arcballCoords );
		Ball_BeginDrag( gArcBall );
		gLocked = true;							// Determines whether the mouse movement is used for rotating the object.
	}
	else
	{
		//glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_NORMAL );
		Ball_EndDrag( gArcBall );
		gLocked = false;						// Go back to normal.
	}
}

/**
 * GLFW mouse motion callback.
 * @param window GLFW window.
 * @param x Mouse x-position.
 * @param y Mouse y-position.
 */
void mousePositionCallback( GLFWwindow* window, double x, double y )
{
	if( glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_LEFT ) == GLFW_PRESS && gLocked )
	{
		HVect arcballCoords;
		int w, h;
		glfwGetWindowSize( window, &w, &h );
		arcballCoords.x = static_cast<float>( 2.0*x/static_cast<float>(w) - 1.0 );
		arcballCoords.y = static_cast<float>( -2.0*y/static_cast<float>(h) + 1.0 );
		Ball_Mouse( gArcBall, arcballCoords );
		Ball_Update( gArcBall );
	}
}

/**
 * GLFW mouse scroll callback.
 * @param window GLFW window.
 * @param xOffset X offset.
 * @param yOffset Y offset.
 */
void mouseScrollCallback( GLFWwindow* window, double xOffset, double yOffset )
{
	gZoom *= (yOffset > 0)? ZOOM_IN: ZOOM_OUT;
	gZoom = max( 0.5f, min( gZoom, 2.5f ) );
}

/**
 * GLFW frame buffer resize callback.
 * @param window GLFW window.
 * @param w New frame buffer width.
 * @param h New frame buffer height.
 */
void resizeCallback( GLFWwindow* window, int w, int h )
{
	fbWidth = w;		// w and h are width and height of framebuffer, not window.
	fbHeight = h;

	//Proj = Tx::frustrum( -0.5, 0.5, -0.5, 0.5, 1.0, 100 );

	// Projection used for 3D.
	double ratio = static_cast<double>(w)/static_cast<double>(h);
	Proj = Tx::perspective( M_PI/3.0, ratio, 0.01, 1000.0 );

	// Projection used for text rendering.
	int windowW, windowH;
	glfwGetWindowSize( window, &windowW, &windowH );
	gTextScaleX = 1.0f / windowW;
	gTextScaleY = 1.0f / windowH;
}

/**
 * Render the swinging lamp.
 * @param Projection The 4x4 projection matrix.
 * @param View The 4x4 view matrix.
 * @param T The transformation matrix for the whole lamp object.
 * @param currentTime Curent step.
 */
void renderSwingingLamp( const mat44& Projection, const mat44& View, const mat44& T, double currentTime )
{
	ogl.setColor( 0.7, 0.7, 0.0, 0.5 );
	vec3 start = {-sqrt(18)+0.78, 0, 0}, end = {sqrt(18)-0.78, 0, 0}, middle = ( start + end ) / 2.0;
	middle[1] -= 0.75;
	vector<vec3> vertices( { start, middle, end } );
	ogl.drawPath( Projection, View, T, vertices );
	
	ogl.setColor( 0.7, 0.7, 0.0, 1.0, -1.0f );
	ogl.drawSphere( Projection, View, T * Tx::translate( middle - Tx::Y_AXIS * 0.08 ) * Tx::scale( 0.08 ) );
	
	ogl.setColor( 0.4, 0.18, 0.15, 0.8 );
	ogl.render3DObject( Projection, View, T * Tx::translate( middle - Tx::Y_AXIS * 0.08 ) * Tx::scale( 0.5 ), "lamp" );
	
	vector<vec3> vertices2( { middle, middle - Tx::Y_AXIS } );
	ogl.drawPath( Projection, View, T, vertices2 );
	
	ogl.setColor( 0.7, 0.7, 0.0, 1.0, -1.0f );
	ogl.drawSphere( Projection, View, T * Tx::translate( middle - Tx::Y_AXIS * 0.725 ) * Tx::scale( 0.08 ) );
	ogl.drawSphere( Projection, View, T * Tx::translate( middle - Tx::Y_AXIS ) * Tx::scale( 0.05 ) );
}

/**
 * Render the scene.
 * @param Projection The 4x4 projection matrix to use.
 * @param View The 4x4 view matrix.
 * @param Model Any previously built 4x4 model matrix (usually containing current zoom and scene rotation as provided by arcball).
 * @param currentTime Current step.
 */
void renderScene( const mat44& Projection, const mat44& View, const mat44& Model, double currentTime )
{
	ogl.setColor( 0.9, 0.9, 0.9, 1.0, 32.0 );			// Columns.
	float r = 6.0f;
	for( int i = 0; i < 4; i++ )
	{
		double angle = M_PI/4.0 + i * M_PI/2.0;
		ogl.render3DObject( Projection, View, Model * Tx::translate( r * sin( angle ), 0, r * cos( angle ) ), "column", true, gLightsCount );	// Use texture.
	}
	
	ogl.setColor( 0.85, 0.85, 0.85 );					// Dragon.
	ogl.render3DObject( Projection, View, Model * Tx::translate( 0.0, 0.2, 0.0 ) * Tx::rotate( M_PI/2.0, Tx::Y_AXIS ), "dragon" );
	
	ogl.setColor( 0.8, 0.8, 0.8, 1.0, 16.0 );			// Ground with tiles.
	for( int i = -9; i <= 9; i++ )
	{
		for( int j = -9; j <= 9; j++ )
		{
			if( i >= -1 && i <= 1 && j >= -1 && j <= 1 )
				continue;
			ogl.render3DObject( Projection, View, Model * Tx::translate( i, 0, j ) * Tx::scale( 0.5 ), "tile", true, gLightsCount );			// Use texture.
		}
	}
	
	// Dragon circular base.
	ogl.setColor( 0.35, 0.18, 0.15, 1.0, 32.0 );
	ogl.drawCylinder( Projection, View, Model * Tx::rotate( -M_PI_2, Tx::X_AXIS ) * Tx::scale( 2.5, 2.5, 0.2 ) );
	ogl.setColor( 0.23, 0.22, 0.25, 1.0, 32.0 );
	ogl.drawCylinder( Projection, View, Model * Tx::rotate( -M_PI_2, Tx::X_AXIS ) * Tx::scale( 3.0, 3.0, 0.1 ) );
	
	// Render swinging lamps.
	mat44 T = Tx::translate( 0.0, 4.48, sqrt(18) ) * Tx::rotate( M_PI_4 * sin( currentTime * 4.0 ), Tx::X_AXIS );
	for( int i = 0; i < 4; i++ )
		renderSwingingLamp( Projection, View, Model * Tx::rotate( M_PI_2 * i, Tx::Y_AXIS ) * T, currentTime );
}

/**
 * Application main function.
 * @param argc Number of input arguments.
 * @param argv Input arguments.
 * @return Exit code.
 */
int main( int argc, const char * argv[] )
{
	srand( static_cast<unsigned>( time( 0 ) ) );
	
	gPointOfInterest = { 0, 0, 0 };		// Camera controls globals.
	gEye = { 3, 7, 17 };
	gUp = Tx::Y_AXIS;
	
	gLocked = false;					// Track if mouse button is pressed down.
	gRotatingLights = false;			// Start with still lights.
	gRotatingCamera = false;
	gUsingArrowKey = false;				// Track pressing action of arrow keys.
	gZoom = 1.0;						// Camera zoom.
	
	GLFWwindow* window;
	glfwSetErrorCallback( errorCallback );
	
	if( !glfwInit() )
		exit( EXIT_FAILURE );
	
	// Indicate GLFW which version will be used and the OpenGL core only.
	glfwWindowHint( GLFW_SAMPLES, 4 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
	glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
	
	cout << glfwGetVersionString() << endl;

	// Create window object (with screen-dependent size metrics).
	int windowWidth = 1024;
	int windowHeight = 1024;
	window = glfwCreateWindow( windowWidth, windowHeight, "Real-Time Rendering", nullptr, nullptr );

	if( !window )
	{
		glfwTerminate();
		exit( EXIT_FAILURE );
	}
	
	glfwMakeContextCurrent( window );
	glfwSwapInterval( 1 );
	
	// Hook up callbacks.
	glfwSetFramebufferSizeCallback( window, resizeCallback );
	glfwSetKeyCallback( window, keyCallback );
	glfwSetMouseButtonCallback( window, mouseButtonCallback );
	glfwSetCursorPosCallback( window, mousePositionCallback );
	glfwSetScrollCallback( window, mouseScrollCallback );

	// Initialize projection matrices and viewport.
	glfwGetFramebufferSize( window, &fbWidth, &fbHeight );
	gRetinaRatio = static_cast<float>( fbWidth ) / windowWidth;
	cout << "Retina pixel ratio: " << gRetinaRatio << endl;
	resizeCallback( window, fbWidth, fbHeight );
	
	gArcBall = new BallData;						// Initialize arcball.
	resetArcBall();
	
	///////////////////////////////////// Intialize OpenGL and rendering shaders ///////////////////////////////////////
	
	ogl.init();
	
	// Initialize shaders for geom/sequence drawing program.
	cout << "Initializing rendering shaders... ";
	Shaders shaders;
	GLuint renderingProgram = shaders.compile( conf::SHADERS_FOLDER + "shader.vert", conf::SHADERS_FOLDER + "shader.frag" );		// Usual rendering.
	cout << "Done!" << endl;
	
	// Initialize shaders program for shadow mapping.
	cout << "Initializing shadow mapping shaders... ";
	GLuint shadowMapProgram = shaders.compile( conf::SHADERS_FOLDER + "shadow.vert", conf::SHADERS_FOLDER + "shadow.frag" );
	cout << "Done!" << endl;
	
	//////////////////////////////////////////////// Create lights /////////////////////////////////////////////////////
	
	float lNearPlane = 0.01f, lFarPlane = 200.0f;									// Setting up the light projection matrix.
	float lSide = 30.0f;
	mat44 LightProjection = Tx::ortographic( -lSide, lSide, -lSide, lSide, lNearPlane, lFarPlane );
	
	gLightsCount = 3;
	const double lRadius = sqrt( 11 * 11 * 2 );
	const double theta = 2.0 * M_PI / gLightsCount;
	const float phi = static_cast<float>( rand() ) / static_cast<float>( RAND_MAX / M_PI_4 );
	const float lHeight = 15;
	const float lRGB[3] = { 0.6, 0.5, 0.5 };
	for( int i = 0; i < gLightsCount; i++ )
		gLights.push_back( Light({ lRadius * sin( i * theta + phi ), lHeight, lRadius * cos( i * theta + phi ) },
								 { lRGB[i % 3], lRGB[(i+1) % 3], lRGB[(i+2) % 3] },
								 LightProjection,
								 i) );
	
	/////////////////////////////////////////// Setting up shadow mapping //////////////////////////////////////////////
	
	const auto SHADOW_SIDE_LENGTH = static_cast<GLuint>( max(fbWidth, fbHeight)*2 );	// Texture size.
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };								// Depth = 1.0.  So the rendering of the normal scene will produce something larger than this.
	char shadowMapLocationStr[12];
	
	for( int i = 0; i < gLightsCount; i++ )											// Create framebuffers for rendering the shadow maps with respect to each light.
	{
		glGenFramebuffers( 1, &(gLights[i].shadowMapFBO) );							// All information is kept in the Light object.
		
		glGenTextures( 1, &(gLights[i].shadowMapTextureID) );						// Generate texture and properties.
		glBindTexture( GL_TEXTURE_2D, gLights[i].shadowMapTextureID );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_SIDE_LENGTH, SHADOW_SIDE_LENGTH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );	// By doing this, anything farther than the shadow map will appear in light.
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
		glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
		
		glBindFramebuffer( GL_FRAMEBUFFER, gLights[i].shadowMapFBO );				// Attach texture as the framebuffer in the depth buffer.
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gLights[i].shadowMapTextureID, 0 );
		glDrawBuffer( GL_NONE );													// We won't render any color.
		glReadBuffer( GL_NONE );
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );										// Unbind.
		
		sprintf( shadowMapLocationStr, "shadowMap%d", gLights[i].getUnit() );
		gLights[i].shadowMapLocation = glGetUniformLocation( renderingProgram, shadowMapLocationStr );
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	double currentTime = 0.0;
	const double timeStep = 0.01;
	const float textColor[] = { 0.0, 0.8, 1.0, 1.0 };
	char text[128];
	
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
	glFrontFace( GL_CCW );

	// Frame rate variables.
	long gNewTicks = duration_cast<milliseconds>( system_clock::now().time_since_epoch() ).count();
	long gOldTicks = gNewTicks;
	float transcurredTimePerFrame;
	string FPS = "FPS: ";

	ogl.setUsingUniformScaling( true );							// Important! We'll be using uniform scaling in the following scene rendering.
	ogl.create3DObject( "column", "column.obj", "Minoan_column_b.png" );	// Create 3D object models.
	ogl.create3DObject( "dragon", "dragon.obj" );
	ogl.create3DObject( "tile", "tile.obj", "Iron_Plate_DIF.png" );
	ogl.create3DObject( "lamp", "lamp.obj", "cl_wires.jpg" );
	
	float eyeY = gEye[1];										// Build eye components from its intial value.
	float eyeXZRadius = sqrt( gEye[0]*gEye[0] + gEye[2]*gEye[2] );
	float eyeAngle = atan2( gEye[0], gEye[2] );
	
	// Rendering loop.
	while( !glfwWindowShouldClose( window ) )
	{
		glClearColor( 0.0f, 0.0f, 0.01f, 1.0f );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glEnable( GL_CULL_FACE );
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		HMatrix abr;
		Ball_Value( gArcBall, abr );
		mat44 ArcBall = {
			{ abr[0][0], abr[0][1], abr[0][2], abr[0][3] },
			{ abr[1][0], abr[1][1], abr[1][2], abr[1][3] },
			{ abr[2][0], abr[2][1], abr[2][2], abr[2][3] },
			{ abr[3][0], abr[3][1], abr[3][2], abr[3][3] } };
		mat44 Model = ArcBall.t() * Tx::scale( gZoom );
		
		///////////////////////////////////////// Define new lights' positions /////////////////////////////////////////
		
		if( gRotatingLights )								// Check if rotating lights is enabled (with key 'L').
		{
			for( int i = 0; i < gLightsCount; i++ )
				gLights[i].rotateBy( static_cast<float>( 0.01 * M_PI ) );
		}
		
		//////////////////////////////////// First pass: render scene to depth maps ////////////////////////////////////
		
		ogl.useProgram( shadowMapProgram );					// Set shadow map writing program.
		for( int i = 0; i < gLightsCount; i++ )
		{
			mat44 LightView = Tx::lookAt( gLights[i].position, gPointOfInterest, Tx::Y_AXIS );
			gLights[i].SpaceMatrix = gLights[i].Projection * LightView;
			
			glViewport( 0, 0, SHADOW_SIDE_LENGTH, SHADOW_SIDE_LENGTH );
			glBindFramebuffer( GL_FRAMEBUFFER, gLights[i].shadowMapFBO );
			glClear( GL_DEPTH_BUFFER_BIT );
			
			ogl.setLighting( gLights[i], LightView );
			renderScene( LightProjection, LightView, Model, currentTime );
			glBindFramebuffer( GL_FRAMEBUFFER, 0 );			// Unbind: return control to normal draw framebuffer.
		}

		//////////////////////////////// Second pass: render scene with shadow mapping /////////////////////////////////

		ogl.useProgram( renderingProgram );					// Set usual rendering program.
		if( gRotatingCamera )
		{
			eyeAngle += 0.01 * M_PI;
			gEye = { eyeXZRadius * sin( eyeAngle ), eyeY, eyeXZRadius * cos( eyeAngle ) };
		}
		
		mat44 Camera = Tx::lookAt( gEye, gPointOfInterest, gUp );
		
		glViewport( 0, 0, fbWidth, fbHeight );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		
		// Enable shadow mapping texture samplers.
		for( int i = 0; i < gLightsCount; i++ )
		{
			glActiveTexture( GL_TEXTURE0 + gLights[i].getUnit() );
			glBindTexture( GL_TEXTURE_2D, gLights[i].shadowMapTextureID );
			glUniform1i( gLights[i].shadowMapLocation, gLights[i].getUnit() );	// The light shadow map is associated to unit GL_TEXTURE0 + light unit.
			
			// Set and send the lighting properties.
			ogl.setLighting( gLights[i], Camera, true );
		}
		renderScene( Proj, Camera, Model, currentTime );

		/////////////////////////////////////////////// Rendering text /////////////////////////////////////////////////

		glUseProgram( ogl.getGlyphsProgram() );				// Switch to text rendering.  The text rendering is the only program created within the OpenGL class.

		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glDisable( GL_CULL_FACE );

		gNewTicks = duration_cast<milliseconds>( system_clock::now().time_since_epoch() ).count();
		transcurredTimePerFrame = (gNewTicks - gOldTicks) / 1000.0f;
		sprintf( text, "FPS: %.2f", ( ( transcurredTimePerFrame <= 0 )? -1 : calculateFPS( transcurredTimePerFrame ) ) );
		gOldTicks = gNewTicks;

		ogl.renderText( text, ogl.atlas48, -1 + 10 * gTextScaleX, 1 - 30 * gTextScaleY, static_cast<float>( gTextScaleX * 0.6 ),
						static_cast<float>( gTextScaleY * 0.6 ), textColor );

		glDisable( GL_BLEND );

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		glfwSwapBuffers( window );
		glfwPollEvents();
		
		currentTime += timeStep;
	}
	
	glfwDestroyWindow( window );
	glfwTerminate();
	
	// Delete OpenGL programs.
	glDeleteProgram( renderingProgram );
	
	return 0;
}

