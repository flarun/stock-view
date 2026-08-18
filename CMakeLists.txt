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
    src/StockModel.cpp 
    src/AppView.cpp
)

target_link_libraries(stock_view PRIVATE
    imgui::imgui
    implot::implot
    CURL::libcurl
    nlohmann_json::nlohmann_json
    d3d11 d3dcompiler
)