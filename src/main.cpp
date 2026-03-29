#define _CRT_SECURE_NO_WARNINGS
#include <Geode/Geode.hpp>
#include <Python.h>
#include "../include/PythonRuntime.hpp"
#include <fstream>
#include <sstream>

#ifdef GEODE_IS_MOBILE
    #include <Geode/utils/file.hpp>
#endif

using namespace geode::prelude;

$on_mod(Loaded) {
    if (!Py_IsInitialized()) {
        Py_Initialize();
        log::info("Python Initialized!");
        PythonRuntime::setupSandbox();
        PythonRuntime::initializeForMod(geode::Mod::get());
    }

    // Delay scanning other mods until they are all loaded
    Loader::get()->queueInMainThread([]() {
        for (auto const& mod : Loader::get()->getAllMods()) {
            if (mod->getID() != "chimed.python_runtime") {
                PythonRuntime::initializeForMod(mod);
            }
        }
    });
}

void PythonRuntime::initializeForMod(geode::Mod* mod) {
    if (!mod) return;

    auto modDir = mod->getConfigDir() / "python";
    if (!std::filesystem::exists(modDir)) {
        auto envFile = modDir / ".env";
        auto reqFile = modDir / "requirements.txt";
        auto mainScript = modDir / "main.py";
        if (!std::filesystem::exists(envFile) && !std::filesystem::exists(reqFile) && !std::filesystem::exists(mainScript)) {
            return;
        }
        std::filesystem::create_directories(modDir);
    }

    log::info("Initializing Python for mod: {}", mod->getID());

    #ifndef GEODE_IS_MOBILE
        ensureDependencies(mod);
    #else
        log::warn("Runtime dependency installation is not supported on mobile. Please bundle your dependencies in the python/site-packages folder.");
    #endif

    auto sitePackages = modDir / "site-packages";
    std::string pathSetup = "import sys; sys.path.append(r'" + modDir.string() + "'); sys.path.append(r'" + sitePackages.string() + "')";
    runString(pathSetup);

    auto mainScript = modDir / "main.py";
    if (std::filesystem::exists(mainScript)) {
        runFile(mainScript, mod);
    }
}

void PythonRuntime::ensureDependencies(geode::Mod* mod) {
    #ifdef GEODE_IS_MOBILE
        return;
    #else
    auto modDir = mod->getConfigDir() / "python";
    auto envFile = modDir / ".env";
    auto reqFile = modDir / "requirements.txt";
    auto sitePackages = modDir / "site-packages";

    if (!std::filesystem::exists(sitePackages)) {
        std::filesystem::create_directories(sitePackages);
    }

    std::vector<std::string> dependencies;
    if (std::filesystem::exists(envFile)) {
        std::ifstream file(envFile);
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("PYTHON_DEPS=") == 0) {
                std::string deps = line.substr(12);
                std::stringstream ss(deps);
                std::string dep;
                while (std::getline(ss, dep, ',')) {
                    dep.erase(0, dep.find_first_not_of(" "));
                    dep.erase(dep.find_last_not_of(" ") + 1);
                    if (!dep.empty()) dependencies.push_back(dep);
                }
            }
        }
    }

    if (std::filesystem::exists(reqFile)) {
        std::ifstream file(reqFile);
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line[0] != '#') {
                dependencies.push_back(line);
            }
        }
    }

    if (!dependencies.empty()) {
        log::info("Installing dependencies for mod {}: {}", mod->getID(), dependencies.size());
        std::string cmd = "python3 -m pip install --target=\"" + sitePackages.string() + "\" ";
        for (auto const& dep : dependencies) {
            cmd += dep + " ";
        }
        std::system(cmd.c_str());
    }
    #endif
}

void PythonRuntime::setupSandbox() {
    const char* sandboxScript = R"(
import sys

class RuntimePermissionError(Exception):
    pass

def audit_hook(event, args):
    restricted = ['os.system', 'os.spawn', 'subprocess.Popen', 'socket.connect', 'urllib.request.urlopen']
    if event in restricted:
        raise RuntimePermissionError(f"Access to {event} is restricted by Geode Python Runtime.")

sys.addaudithook(audit_hook)
)";
    PyRun_SimpleString(sandboxScript);
}

void PythonRuntime::runString(std::string const& code, geode::Mod* mod) {
    if (!Py_IsInitialized()) return;
    PyRun_SimpleString(code.c_str());
}

void PythonRuntime::runFile(std::filesystem::path const& path, geode::Mod* mod) {
    if (!Py_IsInitialized()) return;
    FILE* file = fopen(path.string().c_str(), "r");
    if (file) {
        PyRun_SimpleFile(file, path.string().c_str());
        fclose(file);
    }
}
