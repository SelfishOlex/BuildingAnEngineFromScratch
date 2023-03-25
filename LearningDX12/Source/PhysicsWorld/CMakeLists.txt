
add_library(Physics
    PhysxWorld.cpp
    PhysxWorld.h
)

target_include_directories(Physics PUBLIC
    "${PROJECT_SOURCE_DIR}/Source"
    "${PROJECT_SOURCE_DIR}/3rdparty/PhysX5/include"
)

target_include_directories(Physics
    INTERFACE 
        "${PROJECT_SOURCE_DIR}/3rdparty/PhysX5/include"
)