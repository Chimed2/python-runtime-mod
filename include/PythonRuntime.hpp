#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <filesystem>

#ifdef GEODE_IS_WINDOWS
    #ifdef PYTHON_RUNTIME_EXPORT
        #define PYTHON_API __declspec(dllexport)
    #else
        #define PYTHON_API __declspec(dllimport)
    #endif
#else
    #define PYTHON_API [[gnu::visibility("default")]]
#endif

class PYTHON_API PythonRuntime {
public:
    /**
     * @brief Initialize the Python environment for a specific mod.
     */
    static void initializeForMod(geode::Mod* mod);

    /**
     * @brief Run a python string in the context of a mod.
     */
    static void runString(std::string const& code, geode::Mod* mod = nullptr);

    /**
     * @brief Run a python file in the context of a mod.
     */
    static void runFile(std::filesystem::path const& path, geode::Mod* mod = nullptr);

    /**
     * @brief Setup the security sandbox.
     */
    static void setupSandbox();

    /**
     * @brief Check and install dependencies for a mod.
     */
    static void ensureDependencies(geode::Mod* mod);
};
