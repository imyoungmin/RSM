# Real-Time High Quality Rendering - Reflective Shadow Maps
By Luis Ángel  (임 영민) - All rights reserved (c) 2019

**www.youngmin.com.mx**

## Functionality

This OpenGL 4.1 project creates a GLFW window and renders on it a scene with geometric and textured 3D object models. 
The scene also has 3 colored area lights that make objects cast shadows using the **Percentage Closer Soft Shadows**
procedure.  We further support the **Blinn-Phong Reflectance Model**, and render text using FreeType and textures.

To interact with the application click and drag to rotate the scene, press `L` to rotate the light sources, press `C`
to rotate the camera, or zoom in/out using the mouse scroll button.

All of the fonts, shaders, 3D object models, and textures must be located in a `Resources` directory, and you should 
provide its path in the `Configuration.h` header file.

## Requirements

The code has been tested on macOS 10.13 (High Sierra), and requires the following libraries to be installed 
under `/usr/local/lib` (with their respective `include` directories in `/usr/local/include`):
- GLFW `https://www.glfw.org/download.html`
- LibPNG `https://sourceforge.net/projects/libpng/files/`
- FreeType `https://sourceforge.net/projects/freetype/files/`
- Armadillo `http://arma.sourceforge.net/download.html`

The above libraries may be built using `./configure` - `make` - `sudo make install`, except *GLFW* which requires 
CMake to be installed (`https://cmake.org/download/`).

You may create a project using the *CLion* (`https://www.jetbrains.com/cpp/`), which requires XCode to be installed 
in your macOS system.

If you create the project on *XCode*, make sure to add the `OpenGL`  framework and the libraries `GLFW`, `FreeType`, 
and `Armadillo` to your target in the project configuration settings.
