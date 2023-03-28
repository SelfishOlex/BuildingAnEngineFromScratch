
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_BUILD_DOCS OFF)
add_subdirectory(glfw-3.3.8)

add_subdirectory(glm-0.9.9.8)

set(ENABLE_HLSL OFF)
set(ENABLE_CTEST OFF)
set(ENABLE_SPVREMAPPER OFF)
set(ENABLE_GLSLANG_INSTALL ON)
add_subdirectory(glslang-12.1.0)
#set_target_properties(Renderer PROPERTIES FOLDER "Selfish")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")


add_library(Renderer
    GpuImage.cpp
    GpuImage.h

    Renderer.cpp
    Renderer.h
    Vertex.cpp
    Vertex.h
    stb_image.cpp

    Fbx/FbxLoader.cpp
    Fbx/FbxLoader.h
)

target_include_directories(Renderer PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/Source"
    "${PROJECT_SOURCE_DIR}/Source/ImGui"
    "${PROJECT_SOURCE_DIR}/Source/Renderer"
    "${PROJECT_SOURCE_DIR}/Source/Renderer/vulkan/include"
    "${PROJECT_SOURCE_DIR}/3rdparty/FBXSDK_2020.0.1/include"
)

target_include_directories(Renderer INTERFACE
    "${PROJECT_SOURCE_DIR}/Source/Renderer"
)


target_compile_definitions(Renderer PUBLIC
)

target_include_directories(Renderer 
    PUBLIC    
)

target_link_directories(Renderer
    PUBLIC
        "${PROJECT_SOURCE_DIR}/Source/Renderer/vulkan/lib"
)

target_link_libraries(Renderer PUBLIC 
    Flecs
    Physics 
    glfw   
    glm
    vulkan-1.lib
)

set_target_properties(Renderer PROPERTIES FOLDER "Selfish")
set_target_properties(glfw PROPERTIES FOLDER "3rdparty")
set_target_properties(glm PROPERTIES FOLDER "3rdparty")
