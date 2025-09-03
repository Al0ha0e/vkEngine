import shutil
import os

Import('dlldst','reldir','env')

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
    shutil.copy("./libs/"+filename,dlldst+filename)

selflibs = GetFileWithExt("./libs",".lib")
            
print(dlls)
print(selflibs)

libs = ['msvcrtd', 'libcmt', 'Gdi32', 'shell32', 'user32','vulkan-1'] + selflibs
libpath = ['./libs','D:/VulkanSDK/Lib']
cpppath = ['./include', 'D:/VulkanSDK/Include']
cppdefines = ['NDEBUG','COMPILE_TO_LIB','REL_DIR='+reldir]
commonsrc = ["./src/render/environment.cpp", "./src/asset.cpp", "./src/loader.cpp", "./src/builtin.cpp",
                "./src/render/descriptor.cpp", "./src/render/base_render.cpp","./src/render/opaque_render.cpp",
                "./src/render/render.cpp", "./src/gameobject.cpp", "./src/scene.cpp", "./src/event.cpp", "./src/engine.cpp", 
                "./src/input.cpp", "./src/time.cpp", "./src/physics.cpp"]

env.Library("../libs/vkengine",commonsrc,
                        LIBS=libs, LIBPATH=libpath, CPPPATH=cpppath, CPPDEFINES=cppdefines,
                        SCONS_CXX_STANDARD="c++20")
