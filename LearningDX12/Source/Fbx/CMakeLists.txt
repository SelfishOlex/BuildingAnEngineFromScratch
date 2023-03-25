
add_library(Fbx
    FbxLoader.cpp
    FbxLoader.h
)

message(PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

target_include_directories(Fbx 
    PUBLIC    
        "${PROJECT_SOURCE_DIR}/3rdparty/FBXSDK_2020.0.1/include"
)

target_include_directories(Fbx 
    INTERFACE 
        "${PROJECT_SOURCE_DIR}/Source/Fbx"
)

target_compile_definitions(Fbx PUBLIC
    FBXSDK_SHARED
)