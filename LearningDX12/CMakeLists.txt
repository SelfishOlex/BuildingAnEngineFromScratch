
cmake_minimum_required(VERSION 3.10)

project(SelfishGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")

add_executable(${PROJECT_NAME} 
    LearningDX12.cpp
    LearningDX12.h
    Source/framework.h
    LearningDX12.rc
)

add_subdirectory(Source)

target_link_libraries(${PROJECT_NAME} PUBLIC 
    Fbx
    Flecs
    Physics
    Renderer
)

target_include_directories(${PROJECT_NAME} PUBLIC
    "${PROJECT_SOURCE_DIR}/Source"
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    _WINDOWS
    NOMINMAX
    UNICODE
    _UNICODE
)


target_link_directories(${PROJECT_NAME}
    PUBLIC
        "${PROJECT_SOURCE_DIR}/3rdparty/FBXSDK_2020.0.1" 
        "${PROJECT_SOURCE_DIR}/3rdparty/PhysX5/lib" 
)

target_link_libraries(${PROJECT_NAME} 
    PUBLIC
        D3d12.lib
        DXGI.lib
        D3Dcompiler.lib
        dxguid.lib
        Windowsapp.lib
        libfbxsdk.lib
        PhysXCommon_64.lib
        PhysX_64.lib
        PhysXFoundation_64.lib
        PhysXCooking_64.lib
        PhysXExtensions_static_64.lib
        PhysXPvdSDK_static_64.lib
)

file(COPY "${PROJECT_SOURCE_DIR}/3rdparty/FBXSDK_2020.0.1/libfbxsdk.dll"  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${PROJECT_SOURCE_DIR}/3rdparty/PhysX5/lib/debug/PhysXCommon_64.dll"  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${PROJECT_SOURCE_DIR}/3rdparty/PhysX5/lib/debug/PhysX_64.dll"  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${PROJECT_SOURCE_DIR}/3rdparty/PhysX5/lib/debug/PhysXCooking_64.dll"  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${PROJECT_SOURCE_DIR}/3rdparty/PhysX5/lib/debug/PhysXFoundation_64.dll"  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


file(COPY "${PROJECT_SOURCE_DIR}/Resources/model.fbx"  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${PROJECT_SOURCE_DIR}/Resources/texture.jpg"  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Build HLSL shaders
add_custom_target(shaders)

set(HLSL_SHADER_FILES 
    Resources/PixelShader_Textured_Light.hlsl
    Resources/VertexShader_Textured_Light.hlsl
)

set_source_files_properties(Resources/VertexShader_Textured_Light.hlsl PROPERTIES ShaderType "vs")
set_source_files_properties(Resources/PixelShader_Textured_Light.hlsl PROPERTIES ShaderType "ps")
set_source_files_properties(${HLSL_SHADER_FILES} PROPERTIES ShaderModel "5_1")

foreach(FILE ${HLSL_SHADER_FILES})
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  get_source_file_property(shadertype ${FILE} ShaderType)
  get_source_file_property(shadermodel ${FILE} ShaderModel)
  add_custom_command(TARGET shaders
                     COMMAND fxc.exe /nologo /Emain /T${shadertype}_${shadermodel} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fo ${CMAKE_BINARY_DIR}/${FILE_WE}.cso /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${FILE}
                                    #fxc.exe /?
                                    #fxc.exe /Zi /E"main" /Od /Fo"${CMAKE_BINARY_DIR}/${FILE_WE}.cso" /vs"_5_1" /nologo
                                    #/nologo /Emain /vs"_5_1" /Zi /Fo ${CMAKE_BINARY_DIR}/${FILE_WE}.cso /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${FILE}
                     MAIN_DEPENDENCY ${FILE}
                     COMMENT "HLSL ${FILE}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)
endforeach(FILE)

add_dependencies(${PROJECT_NAME} shaders)
