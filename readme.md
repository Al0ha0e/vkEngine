# vkEngine 
vkEngine is a toy game engine currently in development. 
It uses [Vulkan](https://www.lunarg.com/vulkan-sdk/) as the rendering backend, integrates [JoltPhysics](https://github.com/jrouwe/JoltPhysics) for real-time physics simulation, and plans to support [CoreCLR](https://github.com/dotnet/runtime) as the scripting runtime for C#. 

The project is primarily intended as a platform for experimenting with graphics and game development techniques. 

## Requirements 

- Windows 10/11 
- [Visual Studio 2022](https://visualstudio.microsoft.com/) (with C++23 support)  
- [Miniconda/Anaconda](https://docs.conda.io/) 
- [VulkanSDK](https://www.lunarg.com/vulkan-sdk/) >= 1.4 

## Build

``` bash
git clone --recursive https://github.com/Al0ha0e/vkEngine.git
cd vkEngine
conda create -n vkengine python=3.12
conda activate vkengine
pip install -r requirements.txt
mkdir out
scons
```

## Run

The build will produce several test/demo executables. For example:

```
./out/test_env.exe
./out/test_jolt.exe
```

## Third Party Libraries 

- [assimp](https://github.com/assimp/assimp) 
- [glfw](https://github.com/glfw/glfw) 
- [JoltPhysics](https://github.com/jrouwe/JoltPhysics) 
- [nlohmann/json](https://github.com/nlohmann/json) 
- [SPIRV-Reflect](https://github.com/KhronosGroup/SPIRV-Reflect) 
- [stb_image](https://github.com/nothings/stb) 
- [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)


## Roadmap 

- **Frame Graph**: support for multiple queue families 
- **Rendering**: cluster-based deferred rendering & PBR 
- **C++ Reflection**: python-based preprocessing tool to generate metadata 
- **Scripting**: CoreCLR integration