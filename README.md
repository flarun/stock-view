# Stock View

A lightweight, high-performance cross-platform real-time stock terminal built in C++ with OpenGL 3.2, GLFW, Dear ImGui, and ImPlot.

![Stock View Preview](preview/preview.png)

## Features

- **Real-Time WebSocket Streaming:** Low-latency live trade data streaming via `ixwebsocket`, replacing inefficient polling loops.
- **Dynamic Timeframes:** Adjustable historical candle resolutions ranging from 1-minute to Monthly views.
- **Secure Credentials:** OS-native encrypted vault integration via `keychain` for secure API key management.
- **Native File Dialogs:** Seamless OS-level file pickers for saving and loading workspace configurations via `nativefiledialog-extended`.
- **Flexible Docking Workspace:** Fullscreen ImGui DockBuilder layout supporting custom panel docking, resizing, and persistent window states.
- **Modern Typography & Theming:** High-resolution anti-aliased font rendering using `Roboto-Regular.ttf` paired with a multi-theme engine supporting Dark, Light, Classic, and a custom Nord palette.
- **Modular Indicators:** Extensible quantitative engine allowing users to stack, color-code, and customize technical indicators (like SMA) directly on charts.
- **Advanced Charting:** Interactive ImPlot graphs with pan, zoom, time-axis scaling, and multi-mode rendering (Line & Candlestick).
- **Live System Console:** Real-time observability into network requests and system events with automatic scrolling.

## Tech Stack

- **Core:** C++17, OpenGL 3.2, GLFW
- **UI & Plotting:** Dear ImGui (Docking Branch), ImPlot
- **Networking & Data:** libcurl, ixwebsocket, nlohmann_json, keychain, nativefiledialog-extended
- **Build System:** CMake, vcpkg, Ninja, FetchContent

## Building from Source

Prerequisites: Ensure **CMake**, **Ninja**, and **vcpkg** are installed on your system.

### Linux (Fedora / Ubuntu)

Install system dependencies (Fedora example):

`sudo dnf install gcc gcc-c++ make automake autoconf libtool pkgconf-pkg-config openssl-devel libcurl-devel libsecret-devel`

Configure and Build:

`cmake -B build -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE=$VCPKG_INSTALLATION_ROOT/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release`
`cmake --build build --config Release`

### macOS (Universal Binary)

`cmake -B build -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE=$VCPKG_INSTALLATION_ROOT/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`
`cmake --build build --config Release`

### Windows (Visual Studio / MSVC)

Open a Developer Command Prompt and run (adjust the path to where you installed vcpkg):

`cmake -B build -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release`
`cmake --build build --config Release`

### Creating Native Installers (CPack)

To bundle the application into a distribution package (`.deb` for Linux, `.dmg` for macOS, or an NSIS setup installer for Windows):

`cd build`
`cpack -C Release`

## Quick Start & Configuration

1. Launch the application (`./build/stock_view`).
2. Navigate to **File -> Settings** in the top menu bar to select your preferred theme or configure your Finnhub API Key.
3. Type a stock symbol (e.g., AAPL) into the Watchlist and click **Add Ticker** to begin live streaming data!
