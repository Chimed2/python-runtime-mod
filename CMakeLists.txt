cmake_minimum_required(VERSION 3.21)

# Set C++ standard to 20 for compatibility with Geode's {fmt}
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
    if (IOS)
        set(CMAKE_OSX_ARCHITECTURES "arm64")
    else()
        # On GitHub Actions macOS runners, Python is often single-arch.
        # We try to detect the architecture of the Python library to avoid linking errors.
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
        if (Python3_ROOT_DIR)
            message(STATUS "Mac: Python3_ROOT_DIR is set, checking architecture...")
            # If we're on a Silicon Mac runner (arm64), and Python is arm64, 
            # building for x86_64 will fail unless we have a universal Python.
            # For simplicity, if we are in CI, we'll just target the current arch.
            if (DEFINED ENV{GITHUB_ACTIONS})
                set(CMAKE_OSX_ARCHITECTURES "arm64")
                message(STATUS "CI detected: Targeting arm64 only for macOS build.")
            endif()
        endif()
    endif()
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if (Python3_ROOT_DIR)
    set(Python3_FIND_STRATEGY LOCATION)
endif()

if (ANDROID OR IOS)
    if (NOT DEFINED PREBUILT_PYTHON_PATH)
        if (Python3_ROOT_DIR)
            set(PREBUILT_PYTHON_PATH ${Python3_ROOT_DIR})
        else()
            message(WARNING "PREBUILT_PYTHON_PATH and Python3_ROOT_DIR not set for mobile platform!")
        endif()
    endif()

    if (PREBUILT_PYTHON_PATH)
        add_library(Python3_Mobile INTERFACE)
        target_include_directories(Python3_Mobile INTERFACE ${PREBUILT_PYTHON_PATH}/include)
        
        if (ANDROID)
            # Use a slightly more flexible search for the .so
            find_library(PYTHON_LIB python3.11 PATHS ${PREBUILT_PYTHON_PATH}/lib NO_DEFAULT_PATH)
            if (NOT PYTHON_LIB)
                find_library(PYTHON_LIB python3 PATHS ${PREBUILT_PYTHON_PATH}/lib NO_DEFAULT_PATH)
            endif()
            target_link_libraries(Python3_Mobile INTERFACE ${PYTHON_LIB})
        elseif (IOS)
            target_link_libraries(Python3_Mobile INTERFACE ${PREBUILT_PYTHON_PATH}/lib/libpython3.a)
        endif()
        set(PYTHON_TARGET Python3_Mobile)
    else()
        message(FATAL_ERROR "Python Runtime requires a prebuilt Python for mobile platforms. Please provide PREBUILT_PYTHON_PATH.")
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

if (NOT ANDROID AND NOT IOS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Python3_INCLUDE_DIRS})
endif()

target_include_directories(${PROJECT_NAME} INTERFACE include)
target_compile_definitions(${PROJECT_NAME} PRIVATE PYTHON_RUNTIME_EXPORT)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME} VERSION 4.0.0)
