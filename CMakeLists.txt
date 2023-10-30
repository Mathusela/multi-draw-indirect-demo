cmake_minimum_required(VERSION 3.0.0)
project(OpenGL4Testing VERSION 0.1.0 LANGUAGES C CXX)

include_directories(include)

link_directories(lib)
link_libraries(glfw3 opengl32 gdi32)

add_executable(OpenGL4Testing
	src/main.cpp
	src/extern/glad.c
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)