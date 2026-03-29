# Python Runtime for Geode

A Geode mod that provides a Python 3 environment for other mods to use. It allows developers to write mod logic in Python, bundle dependencies, and run scripts in a sandboxed environment.

## 🚀 Features

-   **Automatic Script Discovery:** Each mod can have its own `python/` folder.
-   **Dependency Management:** Install modules via `.env` or `requirements.txt`.
-   **Security Sandboxing:** Built-in audit hooks to prevent dangerous operations.
-   **C++ API:** Execute Python strings or files directly from C++ mods.

## 📦 How to use for Mod Developers

### 1. Add Dependency
Add `chimed.python_runtime` to your `mod.json`:

```json
"dependencies": [
    {
        "id": "chimed.python_runtime",
        "version": "v1.0.0",
        "importance": "required"
    }
]
```

### 2. Create Python Directory
In your mod's Geode configuration directory, create a `python/` folder:
`.../geode/config/(your.mod.id)/python/`

### 3. Add your Script
Place a `main.py` inside that folder. It will be executed automatically when the mod is loaded.

### 4. (Optional) Manage Dependencies
You can install external Python libraries in two ways:
-   **`.env` file:** Add `PYTHON_DEPS=requests,numpy` to a `.env` file in your `python/` folder.
-   **`requirements.txt`:** Add standard pip requirements to this file in your `python/` folder.

Libraries will be installed into a local `site-packages` folder inside your mod's python directory.

## 🛠️ C++ API

If you want to run Python code from your C++ hooks:

```cpp
#include <Geode/Geode.hpp>
#include <PythonRuntime.hpp>

// Run a simple string
PythonRuntime::runString("print('Hello from Python!')");

// Run a specific file
PythonRuntime::runFile("path/to/script.py");
```

## 🔒 Security Sandbox

To protect users, the following operations are restricted by default:
-   Spawning shell processes (`os.system`, `subprocess`, etc.)
-   Unauthorized network connections (`socket.connect`)
-   Accessing files outside of Geometry Dash and Mod directories (Planned)

## 🏗️ Building

The mod is built using GitHub Actions. It supports:
-   **Windows**
-   **macOS** (Apple Silicon native)
-   **Android** (Work in progress)

## 📄 License

This mod is released under the MIT License.
