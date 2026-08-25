cmake_minimum_required(VERSION 3.20)
project(stock_view)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(stock_view WIN32 
    src/main.cpp
    src/AppView.cpp
    src/AppView.h
    src/StockModel.cpp
    src/StockModel.h
    src/DataService.cpp
    src/DataService.h
    src/DataProvider.h
    src/HistoryStorage.h
    src/ConfigManager.cpp
    src/ConfigManager.h
    src/IChartRenderer.h
    src/ChartRenderers.h
    src/Logger.h
    src/Indicators.h
)

target_link_libraries(stock_view PRIVATE
    imgui::imgui
    implot::implot
    CURL::libcurl
    nlohmann_json::nlohmann_json
    d3d11 d3dcompiler
)

# --- 1. Tell CMake what files to package ---
# Install the main executable
install(TARGETS stock_view DESTINATION bin)

# Install required DLLs (like libcurl) alongside the exe
install(FILES ${CMAKE_BINARY_DIR}/Debug/libcurl-d.dll DESTINATION bin OPTIONAL)
install(FILES ${CMAKE_BINARY_DIR}/Release/libcurl.dll DESTINATION bin OPTIONAL)

# --- 2. Configure CPack ---
set(CPACK_PACKAGE_NAME "StockView")
set(CPACK_PACKAGE_VENDOR "Your Name")
set(CPACK_PACKAGE_VERSION "1.0.0")

# Generate a clean ZIP file (You can change this to "NSIS" later to make a Setup.exe wizard)
set(CPACK_GENERATOR "ZIP") 

include(CPack)