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
set(IMGUI_DIR vendor/imgui)

add_executable(${PROJECT_NAME}
  #ImGui
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp

  # stb_image
  ${CMAKE_SOURCE_DIR}/include/stb_image/stb_image.h
  ${CMAKE_SOURCE_DIR}/include/stb_image/stb_image_imp.cpp

  # tiny_obj_loader
  ${CMAKE_SOURCE_DIR}/include/tiny_obj_loader/tiny_obj_loader.h
  ${CMAKE_SOURCE_DIR}/include/tiny_obj_loader/tiny_obj_loader_imp.cpp
  
  # Project
  ${CMAKE_SOURCE_DIR}/include/utilities.hpp
  ${CMAKE_SOURCE_DIR}/include/model.hpp
  ${CMAKE_SOURCE_DIR}/include/application.hpp

  ${CMAKE_SOURCE_DIR}/src/utilities.cpp
  ${CMAKE_SOURCE_DIR}/src/model.cpp
  ${CMAKE_SOURCE_DIR}/src/application.cpp

  ${CMAKE_SOURCE_DIR}/src/main.cpp
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
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  Vulkan::Vulkan
)

target_link_libraries(${PROJECT_NAME}
  glfw
  glm
  Vulkan::Vulkan
)