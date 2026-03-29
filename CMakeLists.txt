cmake_minimum_required(VERSION 3.21)

# Set C++ standard to 20 for compatibility with Geode's {fmt}
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if (Python3_ROOT_DIR)
    set(Python3_FIND_STRATEGY LOCATION)
endif()

if (ANDROID)
    if (NOT DEFINED PREBUILT_PYTHON_PATH)
        if (Python3_ROOT_DIR)
            set(PREBUILT_PYTHON_PATH ${Python3_ROOT_DIR})
        else()
            message(WARNING "PREBUILT_PYTHON_PATH and Python3_ROOT_DIR not set for Android!")
        endif()
    endif()

    if (PREBUILT_PYTHON_PATH)
        add_library(Python3_Mobile INTERFACE)
        target_include_directories(Python3_Mobile INTERFACE ${PREBUILT_PYTHON_PATH}/include)
        
        find_library(PYTHON_LIB python3.11 PATHS ${PREBUILT_PYTHON_PATH}/lib NO_DEFAULT_PATH)
        if (NOT PYTHON_LIB)
            find_library(PYTHON_LIB python3 PATHS ${PREBUILT_PYTHON_PATH}/lib NO_DEFAULT_PATH)
        endif()
        target_link_libraries(Python3_Mobile INTERFACE ${PYTHON_LIB})
        
        set(PYTHON_TARGET Python3_Mobile)
    else()
        message(STATUS "Python Runtime will attempt to find Python despite no PREBUILT_PYTHON_PATH provided.")
        find_package(Python3 REQUIRED COMPONENTS Development.Embed)
        set(PYTHON_TARGET Python3::Python)
    endif()
else()
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Embed)
    set(PYTHON_TARGET Python3::Python)
endif()

project(PythonRuntime VERSION 1.0.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Use plain linking to match Geode's internal style
target_link_libraries(${PROJECT_NAME} ${PYTHON_TARGET})

if (NOT ANDROID)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Python3_INCLUDE_DIRS})
endif()

target_include_directories(${PROJECT_NAME} INTERFACE include)
target_compile_definitions(${PROJECT_NAME} PRIVATE PYTHON_RUNTIME_EXPORT)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME} VERSION 4.0.0)
