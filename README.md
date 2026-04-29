# D3D11UIFramework
Windows D3D11-based UI rendering framework DLL for reusable desktop controls and render-layer integration

# Info
Modular C++ UI rendering framework for Windows built on Direct3D 11, Direct2D, and DirectWrite.
Provides reusable UI controls, layout panels, SVG icon rendering, animated visual states, event dispatching, and render-layer based integration for external engines or desktop applications.

This project is designed as a DLL-based UI component and integrates shared sibling modules from Core, D3D11Engine, and D3D11EngineInterface.

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
- Modular integration with shared Core, D3D11Engine, and D3D11EngineInterface projects

# Dependencies
- Core
- D3D11Engine
- D3D11EngineInterface
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
- `D3D11UIFramework.sln` : Visual Studio solution

# Repository Layout
This project expects D3D11UIFramework, Core, D3D11Engine, and D3D11EngineInterface to be placed under the same parent directory.

Example:
```text
Module/
+-- Core/
+-- D3D11Engine/
+-- D3D11EngineInterface/
+-- D3D11UIFramework/
```

The Visual Studio solution references the shared projects and interface headers through sibling paths:
- `../Core/Core/Core.vcxproj`
- `../D3D11Engine/D3D11Engine/D3D11Engine.vcxproj`
- `../D3D11EngineInterface/`

# Notes
- The shared Core, D3D11Engine, and D3D11EngineInterface modules are managed as sibling repositories/projects.
- Open D3D11UIFramework.sln with Visual Studio 2022.
- Build the x64 configuration to produce the D3D11UIFramework DLL.
- The main target is a DLL for reusable D3D11-based UI rendering.
- The framework currently includes buttons, labels, panels, split bars, SVG icon resources, and event dispatch support.
- The project builds together with shared `Core` and `D3D11Engine` module outputs through the solution.
