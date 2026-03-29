cmake_minimum_required(VERSION 3.21)

# Standard C++20 for compatibility with modern Geode and {fmt}
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# --- Python Discovery ---
# We need to find the Python development headers and libraries. 
# On mobile (Android/iOS), we often rely on prebuilt versions.

if (ANDROID)
    message(STATUS "Configuring Python for Android...")
    
    # I literally spent all of yesterday fighting with Android's find_library.
    # Why is it so inconsistent with the NDK? If this pathing logic 
    # ever breaks again, I'm quitting.
    if (NOT DEFINED PREBUILT_PYTHON_PATH AND Python3_ROOT_DIR)
        set(PREBUILT_PYTHON_PATH ${Python3_ROOT_DIR})
    endif()

    if (PREBUILT_PYTHON_PATH)
        message(STATUS "Using prebuilt Python from: ${PREBUILT_PYTHON_PATH}")
        add_library(Python3_Mobile INTERFACE)
        target_include_directories(Python3_Mobile INTERFACE "${PREBUILT_PYTHON_PATH}/include")
        
        # We try to find the specific library version
        find_library(PYTHON_LIB NAMES python3.11 python3 PATHS "${PREBUILT_PYTHON_PATH}/lib" NO_DEFAULT_PATH)
        if (NOT PYTHON_LIB)
            message(FATAL_ERROR "Could not find Python library in ${PREBUILT_PYTHON_PATH}/lib")
        endif()
        
        target_link_libraries(Python3_Mobile INTERFACE ${PYTHON_LIB})
        set(PYTHON_TARGET Python3_Mobile)
    else()
        message(WARNING "Neither PREBUILT_PYTHON_PATH nor Python3_ROOT_DIR is set for Android. Build might fail.")
        find_package(Python3 REQUIRED COMPONENTS Development.Embed)
        set(PYTHON_TARGET Python3::Python)
    endif()
else()
    # On desktop platforms, standard discovery is usually fine.
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Embed)
    set(PYTHON_TARGET Python3::Python)
    message(STATUS "Found Python: ${Python3_VERSION} at ${Python3_EXECUTABLE}")
endif()

# --- Project Setup ---
project(PythonRuntime VERSION 1.0.0)

# Automatically pick up any .cpp files in the src/ directory
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

# --- Linking & Includes ---
target_link_libraries(${PROJECT_NAME} ${PYTHON_TARGET})

# Ensure we have access to our own public headers
target_include_directories(${PROJECT_NAME} INTERFACE include)

# This define is used in PythonRuntime.hpp to handle DLL exports
target_compile_definitions(${PROJECT_NAME} PRIVATE PYTHON_RUNTIME_EXPORT)

# Desktop-specific includes if find_package was used
if (NOT ANDROID AND Python3_INCLUDE_DIRS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Python3_INCLUDE_DIRS})
endif()

# --- Geode Integration ---
# Geode is required for this mod to function.
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "GEODE_SDK environment variable is not set! Please point it to your Geode SDK directory.")
endif()

message(STATUS "Geode SDK found at: $ENV{GEODE_SDK}")
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Finalize the Geode mod setup
setup_geode_mod(${PROJECT_NAME} VERSION 4.0.0)
