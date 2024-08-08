# Vulkan Compute with Graphics [![Licence](https://img.shields.io/github/license/Michael-Warrick/Vulkan-Compute-with-Graphics?style=flat)](./LICENSE)

![GIF of simulation running](/resources/screenshots/gpgpu_physics_simulation_demo.gif?raw=true)
*Demo running on an M3 Macbook Pro*

## About
Vulkan Compute with Graphics is a demonstration of a theoretical concept, where physics objects and their relationships with the simulation world are entirely simulated on the GPU. This that for the entire length of the runtime, there is no copying of data from the host (CPU) to the local device (GPU) and all `PhysicsObject` data is stored in local device memory.

## Requirements
[Vulkan SDK 1.2 or later](https://www.lunarg.com/vulkan-sdk/)\
[CMake 3.25.3 or later](https://cmake.org/)

([Vulkan packages](https://vulkan-tutorial.com/Development_environment) if building on Linux)


## Getting Started
### Clone the repository with the `--recursive` flag:
```shell
$ git clone --recursive https://github.com/Michael-Warrick/Vulkan-Compute-with-Graphics.git
```
### Navigate to the project's root directory and create a `build/` folder:
```shell
$ mkdir build
```
### Enter the `build/` folder:
```shell
$ cd build
```

### Generate CMake files:
```shell
$ cmake ..
```

### Build CMake project in `Release` mode:
```shell
$ cmake --build . --config Release
```

### Run the executable (use platform specific extension):
```shell
$ ./Vulkan-Compute-with-Graphics.exe
```
```shell
$ ./Vulkan-Compute-with-Graphics
```

## Dependencies
[GLFW](https://github.com/glfw/glfw) - Cross-platform windowing API.\
[GLM](https://github.com/g-truc/glm) - Mathematics library.\
[ImGui](https://github.com/ocornut/imgui) - Immediate mode GUI library.\
[stb_image](https://github.com/nothings/stb/tree/master) - Helper library for loading common image formats.\
[tiny_obj_loader](https://github.com/tinyobjloader/tinyobjloader) - Helper library for loading OBJ (Wavefront) 3D models.\
[Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp) - C++ Bindings for Vulkan

## Resources
[Vulkan Documentation](https://docs.vulkan.org/spec/latest/index.html)\
[Vulkan Tutorial](https://vulkan-tutorial.com/)\
[Sascha Willems: Vulkan C++ examples and demos](https://github.com/SaschaWillems/Vulkan)