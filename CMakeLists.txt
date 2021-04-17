cmake_minimum_required(VERSION 2.8)
project(CppServer LANGUAGES CXX)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLGAS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-used-function -Wno-builtin-macro-redefined")

include_directories(.)

set(LIB_SRC
    CppServer/log.cpp
    CppServer/util.cpp
    CppServer/config.cpp
    CppServer/thread.cpp
    CppServer/fiber.cpp
    CppServer/scheduler.cpp
    CppServer/iomanager.cpp
    CppServer/timer.cpp
    CppServer/hook.cpp
    CppServer/fd_manager.cpp
    CppServer/address.cpp
    )

add_library(CppServer SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(CppServer)
#add_library(CppServer_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES(CppServer_static PROPERTIES OUTPUT_NAME "CppServer")

set(LIB_LIB
    CppServer
    dl
    pthread
    yaml-cpp
     )

add_executable(test tests/test.cpp)
add_dependencies(test CppServer)
force_redefine_file_macro_for_sources(test)
target_link_libraries(test ${LIB_LIB})

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config CppServer)
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config ${LIB_LIB})

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread CppServer)
force_redefine_file_macro_for_sources(test_thread)
target_link_libraries(test_thread ${LIB_LIB})

add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util CppServer)
force_redefine_file_macro_for_sources(test_util)
target_link_libraries(test_util ${LIB_LIB})

add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber CppServer)
force_redefine_file_macro_for_sources(test_fiber)
target_link_libraries(test_fiber ${LIB_LIB})

add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler CppServer)
force_redefine_file_macro_for_sources(test_scheduler)
target_link_libraries(test_scheduler ${LIB_LIB})

add_executable(test_iomanager tests/test_iomanager.cpp)
add_dependencies(test_iomanager CppServer)
force_redefine_file_macro_for_sources(test_iomanager)
target_link_libraries(test_iomanager ${LIB_LIB})

add_executable(test_hook tests/test_hook.cpp)
add_dependencies(test_hook CppServer)
force_redefine_file_macro_for_sources(test_hook)
target_link_libraries(test_hook ${LIB_LIB})

add_executable(test_address tests/test_address.cpp)
add_dependencies(test_address CppServer)
force_redefine_file_macro_for_sources(test_address)
target_link_libraries(test_address ${LIB_LIB})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
