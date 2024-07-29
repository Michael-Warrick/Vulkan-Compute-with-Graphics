cmake_minimum_required(VERSION 3.25.3)
project(Vulkan-Compute-with-Graphics VERSION 1.0.0)

# Setting C++ Version to 2020
set(CMAKE_CXX_STANDARD 20)

# Preventing the executable from being in a nested folder on Windows
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR})

find_package(Vulkan REQUIRED)

add_subdirectory(vendor/glfw)
add_subdirectory(vendor/glm)

add_executable(${PROJECT_NAME}
  # stb_image
  ${CMAKE_SOURCE_DIR}/include/stb_image/stb_image.h
  ${CMAKE_SOURCE_DIR}/include/stb_image/stb_image_imp.cpp

  # tiny_obj_loader
  ${CMAKE_SOURCE_DIR}/include/tiny_obj_loader/tiny_obj_loader.h
  ${CMAKE_SOURCE_DIR}/include/tiny_obj_loader/tiny_obj_loader_imp.cpp
  
  # Project
  ${CMAKE_SOURCE_DIR}/include/application.hpp

  ${CMAKE_SOURCE_DIR}/src/main.cpp
  ${CMAKE_SOURCE_DIR}/src/application.cpp
)

# Resource subdirectories
add_subdirectory(resources/shaders)
add_subdirectory(resources/models)

add_dependencies(${PROJECT_NAME} Shaders Models)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  glfw
  glm
  Vulkan::Vulkan
)

target_link_libraries(${PROJECT_NAME}
  glfw
  glm
  Vulkan::Vulkan
)