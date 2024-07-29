# shaders/CMakeLists.txt
cmake_minimum_required(VERSION 3.25.3)

# Specify the GLSL compiler
find_program(GLSLC_EXECUTABLE glslc REQUIRED)

# Shader sources
set(VERTEX_SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/shader.vert.glsl)
set(FRAGMENT_SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/shader.frag.glsl)
set(COMPUTE_SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/shader.comp.glsl)

# Shader targets
set(VERTEX_SHADER_SPV ${CMAKE_CURRENT_BINARY_DIR}/shader.vert.spv)
set(FRAGMENT_SHADER_SPV ${CMAKE_CURRENT_BINARY_DIR}/shader.frag.spv)
set(COMPUTE_SHADER_SPV ${CMAKE_CURRENT_BINARY_DIR}/shader.comp.spv)

# Add custom commands to compile shaders
add_custom_command(
        OUTPUT ${VERTEX_SHADER_SPV}
        COMMAND ${GLSLC_EXECUTABLE} -fshader-stage=vertex ${VERTEX_SHADER_SOURCE} -o ${VERTEX_SHADER_SPV}
        DEPENDS ${VERTEX_SHADER_SOURCE}
        COMMENT "Compiling vertex shader"
)

add_custom_command(
        OUTPUT ${FRAGMENT_SHADER_SPV}
        COMMAND ${GLSLC_EXECUTABLE} -fshader-stage=fragment ${FRAGMENT_SHADER_SOURCE} -o ${FRAGMENT_SHADER_SPV}
        DEPENDS ${FRAGMENT_SHADER_SOURCE}
        COMMENT "Compiling fragment shader"
)

add_custom_command(
        OUTPUT ${COMPUTE_SHADER_SPV}
        COMMAND ${GLSLC_EXECUTABLE} -fshader-stage=compute ${COMPUTE_SHADER_SOURCE} -o ${COMPUTE_SHADER_SPV}
        DEPENDS ${COMPUTE_SHADER_SOURCE}
        COMMENT "Compiling compute shader"
)

# Custom target to build all shaders
add_custom_target(Shaders
        ALL
        DEPENDS ${VERTEX_SHADER_SPV} ${FRAGMENT_SHADER_SPV} ${COMPUTE_SHADER_SPV}
        COMMENT "Building all shaders"
)