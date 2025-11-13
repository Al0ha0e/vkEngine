import os
import shutil

# env = Environment(tools=['mingw'])


def GetFileWithExt(dir,ext):
    ret = []
    for _,dir,files in os.walk(dir):
        for filename in files:
            _,suf = os.path.splitext(filename)
            if suf == ext:
                ret.append(filename)
    return ret

dlls = GetFileWithExt("./libs",".dll")
for filename in dlls:
    shutil.copy("./libs/"+filename,"./out/"+filename)

selflibs = GetFileWithExt("./libs",".lib")
            
print(dlls)
print(selflibs)

######################### JOLT #########################
USE_AVX512 = False
USE_AVX2 = True
USE_AVX = True
USE_FMADD = False
CROSS_PLATFORM_DETERMINISTIC = True
OBJECT_LAYER_BITS = 0
DEBUG = False

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
    "JPH_USE_F16C"
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


if USE_AVX512:
    jolt_ccflags.append("/arch:AVX512")
elif USE_AVX2:
    jolt_ccflags.append("/arch:AVX2")
elif USE_AVX:
    jolt_ccflags.append("/arch:AVX")

if USE_AVX512:
    jolt_cppdefines.append("JPH_USE_AVX512")
if USE_AVX2:
    jolt_cppdefines.append("JPH_USE_AVX2")
if USE_AVX:
    jolt_cppdefines.append("JPH_USE_AVX")

#########################

env = Environment(CC = 'cl',
                   CCFLAGS = ['/std:c++23preview','/EHsc','/O2','/utf-8'] + jolt_ccflags)

joltObjs = SConscript(['third_party/Jolt/Sconscript'], exports=['env','jolt_cppdefines'])


libs = ['msvcrtd', 'libcmt', 'Gdi32', 'shell32', 'user32','vulkan-1'] + selflibs
libpath = ['./libs','D:/VulkanSDK/Lib']
cpppath = ['./include','D:/VulkanSDK/Include','./third_party/spirv_reflect','./third_party/']
cppdefines = [] # ['NDEBUG']
commonsrc = joltObjs + ["./src/render/environment.cpp", "./src/asset.cpp", "./src/loader.cpp", "./src/builtin.cpp",
                "./src/render/descriptor.cpp", "./src/render/skybox_render.cpp","./src/render/gbuffer_pass.cpp", "./src/render/deferred_lighting.cpp",
                "./src/render/shader.cpp", "./src/render/pipeline.cpp","./src/render/light.cpp",
                "./src/render/render.cpp", "./src/render/frame_graph.cpp", "./src/render/queue.cpp", 
                "./src/gameobject.cpp", "./src/scene.cpp", "./src/event.cpp", "./src/engine.cpp", 
                "./src/input.cpp", "./src/time.cpp", "./src/logger.cpp", "./src/physics/physics.cpp",
                "./third_party/spirv_reflect/spirv_reflect.cpp","./third_party/vma/vma.cpp","./third_party/stb/stb_image.cpp","./third_party/tinygltf/tiny_gltf.cpp"]

### tools

toolCommonSrc = ["./src/logger.cpp"]

targetinfo = [
    ["tools/gltf_conv", ["./third_party/stb/stb_image.cpp", "./third_party/tinygltf/tiny_gltf.cpp", "./src/tools/gltf_conv.cpp"]],
    ["tools/obj_conv", ["./src/tools/obj_conv.cpp"]]
]

for info in targetinfo:
    env.Program(info[0],
        toolCommonSrc + info[1],
        LIBS=libs, LIBPATH=libpath, CPPPATH=cpppath, CPPDEFINES=cppdefines + jolt_cppdefines)

### tests

targetinfo = [
    ["out/test_env",["./tests/test_env.cpp"]],
    ["out/test_sponza",["./tests/test_sponza.cpp"]],
    ["out/test_jolt", ["./tests/test_jolt.cpp"]],
    ["out/test_compute",["./tests/test_compute.cpp"]],
    ["out/test_spvrefl",["./tests/test_spvrefl.cpp"]]
]

for info in targetinfo:
    env.Program(info[0],
        info[1]+commonsrc,
        LIBS=libs, LIBPATH=libpath, CPPPATH=cpppath, CPPDEFINES=cppdefines + jolt_cppdefines)


env.Library("out/vkengine",commonsrc,
                        LIBS=libs, LIBPATH=libpath, CPPPATH=cpppath, CPPDEFINES=cppdefines + jolt_cppdefines)

### shader

glslc = 'glslc --target-env=vulkan1.4'

sprefix = './builtin_assets/shader/'
shaders = [
    ['default.frag','default_frag.spv'],
    ['default.vert','default_vert.spv'],
    ['default_multi.frag','default_multi_frag.spv'],
    ['default_multi.vert','default_multi_vert.spv'],
    ['deferred_lighting.frag','deferred_lighting_frag.spv'],
    ['deferred_lighting.vert','deferred_lighting_vert.spv'],
    ['skybox.frag','skyfrag.spv'],
    ['skybox.vert','skyvert.spv'],
    ['sky_lut.comp','sky_lut.spv']
]

for s in shaders:
    print(glslc + f" {sprefix+s[0]} -I {sprefix} -o {sprefix+s[1]}")
    os.system(glslc + f" {sprefix+s[0]} -I {sprefix} -o {sprefix+s[1]}")

sprefix = './tests/shader/'
shaders = [
    ['test.comp','test_compute.spv'],
]
for s in shaders:
    print(glslc + f" {sprefix+s[0]} -I {sprefix} -o {sprefix+s[1]}")
    os.system(glslc + f" {sprefix+s[0]} -I {sprefix} -o {sprefix+s[1]}")

cmds = ["python ./tools/gen_transmittance_lut.py ./builtin_assets/texture/ ./builtin_assets/config/atmosphere_param.json"]
for cmd in cmds:
    print(cmd)
    os.system(cmd)