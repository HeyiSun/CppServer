cmake_minimum_required(VERSION 2.8)
project(CppServer LANGUAGES CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLGAS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-used-function")

set(LIB_SRC
   CppServer/log.cpp
   CppServer/util.cpp
    )

add_library(CppServer SHARED ${LIB_SRC})
#add_library(CppServer_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES(CppServer_static PROPERTIES OUTPUT_NAME "CppServer")

add_executable(test tests/test.cpp)
add_dependencies(test CppServer)
target_link_libraries(test CppServer)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
