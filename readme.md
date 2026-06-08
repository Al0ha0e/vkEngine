# vkEngine 

![](./docs/imgs/sponza.jpg)

> This branch provides experimental support for Linux. The Linux build path is still being validated and may require small local adjustments depending on the distribution, driver, compiler, and SDK layout.

vkEngine is a toy game engine currently in development. 
It uses [Vulkan](https://www.lunarg.com/vulkan-sdk/) as the rendering backend, integrates [JoltPhysics](https://github.com/jrouwe/JoltPhysics) for real-time physics simulation, and embeds [CoreCLR](https://github.com/dotnet/runtime) as the scripting runtime for C#. 

The project is primarily intended as a platform for experimenting with graphics and game development techniques. 

## Requirements 

- Linux x86_64
- A C++23-capable compiler, such as recent GCC or Clang
- [Miniconda/Anaconda](https://docs.conda.io/)
- [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) >= 1.4, including `glslc`
- A Vulkan-capable GPU driver
- [.NET SDK 9.0](https://dotnet.microsoft.com/)
- `glfw` and `assimp` development libraries, available from conda-forge or your system package manager

## Environment Setup

The build script looks for third-party headers and libraries in the active conda environment, the Vulkan SDK, and the .NET installation. A typical conda-based setup is:

```bash
git clone --recursive https://github.com/Al0ha0e/vkEngine.git
cd vkEngine

conda create -n vkengine python=3.12
conda activate vkengine
conda install -c conda-forge glfw assimp
pip install -r requirements.txt

export VULKAN_SDK=$HOME/VulkanSDK/<version>/x86_64
export DOTNET_ROOT=$HOME/.dotnet
```

If Vulkan or .NET are installed in different locations, adjust `VULKAN_SDK` and `DOTNET_ROOT` accordingly. `SConstruct` can also discover Vulkan under `~/VulkanSDK/*/x86_64` and .NET under `~/.dotnet`.

## Build

```bash
python init_project.py
scons
```

`SConstruct` will also:

- build `csharp/EngineCore`
- build the test gameplay assembly under `tests/csharp`
- run the reflection code generator for `GameConfig`
- compile the builtin shaders

`init_project.py` creates the local build/generated directories and generates the builtin LUT textures needed by the renderer.

## Run

The Linux build will produce `out/engine`. The executable expects a single game config json:

```bash
./out/engine ./tests/cfg/test_sponza.json
./out/engine ./tests/cfg/test_jolt.json
./out/engine ./tests/cfg/test_anim.json
./out/engine ./tests/cfg/test_env.json
./out/engine ./tests/cfg/test_render.json
```

## Third Party Libraries 

- [glfw](https://github.com/glfw/glfw) 
- [SPIRV-Reflect](https://github.com/KhronosGroup/SPIRV-Reflect) 
- [stb_image](https://github.com/nothings/stb) 
- [assimp](https://github.com/assimp/assimp) 
- [tinygltf](https://github.com/syoyo/tinygltf)
- [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [nlohmann/json](https://github.com/nlohmann/json)
- [spdlog](https://github.com/gabime/spdlog)  
- [JoltPhysics](https://github.com/jrouwe/JoltPhysics) 
- [ozz-animation](https://github.com/guillaumeblanc/ozz-animation)
- [entt](https://github.com/skypjack/entt)
- [freetype](https://freetype.org/)


## Roadmap 

- **Frame Graph**:
  - [x] support for multiple queue families 
  - [x] transient resource allocation 
  - [ ] transient resource schedule optimize
- **Rendering**:
  - [x] cluster-based deferred rendering & PBR 
  - [x] directional/spot/point lights
  - [x] IBL
  - [x] SSAO
  - [x] post-processing: bloom, tone mapping
  - [x] directional light CSM with PCF
  - [ ] UI rendering
  - [ ] spot and point light shadows
- **Physics**:
  - [x] rigidbody
  - [x] collision detection
  - [x] spatial queries
  - [x] character controller
  - [ ] constraints
- **Animation**:
  - [x] animation sampling
  - [x] GPU skinning
  - [ ] blending
  - [ ] IK
- **Scripting**:
  - [x] CoreCLR integration
  - [x] script lifecycle
  - [ ] python-based preprocessing tool to generate metadata 
