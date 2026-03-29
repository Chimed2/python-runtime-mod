cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if (Python3_ROOT_DIR)
    set(Python3_FIND_STRATEGY LOCATION)
endif()

if (ANDROID OR IOS)
    # Android needs prebuilt Python - we'll keep this as an option
    if (NOT DEFINED PREBUILT_PYTHON_PATH)
        message(STATUS "PREBUILT_PYTHON_PATH not set, attempting to use Python3_ROOT_DIR: ${Python3_ROOT_DIR}")
        set(PREBUILT_PYTHON_PATH ${Python3_ROOT_DIR})
    endif()

    add_library(Python3_Mobile INTERFACE)
    target_include_directories(Python3_Mobile INTERFACE ${PREBUILT_PYTHON_PATH}/include)
    # This might need adjustments based on the exact structure of the Android Python lib
    target_link_libraries(Python3_Mobile INTERFACE ${PREBUILT_PYTHON_PATH}/lib/libpython3.so)
    
    set(PYTHON_TARGET Python3_Mobile)
else()
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Embed)
    set(PYTHON_TARGET Python3::Python)
endif()

project(PythonRuntime VERSION 1.0.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE ${PYTHON_TARGET})
if (NOT ANDROID AND NOT IOS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Python3_INCLUDE_DIRS})
endif()
target_include_directories(${PROJECT_NAME} INTERFACE include)
target_compile_definitions(${PROJECT_NAME} PRIVATE PYTHON_RUNTIME_EXPORT)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})
