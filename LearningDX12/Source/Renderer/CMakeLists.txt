
add_subdirectory(glfw-3.3.8)
add_subdirectory(glm-0.9.9.8)

add_library(Renderer
    Renderer.cpp
    Renderer.h
)

target_include_directories(Renderer PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/Source"
    "${PROJECT_SOURCE_DIR}/Source/ImGui"
    "${PROJECT_SOURCE_DIR}/Source/Renderer"
    "${PROJECT_SOURCE_DIR}/Source/Renderer/vulkan/include"
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
    Fbx
    Flecs
    Physics 
    glfw   
    glm
    vulkan-1.lib
)

set_target_properties(Renderer PROPERTIES FOLDER "Selfish")
set_target_properties(glfw PROPERTIES FOLDER "3rdparty")
set_target_properties(glm PROPERTIES FOLDER "3rdparty")
