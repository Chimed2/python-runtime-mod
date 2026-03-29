#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <filesystem>

/**
 * DLL Export/Import macros for Windows.
 * This allows other mods to potentially use the PythonRuntime class if they link against it.
 */
#ifdef GEODE_IS_WINDOWS
    #ifdef PYTHON_RUNTIME_EXPORT
        #define PYTHON_API __declspec(dllexport)
    #else
        #define PYTHON_API __declspec(dllimport)
    #endif
#else
    #define PYTHON_API [[gnu::visibility("default")]]
#endif

/**
 * The main interface for interacting with the Python environment within Geode.
 * This class handles initialization, dependency management, and script execution.
 */
class PYTHON_API PythonRuntime {
public:
    /**
     * Scans a mod for Python-related content (main.py, requirements.txt, etc.)
     * and sets up the environment for that specific mod.
     * @param mod The mod to initialize.
     */
    static void initializeForMod(geode::Mod* mod);

    /**
     * Executes a raw string of Python code.
     * @param code The Python source code to run.
     * @param mod Optional mod context (currently unused but reserved for future scoped execution).
     */
    static void runString(std::string const& code, geode::Mod* mod = nullptr);

    /**
     * Loads and executes a Python script from the filesystem.
     * @param path Path to the .py file.
     * @param mod Optional mod context.
     */
    static void runFile(std::filesystem::path const& path, geode::Mod* mod = nullptr);

    /**
     * Configures the Python audit hooks and basic security restrictions.
     * This is called once during the initial startup of the runtime.
     */
    static void setupSandbox();

    /**
     * Checks for dependency files (.env or requirements.txt) and attempts
     * to install them into the mod's 'site-packages' directory using pip.
     * Note: This is a no-op on mobile platforms.
     * @param mod The mod to check dependencies for.
     */
    static void ensureDependencies(geode::Mod* mod);
};
