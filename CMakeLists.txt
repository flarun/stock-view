cmake_minimum_required(VERSION 3.20)
project(stock_view)
set(APP_VERSION "0.1.6-beta")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ixwebsocket CONFIG REQUIRED)

# Base source files
set(APP_SOURCES
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

# --- PLATFORM SPECIFIC PACKAGING ---
if(APPLE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.icns")
        set_source_files_properties(resources/app.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        list(APPEND APP_SOURCES resources/app.icns)
    else()
        message(WARNING "app.icns not found! Building macOS bundle without a custom icon.")
    endif()
elseif(WIN32)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rc")
        list(APPEND APP_SOURCES resources/app.rc)
    endif()
endif()

# We add MACOSX_BUNDLE to create a .app on Mac. (Ignored on Windows/Linux)
add_executable(stock_view MACOSX_BUNDLE ${APP_SOURCES})

target_link_libraries(stock_view PRIVATE
    imgui::imgui
    implot::implot
    CURL::libcurl
    nlohmann_json::nlohmann_json
    glfw
    OpenGL::GL
    ixwebsocket::ixwebsocket
)

target_compile_definitions(stock_view PRIVATE APP_VERSION=\"${APP_VERSION}\")

# macOS Bundle Properties
if(APPLE)
    set_target_properties(stock_view PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "StockView"
        MACOSX_BUNDLE_ICON_FILE "app.icns"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.flarun.stockview"
    )
endif()

# --- INSTALLATION SETTINGS ---
if(APPLE)
    install(TARGETS stock_view BUNDLE DESTINATION .)
else()
    install(TARGETS stock_view DESTINATION bin)
endif()

# Linux specific desktop and icon installation (Made Optional)
if(UNIX AND NOT APPLE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/stock-view.desktop")
        install(FILES resources/stock-view.desktop DESTINATION share/applications)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.png")
        install(FILES resources/app.png DESTINATION share/icons/hicolor/256x256/apps RENAME stock-view.png)
    endif()
endif()

# --- PACKAGING SETTINGS (CPack) ---
set(CPACK_PACKAGE_NAME "StockView")
set(CPACK_PACKAGE_VERSION "${APP_VERSION}")

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop") # Creates a macOS .dmg file
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS")      # Creates a professional Windows installer
else()
    set(CPACK_GENERATOR "DEB")       # Creates a Linux .deb installer
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "flarun")
endif()

include(CPack)