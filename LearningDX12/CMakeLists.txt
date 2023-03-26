
cmake_minimum_required(VERSION 3.10)

project(SelfishGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Hide console
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_executable(${PROJECT_NAME} 
    main.cpp
)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")

add_subdirectory(Source)

target_link_libraries(${PROJECT_NAME} PUBLIC 
    Fbx
    Flecs
    ImGui
    Physics
    Renderer
)

target_include_directories(${PROJECT_NAME} PUBLIC
    "${PROJECT_SOURCE_DIR}/Source"
)

# target_compile_definitions(${PROJECT_NAME} PUBLIC
#     _WINDOWS
#     NOMINMAX
#     UNICODE
#     _UNICODE
# )

set (PHYSX5_DEBUG_LIB_PATH "${PROJECT_SOURCE_DIR}/3rdparty/PhysX5/lib/debug")
set (PHYSX5_RELEASE_LIB_PATH "${PROJECT_SOURCE_DIR}/3rdparty/PhysX5/lib/checked")

set(PHYSX5_LIBS 
    PhysXCommon_64
    PhysX_64
    PhysXFoundation_64
    PhysXCooking_64
    PhysXExtensions_static_64
    PhysXPvdSDK_static_64
)
foreach(LIB ${PHYSX5_LIBS})
    add_library(${LIB} UNKNOWN IMPORTED)
    set_target_properties(${LIB} PROPERTIES
        IMPORTED_LOCATION_RELEASE
            "${PHYSX5_RELEASE_LIB_PATH}/${LIB}.lib"
        IMPORTED_LOCATION_RELWITHDEBINFO
            "${PHYSX5_RELEASE_LIB_PATH}/${LIB}.lib"    
        IMPORTED_LOCATION_MINSIZEREL
            "${PHYSX5_RELEASE_LIB_PATH}/${LIB}.lib"    
        IMPORTED_LOCATION_DEBUG
            "${PHYSX5_DEBUG_LIB_PATH}/${LIB}.lib"        
    )
endforeach()


target_link_directories(${PROJECT_NAME}
    PUBLIC
        "${PROJECT_SOURCE_DIR}/3rdparty/FBXSDK_2020.0.1" 
)

target_link_libraries(${PROJECT_NAME} 
    PUBLIC
        D3d12.lib
        DXGI.lib
        D3Dcompiler.lib
        dxguid.lib
        Windowsapp.lib
        libfbxsdk.lib
        PhysXCommon_64
        PhysX_64
        PhysXFoundation_64
        PhysXCooking_64
        PhysXExtensions_static_64
        PhysXPvdSDK_static_64
)



file(COPY "${PROJECT_SOURCE_DIR}/3rdparty/FBXSDK_2020.0.1/libfbxsdk.dll"  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(PHYSX5_SHARED_PATH "${PROJECT_SOURCE_DIR}/3rdparty/PhysX5/lib")

set(PHYSX5_BUILD_TYPE "checked")

    file(COPY_FILE "${PHYSX5_SHARED_PATH}/${PHYSX5_BUILD_TYPE}/PhysXCommon_64.dll"      "${CMAKE_CURRENT_BINARY_DIR}/PhysXCommon_64.dll" )
    file(COPY_FILE "${PHYSX5_SHARED_PATH}/${PHYSX5_BUILD_TYPE}/PhysX_64.dll"            "${CMAKE_CURRENT_BINARY_DIR}/PhysX_64.dll" )
    file(COPY_FILE "${PHYSX5_SHARED_PATH}/${PHYSX5_BUILD_TYPE}/PhysXCooking_64.dll"     "${CMAKE_CURRENT_BINARY_DIR}/PhysXCooking_64.dll" )
    file(COPY_FILE "${PHYSX5_SHARED_PATH}/${PHYSX5_BUILD_TYPE}/PhysXFoundation_64.dll"  "${CMAKE_CURRENT_BINARY_DIR}/PhysXFoundation_64.dll" )

file(COPY "${PROJECT_SOURCE_DIR}/Resources/model.fbx"  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${PROJECT_SOURCE_DIR}/Resources/texture.jpg"  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Build GLSL shaders
add_custom_target(shaders)

set(GLSL_SHADER_FILES 
    Resources/shaders/shaders.frag
    Resources/shaders/shaders.vert
)

source_group("Selfish Shaders" FILES ${GLSL_SHADER_FILES})

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

file(COPY "${PROJECT_SOURCE_DIR}/Resources/shaders/shader.frag"  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${PROJECT_SOURCE_DIR}/Resources/shaders/shader.vert"  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_custom_command(
    TARGET shaders
    COMMAND ${CMAKE_BINARY_DIR}/debug/glslangValidator.exe -V ${CMAKE_BINARY_DIR}/shader.frag -o ${CMAKE_BINARY_DIR}/shaders/frag.spv
    MAIN_DEPENDENCY ${FILE}
    COMMENT "Compiling shader.frag..."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)

add_custom_command(
    TARGET shaders
    COMMAND ${CMAKE_BINARY_DIR}/debug/glslangValidator.exe -V ${CMAKE_BINARY_DIR}/shader.vert -o ${CMAKE_BINARY_DIR}/shaders/vert.spv
    MAIN_DEPENDENCY ${FILE}
    COMMENT "Compiling shader.vert..."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)

# foreach(FILE ${GLSL_SHADER_FILES})
#   get_filename_component(FILE_WE ${FILE} NAME_WE)
#   get_source_file_property(shadertype ${FILE} ShaderType)
#   get_source_file_property(shadermodel ${FILE} ShaderModel)
#   add_custom_command(TARGET shaders
#                      COMMAND fxc.exe /nologo /Emain /T${shadertype}_${shadermodel} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fo ${CMAKE_BINARY_DIR}/${FILE_WE}.cso /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${FILE}
#                      MAIN_DEPENDENCY ${FILE}
#                      COMMENT "HLSL ${FILE}"
#                      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#                      VERBATIM)
# endforeach(FILE)

add_dependencies(${PROJECT_NAME} shaders)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set_target_properties(glslangValidator PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
