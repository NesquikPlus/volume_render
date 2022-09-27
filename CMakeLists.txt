cmake_minimum_required (VERSION 3.0)
cmake_policy(VERSION 3.0)

project(VolumeRender)

set(CMAKE_CXX_STANDARD 17) # this does nothing for MSVC, use target_compile_options below
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

link_directories(${CMAKE_SOURCE_DIR}/lib)

# find the required packages
find_package(GLM REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")
find_package(GLFW3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")

if(WIN32)
  set(LIBS glfw3 opengl32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(WIN32)

# first create relevant static libraries requried for other projects
add_library(STB_IMAGE "src/stb_image.cpp")
set(LIBS ${LIBS} STB_IMAGE)

add_library(GLAD "src/glad.c")
set(LIBS ${LIBS} GLAD)

file(GLOB SOURCE
  "src/*.h"
  "src/*.cpp"
)

set(NAME VolumeRender)
add_executable(${NAME} ${SOURCE})
target_link_libraries(${NAME} ${LIBS})


if(MSVC)
		target_compile_options(${NAME} PRIVATE /std:c++17 /MP)
    target_link_options(${NAME} PUBLIC /ignore:4099)
endif(MSVC)

if(WIN32)
        set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
        set_target_properties(${NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif(WIN32)

include_directories(${CMAKE_SOURCE_DIR}/includes)