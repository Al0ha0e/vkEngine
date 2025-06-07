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

env = Environment(CC = 'cl',
                   CCFLAGS = ['/std:c++17','/EHsc','/O2'])

libs = ['msvcrtd', 'libcmt', 'Gdi32', 'shell32', 'user32','vulkan-1'] + selflibs
libpath = ['./libs','D:/VulkanSDK/Lib']
cpppath = ['./include',"./include/physx/",'D:/VulkanSDK/Include','./third_party/spirv_reflect','./third_party/vma']
cppdefines = ['NDEBUG']
commonsrc = ["./src/render/environment.cpp", "./src/asset.cpp", "./src/loader.cpp", "./src/builtin.cpp",
                "./src/render/descriptor.cpp", "./src/render/base_render.cpp","./src/render/opaque_render.cpp", "./src/render/shader.cpp",
                "./src/render/render.cpp", "./src/render/frame_graph.cpp", "./src/gameobject.cpp", "./src/scene.cpp", "./src/event.cpp", "./src/engine.cpp", 
                "./src/input.cpp", "./src/time.cpp", "./src/physics.cpp","./third_party/spirv_reflect/spirv_reflect.cpp","./third_party/vma/vma.cpp"]

targetinfo = [
    ["out/test", ["./tests/test.cpp" ]],
    ["out/test_env",["./tests/test_env.cpp"]],
    ["out/test_physx",["./tests/test_physx.cpp"]],
    ["out/test_compute",["./tests/test_compute.cpp"]],
    ["out/test_spvrefl",["./tests/test_spvrefl.cpp"]]
]

for info in targetinfo:
    env.Program(info[0],
        info[1]+commonsrc,
        LIBS=libs, LIBPATH=libpath, CPPPATH=cpppath, CPPDEFINES=cppdefines,
        SCONS_CXX_STANDARD="c++17")


env.Library("out/vkengine",commonsrc,
                        LIBS=libs, LIBPATH=libpath, CPPPATH=cpppath, CPPDEFINES=cppdefines,
                        SCONS_CXX_STANDARD="c++17")

### shader

glslc = 'glslc'

sprefix = './builtin_assets/shader/'
shaders = [
    ['default.frag','default_frag.spv'],
    ['default.vert','default_vert.spv'],
    ['skybox.frag','skyfrag.spv'],
    ['skybox.vert','skyvert.spv'],
]


for s in shaders:
    print(glslc + f" {sprefix+s[0]} -o {sprefix+s[1]}")
    os.system(glslc + f" {sprefix+s[0]} -o {sprefix+s[1]}")