# env = Environment(tools=['mingw'])

env = Environment(CC = 'cl',
                   CCFLAGS = ['/std:c++17','/EHsc'])

env.Program("out/test",
        ["./tests/test.cpp" ],
        LIBS=['msvcrtd', 'libcmt', 'Gdi32', 'shell32', 'user32','vulkan-1', 'glfw3'], LIBPATH=['./libs','D:/VulkanSDK/Lib'], CPPPATH=['./include','D:/VulkanSDK/Include'],
        SCONS_CXX_STANDARD="c++17")

env.Program("out/test_env",
            ["./tests/test_env.cpp", "./src/render/environment.cpp", "./src/render/resource.cpp",
                "./src/render/descriptor.cpp", "./src/render/opaque_render.cpp", "./src/engine.cpp"],
        LIBS=['msvcrtd', 'libcmt', 'Gdi32', 'shell32', 'user32','vulkan-1', 'glfw3'], LIBPATH=['./libs','D:/VulkanSDK/Lib'], CPPPATH=['./include','D:/VulkanSDK/Include'],
        SCONS_CXX_STANDARD="c++17")

# Program("test",
#         ["./src/stb_image.cpp",
#          "./src/events/event.cpp",
#          "./src/render/renderer.cpp",
#          "./src/render/render_queue.cpp",
#          "./src/render/skybox.cpp",
#          "./src/render/light.cpp",
#          "./src/common/common.cpp",
#          "./src/common/game_object.cpp",
#          "./src/glad.c",
#          "./src/test.cpp", 
#          "./src/resource/resource.cpp", ],
#         LIBS=['msvcrtd', 'libcmt', 'Gdi32', 'shell32', 'user32', 'opengl32', 'glfw3'], LIBPATH=['./libs'], CPPPATH=['./include'])

# Program("gen_six",
#         ["./tools/irr_gen/gen_six.cpp",
#          "./src/common/common.cpp",
#          "./src/stb_image.cpp",
#          "./src/glad.c", ],
#         LIBS=['msvcrtd', 'libcmt', 'Gdi32', 'shell32', 'user32', 'opengl32', 'glfw3'], LIBPATH=['./libs'], CPPPATH=['./include'])

# Program("gen_one",
#         ["./tools/irr_gen/gen_one.cpp",
#          "./src/common/common.cpp",
#          "./src/stb_image.cpp",
#          "./src/glad.c", ],
#         LIBS=['msvcrtd', 'libcmt', 'Gdi32', 'shell32', 'user32', 'opengl32', 'glfw3'], LIBPATH=['./libs'], CPPPATH=['./include'])


# Program("texture_conv",
#         ["./tools/texture_conv/texture_conv.cpp",
#          "./src/common/common.cpp",
#          "./src/stb_image.cpp",
#          "./src/glad.c", ],
#         LIBS=['msvcrtd', 'libcmt', 'Gdi32', 'shell32', 'user32', 'opengl32', 'glfw3'], LIBPATH=['./libs'], CPPPATH=['./include'])
