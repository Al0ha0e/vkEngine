import glob
import os
import platform
import shutil

# env = Environment(tools=['mingw'])


def GetFileWithExt(dir, ext):
    ret = []
    for _, dirs, files in os.walk(dir):
        for filename in files:
            _, suf = os.path.splitext(filename)
            if suf == ext:
                ret.append(filename)
    return ret


is_windows = platform.system() == "Windows"
is_linux = platform.system() == "Linux"
out_dir = "./out"
os.makedirs(out_dir, exist_ok=True)

dlls = GetFileWithExt("./libs", ".dll") if is_windows else []
for filename in dlls:
    shutil.copy("./libs/" + filename, out_dir + "/" + filename)

selflibs = GetFileWithExt("./libs", ".lib") if is_windows else []

print(dlls)
print(selflibs)

######################### CodeGen ######################

files_need_reflect = ["game_config.hpp"]
for filename in files_need_reflect:
    cmd = f"python ./tools/codegen/reflect.py ./include/{filename}"
    print(cmd)
    os.system(cmd)

generated_files = Glob("./src/generated/*.cpp")

######################### JOLT #########################
USE_AVX512 = False
USE_AVX2 = True
USE_AVX = True
USE_FMADD = False
CROSS_PLATFORM_DETERMINISTIC = True
OBJECT_LAYER_BITS = 32
DEBUG = True

jolt_ccflags = []

jolt_cppdefines = [
    # "JPH_DISABLE_CUSTOM_ALLOCATOR",
    # "JPH_ENABLE_ASSERTS",
    "JPH_DOUBLE_PRECISION",
    # "JPH_USE_STD_VECTOR",
    # "JPH_TRACK_BROADPHASE_STATS",
    # "JPH_TRACK_NARROWPHASE_STATS",
    "JPH_OBJECT_STREAM",
    "JPH_USE_SSE4_1",
    "JPH_USE_SSE4_2",
    "JPH_USE_LZCNT",
    "JPH_USE_TZCNT",
    "JPH_USE_F16C",
]

if DEBUG:
    jolt_cppdefines.append("_DEBUG")
else:
    jolt_cppdefines.append("NDEBUG")

if OBJECT_LAYER_BITS > 0:
    jolt_cppdefines.append(f"JPH_OBJECT_LAYER_BITS={OBJECT_LAYER_BITS}")

if CROSS_PLATFORM_DETERMINISTIC:
    jolt_cppdefines.append("JPH_CROSS_PLATFORM_DETERMINISTIC")
elif USE_FMADD:
    jolt_cppdefines.append("JPH_USE_FMADD")


if is_windows:
    if USE_AVX512:
        jolt_ccflags.append("/arch:AVX512")
    elif USE_AVX2:
        jolt_ccflags.append("/arch:AVX2")
    elif USE_AVX:
        jolt_ccflags.append("/arch:AVX")
else:
    jolt_ccflags.extend(["-msse4.1", "-msse4.2", "-mlzcnt", "-mbmi", "-mf16c"])
    if USE_AVX512:
        jolt_ccflags.extend(["-mavx512f", "-mavx512vl", "-mavx512dq"])
    elif USE_AVX2:
        jolt_ccflags.append("-mavx2")
    elif USE_AVX:
        jolt_ccflags.append("-mavx")

if USE_AVX512:
    jolt_cppdefines.append("JPH_USE_AVX512")
if USE_AVX2:
    jolt_cppdefines.append("JPH_USE_AVX2")
if USE_AVX:
    jolt_cppdefines.append("JPH_USE_AVX")

#########################

if is_windows:
    env = Environment(
        CC="cl",
        CXX="cl",
        CCFLAGS=[
            "/std:c++23preview",
            "/EHs-",
            "/O2",
            "/utf-8",
            "/MDd" if DEBUG else "/MD",
        ]
        + jolt_ccflags,
    )
else:
    env = Environment(
        ENV=os.environ,
        CC=os.environ.get("CC", "gcc"),
        CXX=os.environ.get("CXX", "g++"),
        CCFLAGS=[
            "-std=c++23",
            "-O0" if DEBUG else "-O2",
            "-fno-exceptions",
            "-fPIC",
            "-Wall",
            "-Wextra",
            "-Wno-unused-parameter",
            "-Wno-missing-field-initializers",
        ]
        + jolt_ccflags,
        LINKFLAGS=[],
    )

SConscript(["csharp/Sconscript", "tests/csharp/Sconscript"])
joltObjs = SConscript(
    ["third_party/Jolt/Sconscript"], exports=["env", "jolt_cppdefines"]
)
ozzObjs = SConscript(["third_party/ozz/Sconscript"], exports=["env"])
freetypeObjs = SConscript(["third_party/freetype/Sconscript"], exports=["env"])


def find_latest_dir(pattern):
    matches = sorted(glob.glob(pattern))
    return matches[-1] if matches else None


libs = []
libpath = []
cpppath = [
    "./include",
    "./third_party/spirv_reflect",
    "./third_party/",
]
if is_windows:
    libs = ["Gdi32", "shell32", "user32", "vulkan-1"] + selflibs
    libpath = ["./libs", "D:/VulkanSDK/Lib"]
    cpppath.append("D:/VulkanSDK/Include")
else:
    conda_prefix = os.environ.get("CONDA_PREFIX")
    if not conda_prefix:
        conda_prefix = find_latest_dir(os.path.expanduser("~/miniconda3/envs/vkengine"))

    vulkan_sdk = os.environ.get("VULKAN_SDK")
    if not vulkan_sdk:
        vulkan_sdk = find_latest_dir(os.path.expanduser("~/VulkanSDK/*/x86_64"))

    libs = ["vulkan", "glfw", "assimp", "dl", "pthread"]
    if conda_prefix:
        cpppath.append(os.path.join(conda_prefix, "include"))
        libpath.append(os.path.join(conda_prefix, "lib"))
        env.PrependENVPath("PATH", os.path.join(conda_prefix, "bin"))
        env.PrependENVPath("LD_LIBRARY_PATH", os.path.join(conda_prefix, "lib"))
    if vulkan_sdk:
        cpppath.append(os.path.join(vulkan_sdk, "include"))
        libpath.append(os.path.join(vulkan_sdk, "lib"))
        env.PrependENVPath("PATH", os.path.join(vulkan_sdk, "bin"))
        env.PrependENVPath("LD_LIBRARY_PATH", os.path.join(vulkan_sdk, "lib"))

    env.Append(LINKFLAGS=[f"-Wl,-rpath,{path}" for path in libpath])

cpppath.append("./third_party/freetype/include")
cppdefines = ["JSON_NOEXCEPTION", "SPDLOG_NO_EXCEPTIONS"]
if DEBUG:
    cppdefines.append("VKE_DEBUG")
else:
    cppdefines.append("NDEBUG")
commonsrc = (
    joltObjs
    + ozzObjs
    + freetypeObjs
    + [
        "./src/render/environment.cpp",
        "./src/asset.cpp",
        "./src/loader.cpp",
        "./src/builtin.cpp",
        "./src/render/descriptor.cpp",
        "./src/render/skybox.cpp",
        "./src/render/skybox_render.cpp",
        "./src/render/gbuffer_pass.cpp",
        "./src/render/shadow_pass.cpp",
        "./src/render/ssao.cpp",
        "./src/render/deferred_lighting.cpp",
        "./src/render/atmosphere_pass.cpp",
        "./src/render/bloom.cpp",
        "./src/render/tone_mapping.cpp",
        "./src/render/shader.cpp",
        "./src/render/pipeline.cpp",
        "./src/render/light.cpp",
        "./src/render/ui.cpp",
        "./src/render/render.cpp",
        "./src/render/frame_graph.cpp",
        "./src/render/queue.cpp",
        "./src/component.cpp",
        "./src/scene.cpp",
        "./src/scene_transform_system.cpp",
        "./src/event.cpp",
        "./src/engine.cpp",
        "./src/engine_state.cpp",
        "./src/input.cpp",
        "./src/time.cpp",
        "./src/logger.cpp",
        "./src/physics/physics_config.cpp",
        "./src/physics/physics.cpp",
        "./src/script.cpp",
        "./third_party/spirv_reflect/spirv_reflect.cpp",
        "./third_party/vma/vma.cpp",
        "./third_party/stb/stb_image.cpp",
        "./third_party/tinygltf/tiny_gltf.cpp",
        "./src/interop/native.cpp",
        "./src/interop/physics.cpp",
    ]
    + generated_files
)

### coreclr
if is_windows:
    NETHOST_PATH = "C:/Program Files/dotnet/packs/Microsoft.NETCore.App.Host.win-x64/9.0.8/runtimes/win-x64/native/"
else:
    dotnet_root = os.environ.get("DOTNET_ROOT", os.path.expanduser("~/.dotnet"))
    runtime_id = "linux-x64"
    NETHOST_PATH = find_latest_dir(
        os.path.join(
            dotnet_root,
            "packs",
            f"Microsoft.NETCore.App.Host.{runtime_id}",
            "*",
            "runtimes",
            runtime_id,
            "native",
        )
    )
    if not NETHOST_PATH:
        raise RuntimeError("Cannot find .NET nethost native directory. Set DOTNET_ROOT or install the .NET host pack.")

cpppath.append(NETHOST_PATH)
libpath.append(NETHOST_PATH)
libs.append("nethost")
if is_linux:
    env.Append(LINKFLAGS=[f"-Wl,-rpath,{NETHOST_PATH}"])


### tools

toolCommonSrc = ["./src/logger.cpp"]
tool_out_dir = "./tools/out"

targetinfo = [
    [
        f"{tool_out_dir}/gltf_conv",
        [
            "./third_party/stb/stb_image.cpp",
            "./third_party/tinygltf/tiny_gltf.cpp",
            "./src/tools/gltf_conv.cpp",
        ],
    ],
    [
        f"{tool_out_dir}/gltf_skin_conv",
        [
            "./third_party/stb/stb_image.cpp",
            "./third_party/tinygltf/tiny_gltf.cpp",
            "./src/tools/gltf_skin_conv.cpp",
        ]
        + ozzObjs,
    ],
    [
        f"{tool_out_dir}/gltf_scene_conv",
        [
            "./third_party/stb/stb_image.cpp",
            "./third_party/tinygltf/tiny_gltf.cpp",
            "./src/tools/gltf_scene_conv.cpp",
        ],
    ],
    [
        f"{tool_out_dir}/gltf_log",
        [
            "./third_party/stb/stb_image.cpp",
            "./third_party/tinygltf/tiny_gltf.cpp",
            "./src/tools/gltf_log.cpp",
        ],
    ],
    [f"{tool_out_dir}/obj_conv", ["./src/tools/obj_conv.cpp"]],
]

for info in targetinfo:
    env.Program(
        info[0],
        toolCommonSrc + info[1],
        LIBS=libs,
        LIBPATH=libpath,
        CPPPATH=cpppath,
        CPPDEFINES=cppdefines + jolt_cppdefines,
    )

### tests

targetinfo = [
    ["out/test_spvrefl", ["./tests/test_spvrefl.cpp"]],
    ["out/engine", ["./src/main.cpp"]],
]

for info in targetinfo:
    env.Program(
        info[0],
        info[1] + commonsrc,
        LIBS=libs,
        LIBPATH=libpath,
        CPPPATH=cpppath,
        CPPDEFINES=cppdefines + jolt_cppdefines,
    )


env.Library(
    "out/vkengine",
    commonsrc,
    LIBS=libs,
    LIBPATH=libpath,
    CPPPATH=cpppath,
    CPPDEFINES=cppdefines + jolt_cppdefines,
)

### shader

glslc = "glslc --target-env=vulkan1.4"

sprefix = "./builtin_assets/shader/"
shaders = [
    ["default.frag", "default_frag.spv"],
    ["default.vert", "default_vert.spv"],
    ["default_skin.vert", "default_skin_vert.spv"],
    ["default_multi.frag", "default_multi_frag.spv"],
    ["default_multi.vert", "default_multi_vert.spv"],
    ["deferred_lighting.frag", "deferred_lighting_frag.spv"],
    ["deferred_lighting.vert", "deferred_lighting_vert.spv"],
    ["fullscreen_triangle.vert", "fullscreen_triangle_vert.spv"],
    ["ssao.frag", "ssao_frag.spv"],
    ["ssao_blur.frag", "ssao_blur_frag.spv"],
    ["atmosphere_post.frag", "atmosphere_post_frag.spv"],
    ["bloom.frag", "bloom_frag.spv"],
    ["tone_mapping.frag", "tone_mapping_frag.spv"],
    ["text.frag", "text_frag.spv"],
    ["text.vert", "text_vert.spv"],
    ["shadow.frag", "shadow_frag.spv"],
    ["shadow.vert", "shadow_vert.spv"],
    ["shadow_skin.vert", "shadow_skin_vert.spv"],
    ["skybox.frag", "skyfrag.spv"],
    ["skybox.vert", "skyvert.spv"],
    ["sky_lut.comp", "sky_lut.spv"],
    ["atmosphere_lut.comp", "atmosphere_lut.spv"],
    ["ibl_lut.comp", "ibl_lut.spv"],
    ["light_cull.comp", "light_cull.spv"],
]

for s in shaders:
    print(glslc + f" {sprefix+s[0]} -I {sprefix} -o {sprefix+s[1]}")
    os.system(glslc + f" {sprefix+s[0]} -I {sprefix} -o {sprefix+s[1]}")

sprefix = "./tests/shader/"
shaders = [
    ["material_constant.vert", "material_constant_vert.spv"],
    ["material_constant.frag", "material_constant_frag.spv"],
]
for s in shaders:
    print(glslc + f" {sprefix+s[0]} -I {sprefix} -o {sprefix+s[1]}")
    os.system(glslc + f" {sprefix+s[0]} -I {sprefix} -o {sprefix+s[1]}")
