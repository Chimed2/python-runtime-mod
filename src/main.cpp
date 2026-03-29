// idk why i decided to put this here but i use arch btw
#include <Geode/Geode.hpp>
#include <Python.h>
#include "../include/PythonRuntime.hpp"
#include <fstream>
#include <sstream>

using namespace geode::prelude;

/**
 * Entry point for the Python Runtime mod.
 * We initialize Python here and then scan all currently loaded mods
 * to see if any of them have Python scripts that need to be run.
 */
$on_mod(Loaded) {
    if (!Py_IsInitialized()) {
        log::info("Initializing Python interpreter...");
        Py_Initialize();
        
        // We set up a basic sandbox to prevent malicious mods from doing too much damage.
        // It's not perfect, but it's better than nothing.
        PythonRuntime::setupSandbox();

        // Register this mod itself (if we ever add internal scripts)
        PythonRuntime::initializeForMod(Mod::get());
    }

    // I spent way too long (like 5 hours) wondering why some mods weren't being 
    // detected, only to realize I needed to queue this to the main thread 
    // so the Loader actually has time to finish its internal mod list. 
    // I can't believe this was the fix.
    Loader::get()->queueInMainThread([]() {
        for (auto const& mod : Loader::get()->getAllMods()) {
            if (mod->getID() != "chimed.python_runtime") {
                PythonRuntime::initializeForMod(mod);
            }
        }
    });
}

void PythonRuntime::initializeForMod(Mod* mod) {
    if (!mod) return;

    // We look for a 'python' folder inside the mod's configuration directory.
    auto modDir = mod->getConfigDir() / "python";
    
    // If the directory doesn't exist, we check if we should create it.
    // We only create it if there's actually something to run or install.
    if (!std::filesystem::exists(modDir)) {
        bool hasContent = std::filesystem::exists(modDir / ".env") || 
                          std::filesystem::exists(modDir / "requirements.txt") || 
                          std::filesystem::exists(modDir / "main.py");
        
        if (!hasContent) return;

        std::error_code ec;
        std::filesystem::create_directories(modDir, ec);
        if (ec) {
            log::error("Failed to create python directory for mod {}: {}", mod->getID(), ec.message());
            return;
        }
    }

    log::info("Found Python content for mod: {}", mod->getID());

    // On non-mobile platforms, we try to install missing dependencies.
#ifndef GEODE_IS_MOBILE
    ensureDependencies(mod);
#else
    log::warn("Automatic dependency installation is skipped on mobile. Make sure to bundle 'site-packages'.");
#endif

    // Setup sys.path so the mod can import its own scripts and dependencies.
    auto sitePackages = modDir / "site-packages";
    std::string pathSetup = "import sys; sys.path.extend([r'" + modDir.string() + "', r'" + sitePackages.string() + "'])";
    runString(pathSetup);

    // If there's a main.py, we run it as the entry point.
    auto mainScript = modDir / "main.py";
    if (std::filesystem::exists(mainScript)) {
        log::info("Executing main.py for mod: {}", mod->getID());
        runFile(mainScript, mod);
    }
}

void PythonRuntime::ensureDependencies(Mod* mod) {
#ifdef GEODE_IS_MOBILE
    return; // Pip isn't exactly easy to run on mobile environments.
#else
    auto modDir = mod->getConfigDir() / "python";
    auto envFile = modDir / ".env";
    auto reqFile = modDir / "requirements.txt";
    auto sitePackages = modDir / "site-packages";

    std::vector<std::string> dependencies;

    // 1. Parse .env file (older style or simple config)
    // I really hate manual string parsing, but I didn't want to add a 
    // whole library just to read a few lines. This specific block took 
    // like 3 hours of trial and error with different line endings.
    if (std::filesystem::exists(envFile)) {
        std::ifstream file(envFile);
        std::string line;
        while (std::getline(file, line)) {
            if (line.rfind("PYTHON_DEPS=", 0) == 0) {
                std::string deps = line.substr(12);
                std::stringstream ss(deps);
                std::string dep;
                while (std::getline(ss, dep, ',')) {
                    // Trim whitespace
                    dep.erase(0, dep.find_first_not_of(" \t\r\n"));
                    dep.erase(dep.find_last_not_of(" \t\r\n") + 1);
                    if (!dep.empty()) dependencies.push_back(dep);
                }
            }
        }
    }

    // 2. Parse requirements.txt (standard python style)
    if (std::filesystem::exists(reqFile)) {
        std::ifstream file(reqFile);
        std::string line;
        while (std::getline(file, line)) {
            // Basic trimming and skipping comments
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (!line.empty() && line[0] != '#') {
                dependencies.push_back(line);
            }
        }
    }

    if (dependencies.empty()) return;

    // We only run pip if site-packages doesn't exist yet. 
    // Optimization: A real dev might want a 'force reinstall' flag, but for now this is fine.
    if (!std::filesystem::exists(sitePackages)) {
        std::filesystem::create_directories(sitePackages);
        
        log::info("Installing {} dependencies for {}...", dependencies.size(), mod->getID());
        
        // We use 'python' as a fallback if 'python3' isn't in the path (common on Windows)
        std::string pythonCmd = "python3";
        if (std::system("python3 --version > nul 2>&1") != 0) {
            pythonCmd = "python";
        }

        std::string cmd = pythonCmd + " -m pip install --target=\"" + sitePackages.string() + "\" ";
        for (auto const& dep : dependencies) {
            cmd += "\"" + dep + "\" ";
        }
        
        int result = std::system(cmd.c_str());
        if (result != 0) {
            log::error("Failed to install dependencies for {}. Pip returned {}", mod->getID(), result);
        }
    }
#endif
}

void PythonRuntime::setupSandbox() {
    // A simple audit hook to catch common "dangerous" operations.
    // Real security in Python is hard, but this blocks the most obvious stuff.
    const char* sandboxScript = R"(
import sys

class RestrictedAccessError(Exception):
    pass

def _geode_audit_hook(event, args):
    # These are some common things we might want to restrict in a modding environment.
    danger_zone = {
        'os.system', 'os.spawn', 'subprocess.Popen', 
        'socket.connect', 'urllib.request.urlopen'
    }
    if event in danger_zone:
        raise RestrictedAccessError(f"Access to '{event}' is blocked by the Geode Python Sandbox.")

sys.addaudithook(_geode_audit_hook)
log_info = lambda msg: print(f"[Python] {msg}")
)";
    PyRun_SimpleString(sandboxScript);
}

void PythonRuntime::runString(std::string const& code, Mod* mod) {
    if (!Py_IsInitialized()) return;
    
    // We wrap the execution to catch potential errors if needed, 
    // but PyRun_SimpleString is usually enough for simple stuff.
    PyRun_SimpleString(code.c_str());
}

void PythonRuntime::runFile(std::filesystem::path const& path, Mod* mod) {
    if (!Py_IsInitialized()) return;

    // Using a scope block for the file pointer just in case.
    if (auto* fp = fopen(path.string().c_str(), "r")) {
        PyRun_SimpleFile(fp, path.string().c_str());
        fclose(fp);
    } else {
        log::error("Could not open Python file: {}", path.string());
    }
}
