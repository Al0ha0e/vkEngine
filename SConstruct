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
cpppath = ['./include',"./include/physx/",'D:/VulkanSDK/Include']
cppdefines = ['NDEBUG']
commonsrc = ["./src/render/environment.cpp", "./src/asset.cpp", "./src/loader.cpp", "./src/builtin.cpp",
                "./src/render/descriptor.cpp", "./src/render/base_render.cpp","./src/render/opaque_render.cpp",
                "./src/render/render.cpp", "./src/gameobject.cpp", "./src/scene.cpp", "./src/event.cpp", "./src/engine.cpp", 
                "./src/input.cpp", "./src/time.cpp", "./src/physics.cpp"]

targetinfo = [
    ["out/test", ["./tests/test.cpp" ]],
    ["out/test_env",["./tests/test_env.cpp"]],
    ["out/test_physx",["./tests/test_physx.cpp"]],
    ["out/test_compute",["./tests/test_compute.cpp"]]
]

for info in targetinfo:
    env.Program(info[0],
        info[1]+commonsrc,
        LIBS=libs, LIBPATH=libpath, CPPPATH=cpppath, CPPDEFINES=cppdefines,
        SCONS_CXX_STANDARD="c++17")


env.Library("out/vkengine",commonsrc,
                        LIBS=libs, LIBPATH=libpath, CPPPATH=cpppath, CPPDEFINES=cppdefines,
                        SCONS_CXX_STANDARD="c++17")
