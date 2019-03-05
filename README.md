# Real-Time High-Quality Rendering - Reflective Shadow Maps
By Luis Ángel  (임 영민) - All rights reserved (c) 2019

**www.youngmin.com.mx**

## Functionality

We have implemented **Reflective Shadow Maps** (RSM), together with **Percentage-Closer Soft Shadows** (PCSS) and **Screen-
Space Ambient Occlusion** (SSAO) for added realism and details.  Our approach works mostly with blurred textures and diffuse 3D 
objects shaded with the **Blinn-Phong Reflectance Model**.  We have also resorted to **Deferred Rendering** to achieve interactive 
rates when RSM and SSAO are enabled, which are, by definition, very expensive tasks in terms of GPU resources.  Finally, with regards 
to random sampling, we are using **Poisson Disks** which provide a good even distribution of 2D points without the artifacts that 
usually appear when employing pure uniformly distributed numbers in both PCSS and the importance driven sampling in RSM's indirect 
lighting.

This OpenGL 4.1 project creates a GLFW window and renders on it a scene with geometric and textured 3D object models.
To interact with the application click and drag to rotate the scene, press `L` to rotate the light source, press `C`
to rotate the camera, press `O` to enable/disable SSAO, and zoom in/out using the mouse scroll button.

All of the fonts, shaders, 3D object models, textures, and Poisson disks (2D points) must be located in a `Resources` directory,  and you 
should provide its path in the `Configuration.h` header file.

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
