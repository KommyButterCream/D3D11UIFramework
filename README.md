# D3D11UIFramework
Windows D3D11-based UI rendering framework DLL for reusable desktop controls and render-layer integration

# Info
Modular C++ UI rendering framework for Windows built on Direct3D 11, Direct2D, and DirectWrite.
Provides reusable UI controls, layout panels, SVG icon rendering, animated visual states, event dispatching, and render-layer based integration for external engines or desktop applications.

This project is designed as a DLL-based UI component and integrates shared modules from `Core`, `D3D11Engine`, and `D3D11EngineInterface`.

# Features
- Direct3D 11 / Direct2D-based UI rendering
- Reusable UI control hierarchy with shared base layer support
- Button controls including context menu button variants
- Label controls including icon label rendering
- Panel-based layout containers with vertical and horizontal arrangement
- Split bar rendering support
- SVG icon resource loading and rendering
- Animated hover / pressed / disabled state transitions
- Callback-based UI command dispatching
- Modular shared library integration through submodules

# Dependencies
- [Core](./Modules/Core) as a submodule
- [D3D11Engine](./Modules/D3D11Engine) as a submodule
- [D3D11EngineInterface](./Modules/D3D11EngineInterface) as a submodule
- Windows Direct3D 11 / Direct2D / DirectWrite
- C++20
- MSVC (Visual Studio 2022)

# Build Environment
- C++20
- MSVC (Visual Studio 2022)
- Windows 10/11 x64

# Project Structure
- `D3D11UIFramework/` : main DLL project sources and headers
- `D3D11UIFramework/Base/` : common UI element base classes and shared style/state logic
- `D3D11UIFramework/Button/` : button controls and context menu button implementation
- `D3D11UIFramework/Label/` : text label and icon label controls
- `D3D11UIFramework/Panel/` : panel containers, layout logic, and status/context panel variants
- `D3D11UIFramework/Splitbar/` : split bar rendering component
- `D3D11UIFramework/Resource/` : SVG resources, icon helpers, color utilities, and shared UI resource helpers
- `D3D11UIFramework/Event/` : UI event dispatcher and event result structures
- `Modules/Core/` : shared utility and geometry/base infrastructure
- `Modules/D3D11Engine/` : D3D11 render engine and shared rendering utilities
- `Modules/D3D11EngineInterface/` : shared rendering interface definitions
- `D3D11UIFramework.sln` : Visual Studio solution

# Notes
- This repository uses multiple submodules, including nested submodules inside shared modules.
- Make sure submodules are initialized recursively before building.
- The main target is a DLL for reusable D3D11-based UI rendering.
- The framework currently includes buttons, labels, panels, split bars, SVG icon resources, and event dispatch support.
- The project builds together with shared `Core` and `D3D11Engine` module outputs through the solution.

# Clone
- Clone with submodules:
```bash
git clone --recurse-submodules https://github.com/KommyButterCream/D3D11UIFramework.git
```
- If already cloned without submodules:
```bash
git submodule update --init --recursive
```
