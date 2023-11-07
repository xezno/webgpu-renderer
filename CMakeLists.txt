cmake_minimum_required(VERSION 3.0...3.25)
project(HackweekWebGPU VERSION 1.0.0 LANGUAGES CXX)

add_subdirectory(thirdparty/glfw)                   # glfw
add_subdirectory(thirdparty/dawn)                   # webgpu
add_subdirectory(thirdparty/glfw3webgpu)            # glfw3webgpu

# Glob src/
file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.h)

add_executable(
  ${PROJECT_NAME}
  "${SOURCE_FILES}"
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)

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
)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1")
target_copy_webgpu_binaries(${PROJECT_NAME})