
set (LIB_NAME "ImGui")

add_library(${LIB_NAME}
    imconfig.h
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_internal.h
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui.cpp
    imgui.h
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h

    backends/imgui_impl_vulkan.cpp
    backends/imgui_impl_vulkan.h
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_glfw.h
)

target_include_directories(${LIB_NAME} PUBLIC
    "${PROJECT_SOURCE_DIR}/Source"
    "${PROJECT_SOURCE_DIR}/Source/ImGui"
    "${PROJECT_SOURCE_DIR}/Source/Renderer/vulkan/include"
)

set_target_properties(${LIB_NAME} PROPERTIES FOLDER "Selfish")

target_link_libraries(${LIB_NAME} PUBLIC 
    glfw   
    glm
    vulkan-1.lib
)
