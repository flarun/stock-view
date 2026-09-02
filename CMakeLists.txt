cmake_minimum_required(VERSION 3.20)
project(stock_view)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(stock_view  
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
    glfw
    OpenGL::GL
)

# --- PACKAGING SETTINGS (CPack) ---
# Tell CMake what exactly needs to be put inside the installer
install(TARGETS stock_view DESTINATION bin)

set(CPACK_PACKAGE_NAME "StockView")
set(CPACK_PACKAGE_VERSION "1.0.0")

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop") # Creates a macOS .dmg file
elseif(WIN32)
    set(CPACK_GENERATOR "ZIP")       # Creates a Windows .zip containing the .exe
else()
    set(CPACK_GENERATOR "DEB")       # Creates a Linux .deb installer
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "flarun")
endif()

include(CPack)