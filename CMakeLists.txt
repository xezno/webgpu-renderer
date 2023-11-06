cmake_minimum_required(VERSION 3.0...3.25)
project(HackweekWebGPU VERSION 1.0.0 LANGUAGES CXX)

add_subdirectory(thirdparty/crosswindow)    # CrossWindow
add_subdirectory(thirdparty/dawn)           # webgpu

set(SOURCE_FILES
  main.cpp
)

xwin_add_executable(
  ${PROJECT_NAME}
  "${SOURCE_FILES}"
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "/WX")
else()
  target_compile_options(${PROJECT_NAME} PRIVATE "-Werror")
endif()

target_link_libraries(
  ${PROJECT_NAME}
  CrossWindow
  webgpu
)

target_copy_webgpu_binaries(${PROJECT_NAME})