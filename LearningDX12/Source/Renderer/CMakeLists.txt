
add_library(Renderer
    D3D12HelloTexture.cpp
    D3D12HelloTexture.h
    d3dx12.h
    DXSample.cpp
    DXSample.h
    DXSampleHelper.h
    Resource.h
    targetver.h
)

target_include_directories(Renderer PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/Source"
    "${PROJECT_SOURCE_DIR}/Source/ImGui"
    "${PROJECT_SOURCE_DIR}/Source/Renderer"
)


target_compile_definitions(Renderer PUBLIC
    _WINDOWS
    NOMINMAX
    UNICODE
)


target_link_libraries(Renderer PUBLIC 
    Fbx
    Flecs
    Physics    
)

set_target_properties(Renderer PROPERTIES FOLDER "Selfish")
