cmake_minimum_required(VERSION 3.0...3.25)
project(HackweekWebGPU VERSION 1.0.0 LANGUAGES CXX)

set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)

add_subdirectory(thirdparty/glfw)                   # glfw
add_subdirectory(thirdparty/dawn)                   # webgpu
add_subdirectory(thirdparty/glfw3webgpu)            # glfw3webgpu
add_subdirectory(thirdparty/tinygltf)               # tinygltf

# Glob src/
file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.h)

add_executable(
  ${PROJECT_NAME}
  "${SOURCE_FILES}"
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "/WX")
else()
  target_compile_options(${PROJECT_NAME} PRIVATE "-Werror")
endif()

target_link_libraries(
  ${PROJECT_NAME}
  glfw
  webgpu
  glfw3webgpu
  tinygltf
)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1")
target_copy_webgpu_binaries(${PROJECT_NAME})