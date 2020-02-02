# RealTimeEngine

RealTimeEngine is a graphic visualization application developed from scratch using C++14, OpenGL 2.0 and shaders GLSL. The program uses OpenGL for rendering and GLFW for window management and user input. It showcases the following concepts:

* Vertex shaders.
* Fragment shaders.
* Reflection and refraction environment mapping effects using cubemaps.
* Window management with GLFW.
* Creation of a generic scene graph from a json file.
* Model loading with the Assimp library.
* Unit testing with Google Test.

# Building on Linux

1. In order to build you'll need a g++-4.9 for the C++14 features. In Ubuntu 14.04 you can install it with the following commands:
  * `$ sudo add-apt-repository ppa:ubuntu-toolchain-r/test`
  * `$ sudo apt-get update`
  * `$ sudo apt-get install g++-4.9`
2. Download the code or clone the repo
3. Cd to the directory where you downloaded the code
4. Build makefiles
   * `$ mkdir build`
   * `$ cd build`
   * `$ cmake ..`
5. Build the code
   * `$ make`

# Running

1. Run RealTimeEngine
  * `cd build/rte`
  * `./rte -config ../../config.json`
2. Use W, A, S and D to move and the mouse to look around.
