# models/CMakeLists.txt
cmake_minimum_required(VERSION 3.25.3)

add_custom_target(Models ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/resources/models
    ${PROJECT_BINARY_DIR}/resources/models
    COMMENT "Copying models into binary directory"
)