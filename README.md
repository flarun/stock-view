# Stock View

A lightweight, high-performance real-time stock terminal built natively for Linux in C++ with OpenGL3, GLFW, Dear ImGui, and ImPlot.

![Stock View Preview](preview/preview.png)

## Features

- Dual-Fetch Architecture: Automatically backfills historical data (7-day hourly candles) upon adding a ticker, then seamlessly transitions to real-time live polling.
- Resilient Networking: Background C++ worker thread utilizes a Token Bucket rate limiter, graceful API fallbacks, and self-healing polling loops to prevent rate limits or silent crashes.
- Modular Technical Indicators: An extensible quantitative engine allowing users to stack, color-code, and customize indicators (like Simple Moving Averages) directly on the charts.
- Advanced Charting: Interactive ImPlot graphs with pan, zoom, real-world time-axis scaling, and multiple rendering strategies (Line & Candlestick).
- Live System Console: Real-time X-ray observability into network requests, API responses, and system events, now featuring automatic scrolling to keep the newest logs instantly in view.
- Secure & Persistent: API keys and user workspace configurations are securely managed at runtime and serialized to a local JSON file (settings.json).

## Tech Stack

- Core: C++17, OpenGL3, GLFW
- UI & Plotting: Dear ImGui (Docking Branch), ImPlot
- Networking & Data: System libcurl, nlohmann_json
- Build System: CMake, vcpkg

## Prerequisites (Linux / Fedora)

This project uses vcpkg for UI dependencies but relies on native system packages for networking and compilation. Before building, install the required development tools:

sudo dnf install gcc gcc-c++ make automake autoconf libtool autoconf-archive perl-FindBin perl-Thread-Queue openssl-devel libcurl-devel pkgconf-pkg-config

## Quick Start

### 1. Build the Application

Ensure you have CMake and vcpkg installed, then run (replace the toolchain path with your local vcpkg path):

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/home/youruser/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release

### 2. Configure Your API Key

For security, API keys are no longer hardcoded into the source code.

1. Launch the application from your terminal: ./build/stock_view
2. Navigate to File -> Settings -> API in the top menu bar.
3. Paste your free Finnhub.io API key and click Close.
4. Type a stock symbol (e.g., AAPL) into the Watchlist and click Add Ticker to begin streaming data!
