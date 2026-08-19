# Stock View

A lightweight real-time stock terminal built in C++ with DirectX 11, Dear ImGui, and ImPlot.

![Stock View Preview](preview/preview.png)

## Features

- Real-time plotting with interactive pan and zoom
- Multi-panel dockable workspace (Watchlist, Charts, Details, Console)
- Background data worker with Token Bucket rate limiting
- Save/load session history via JSON

## Tech Stack

- **Core:** C++17, DirectX 11, Win32
- **UI & Plotting:** Dear ImGui (Docking), ImPlot
- **Networking & Data:** libcurl, nlohmann_json
- **Build System:** CMake, vcpkg

## Quick Start

1. Add your Finnhub API key to `src/config.h`.
2. Build via CMake:

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```
