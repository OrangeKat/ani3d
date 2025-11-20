cmake_minimum_required(VERSION 3.8)

# Relative path to the CGP library (defaults to the repo-local copy)
set(PATH_TO_CGP "${CMAKE_CURRENT_LIST_DIR}/cgp/library/" CACHE PATH "Relative path to CGP library location")

option(MACOS_GLFW_PRECOMPILED "Use precompiled library for GLFW on MacOS" OFF)

get_filename_component(ABS_PATH_TO_CGP ${PATH_TO_CGP} ABSOLUTE)
message(STATUS "The relative path to the library is set to ${PATH_TO_CGP}")
message(STATUS "The absolute path to the library is set to ${ABS_PATH_TO_CGP}")
if(NOT EXISTS ${ABS_PATH_TO_CGP})
   message(FATAL_ERROR "\nError: Could not import the CGP library using the relative path \"${PATH_TO_CGP}\".\n Please adjust this path in the CMakeLists.txt=>PATH_TO_CGP or via the cmake-gui\n Note that this relative path should point to the directory cgp/library/ ")
   return()
endif()

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CONFIGURATION_TYPES RelWithDebInfo)

set(src_files
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/application.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/environment.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/fire_scene.cpp
)

get_filename_component(executable_name ${CMAKE_CURRENT_LIST_DIR} NAME)
message(STATUS "Configure steps to build executable file [${executable_name}]")
project(${executable_name})

# Keep local headers (none yet, but mirrors template expectations)
include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/src)
include_directories(${ABS_PATH_TO_CGP})
include_directories(${ABS_PATH_TO_CGP}/third_party/src/imgui)

message(STATUS "Include CGP lib and external dependencies files from relative path")
include(${ABS_PATH_TO_CGP}/CMakeLists.txt)

add_definitions(-DSOLUTION)
add_definitions(-DCGP_OPENGL_3_3)

add_executable(${executable_name} ${src_files_cgp} ${src_files_third_party} ${src_files})
target_compile_definitions(${executable_name} PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)

if(UNIX)
   set(CMAKE_CXX_COMPILER g++)
   add_definitions(-g -O2 -std=c++14 -Wall -Wextra -Wfatal-errors -Wno-pragmas -Wno-unknown-warning-option)
   add_definitions(-Wno-sign-compare -Wno-type-limits)
endif()

if(MSVC)
   set_property( DIRECTORY PROPERTY VS_STARTUP_PROJECT  ${executable_name} )
   set_target_properties( ${executable_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}$<0:> )
   set_target_properties( ${executable_name} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} )
   
   if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
   else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
   endif()

    add_definitions(/MP /wd4244 /wd4127 /wd4267 /wd4706 /wd4458 /wd4996 /wd26495 /openmp)
    source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${src_files})
endif()

target_link_libraries(${executable_name} ${GLFW_LIBRARIES})
if(UNIX)
   target_link_libraries(${executable_name} dl)
endif()