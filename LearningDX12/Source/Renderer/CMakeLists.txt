
add_subdirectory(DirectXTK12-main)

add_library(Renderer
    BaseGameInterface.cpp
    BaseGameInterface.h
    CommandQueue.cpp
    CommandQueue.h
    d3dx12.h
    DemoBoxGame.cpp
    DemoBoxGame.h
    DX12App.cpp
    DX12App.h
    LightingTexturedDemoBoxGame.cpp
    LightingTexturedDemoBoxGame.h
    MultipleObjectsDemo.cpp
    MultipleObjectsDemo.h
    Resource.h
    targetver.h
    TexturedDemoBoxGame.cpp
    TexturedDemoBoxGame.h
)

target_include_directories(Renderer PUBLIC
    "${PROJECT_SOURCE_DIR}/Source"
    "${PROJECT_SOURCE_DIR}/Source/Renderer"
)


target_compile_definitions(Renderer PUBLIC
    _WINDOWS
    NOMINMAX
    UNICODE
)


target_link_libraries(Renderer PUBLIC 
    DirectXTK12
    Fbx
    Flecs
    Physics    
)
