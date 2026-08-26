# Stock View

A lightweight, high-performance real-time stock terminal built in C++ with DirectX 11, Dear ImGui, and ImPlot.

![Stock View Preview](preview/preview.png)

## Features

- **Dual-Fetch Architecture:** Automatically backfills historical data (7-day hourly candles) upon adding a ticker, then seamlessly transitions to real-time live polling.
- **Resilient Networking:** Background C++ worker thread utilizes a Token Bucket rate limiter, graceful API fallbacks, and self-healing polling loops to prevent rate limits or silent crashes.
- **Modular Technical Indicators:** An extensible quantitative engine allowing users to stack, color-code, and customize indicators (like Simple Moving Averages) directly on the charts.
- **Advanced Charting:** Interactive ImPlot graphs with pan, zoom, real-world time-axis scaling, and multiple rendering strategies (Line & Candlestick).
- **Live System Console:** Real-time X-ray observability into network requests, API responses, and system events.
- **Secure & Persistent:** API keys and user workspace configurations are securely managed at runtime and serialized to a local JSON file (`settings.json`).

## Tech Stack

- **Core:** C++17, DirectX 11, Win32
- **UI & Plotting:** Dear ImGui (Docking Branch), ImPlot
- **Networking & Data:** libcurl, nlohmann_json
- **Build System:** CMake, vcpkg

## Quick Start

### 1. Build the Application

Ensure you have CMake and `vcpkg` installed, then run:

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release

### 2. Configure Your API Key

For security, API keys are no longer hardcoded into the source code.

1. Launch `stock_view.exe`.
2. Navigate to **File -> Settings -> API** in the top menu bar.
3. Paste your free [Finnhub.io](https://finnhub.io/) API key and click Close.
4. Type a stock symbol (e.g., `AAPL`) into the Watchlist and click **Add Ticker** to begin streaming data!
