# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

Godot uses **SCons** as its build system. The main branch is `4.3`.

### Building the Editor

```bash
# Basic editor build
scons platform=macos target=editor

# Development build with tests and extra warnings
scons platform=macos target=editor dev_mode=yes

# Production editor build (optimized)
scons platform=macos target=editor production=yes

# Build with tests included
scons platform=macos target=editor tests=yes
```

Platforms: `macos`, `linuxbsd`, `windows`, `android`, `ios`, `web`

Common build options:
- `tests=yes` - Include unit tests
- `dev_mode=yes` - Alias for `verbose=yes warnings=extra werror=yes tests=yes`
- `production=yes` - Production build settings
- `optimize=speed|size|debug|none` - Optimization level
- `arch=x86_64|arm64|universal` - Architecture (macOS)
- `-j8` - Parallel builds (auto-detected by default)

### Running Tests

```bash
# Build with tests
scons platform=macos target=editor tests=yes

# Run all unit tests
./bin/godot.macos.editor.universal --test

# Run with color output
./bin/godot.macos.editor.universal --test --force-colors
```

### Linting and Formatting

The project uses pre-commit hooks:

```bash
# One-time setup
pip3 install pre-commit
pre-commit install

# Run all checks
pre-commit run --all-files

# Run specific checks
pre-commit run clang-format --all-files  # C/C++ formatting
pre-commit run ruff --all-files          # Python linting
pre-commit run ruff-format --all-files   # Python formatting
```

### Cleaning Build

```bash
scons --clean platform=macos target=editor
```

## Architecture Overview

### Core Components

- **`core/`** - Foundation layer:
  - `core/object/` - Base Object class, ClassDB (reflection), method binding, signals
  - `core/variant/` - Dynamic type system supporting 40+ types
  - `core/io/` - File access, resource loading/saving, networking
  - `core/math/` - Vector math, transforms, geometry utilities
  - `core/os/` - Threading, memory, time, mutexes
  - `core/extension/` - GDExtension C/C++ plugin API

- **`servers/`** - Singleton servers providing core functionality:
  - `RenderingServer` - All rendering operations (2D/3D, materials, shaders)
  - `PhysicsServer2D/3D` - Physics simulation and collision
  - `AudioServer` - Audio mixing and effects
  - `NavigationServer2D/3D` - Pathfinding
  - `DisplayServer` - Window management and OS integration

- **`scene/`** - Scene graph and node system:
  - `scene/main/` - Base Node class, scene tree, viewports
  - `scene/2d/` - 2D nodes (sprites, canvas items, physics bodies)
  - `scene/3d/` - 3D nodes (meshes, lights, physics bodies)
  - `scene/gui/` - UI controls
  - `scene/animation/` - Animation system

- **`drivers/`** - Platform-specific low-level drivers (Vulkan, D3D12, GLES3, audio)

- **`platform/`** - Platform implementations (android, ios, linuxbsd, macos, windows, web)
  - Each platform directory contains `detect.py` for build configuration
  - Platform-specific OS integration and display servers

- **`modules/`** - Optional modular functionality
  - GDScript, C#, format importers, features like CSG, navigation
  - Each module has `config.py` and `register_types.cpp`

- **`editor/`** - Editor-specific code (only compiled with `TOOLS_ENABLED`)

### Build System

- **SCons-based**: `SConstruct` is the main build script
- **SCsub files**: Scattered throughout directories defining local build rules
- **Module system**: Modules declare dependencies and initialization levels (CORE ’ SERVERS ’ SCENE ’ EDITOR)

### Key Patterns

- **Server Pattern**: Low-level functionality via singleton servers; can be multi-threaded with `_wrap_mt` variants
- **Object & ClassDB**: All classes inherit from Object; ClassDB provides reflection and method binding
- **Resource ID (RID)**: Lightweight handles to server-managed resources
- **Node Pattern**: Scene graph with parent-child hierarchy; lifecycle: `_enter_tree()` ’ `_ready()` ’ `_process()` ’ `_exit_tree()`
- **Variant System**: Dynamic type for scripting interop and property system

### GDExtension

- C/C++ plugin system with stable ABI (`core/extension/gdextension_interface.h`)
- Extensions register via C API and can be reloaded at runtime in editor
- Compatible across Godot versions with same API hash

### Naming Conventions

- Classes: PascalCase
- Files: snake_case matching class name
- Private members: prefix with `_`
- Header guards: `#ifndef FILENAME_H`

### Compatibility

- `.compat.inc` files provide backward compatibility shims
- API changes tracked in `misc/extension_api_validation/`

## Important Files

- `SConstruct` - Main build configuration
- `pyproject.toml` - Python tools configuration (ruff, mypy)
- `.clang-format` - C/C++ code formatting rules
- `.pre-commit-config.yaml` - Pre-commit hook configuration
- `doc/classes/*.xml` - API documentation (XML format)
