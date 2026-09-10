#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <nfd.h>

#include "StockModel.h"
#include "AppView.h"
#include "HistoryStorage.h"
#include "DataProvider.h"
#include "DataService.h"
#include "ConfigManager.h"
#include "Logger.h"

// --- Native File Dialog Helpers ---
std::string OpenSaveFileDialog()
{
  nfdchar_t *outPath = nullptr;
  nfdfilteritem_t filterItem[1] = {{"JSON Workspace", "json"}};

  nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, nullptr, "stock_history.json");
  std::string res = "";
  if (result == NFD_OKAY)
  {
    res = outPath;
    NFD_FreePath(outPath);
  }
  return res;
}

std::string OpenLoadFileDialog()
{
  nfdchar_t *outPath = nullptr;
  nfdfilteritem_t filterItem[1] = {{"JSON Workspace", "json"}};

  nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);
  std::string res = "";
  if (result == NFD_OKAY)
  {
    res = outPath;
    NFD_FreePath(outPath);
  }
  return res;
}

static void glfw_error_callback(int error, const char *description)
{
  std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

int main(int, char **)
{
  // 1. Setup GLFW
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return 1;

  // Initialize Native File Dialogs (REQUIRED)
  NFD_Init();

#if defined(__APPLE__)
  // GL 3.2 + GLSL 150 for macOS
  const char *glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Required on Mac
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
#else
  // GL 3.0 + GLSL 130 for Linux/Windows
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

  // Create window with graphics context
  std::string windowTitle = std::string("Stock View v") + APP_VERSION;
  GLFWwindow *window = glfwCreateWindow(1000, 700, windowTitle.c_str(), nullptr, nullptr);
  if (window == nullptr)
    return 1;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // 2. Setup Dear ImGui Context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

// --- DPI SCALING MATH FOR LINUX / WINDOWS ---
#if !defined(__APPLE__)
  float xscale, yscale;
  glfwGetWindowContentScale(window, &xscale, &yscale);
  ImGui::GetIO().FontGlobalScale = xscale;
  ImGui::GetStyle().ScaleAllSizes(xscale);
#endif

  ConfigManager::GetInstance().Load();
#if defined(__APPLE__)
  Logger::GetInstance().Log("[SYSTEM] Application booted on macOS. Settings loaded.");
#elif defined(_WIN32)
  Logger::GetInstance().Log("[SYSTEM] Application booted on Windows. Settings loaded.");
#else
  Logger::GetInstance().Log("[SYSTEM] Application booted on Linux. Settings loaded.");
#endif

  // 3. Initialize Architecture
  StockModel model;
  AppView view;

  auto provider = std::make_shared<FinnhubProvider>();
  DataService dataService(model, provider);
  dataService.Start();

  auto &activeTickers = ConfigManager::GetInstance().GetSettings().activeTickers;
  for (const std::string &ticker : activeTickers)
  {
    dataService.EnqueueFetch(ticker, 0.0, TaskType::History);
  }

  // 4. Main Loop
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    double currentAppTime = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Render View & Handle Events
    AppEvents events = view.Render(model.GetStocks(), model.HasApiError());

    if (events.quit)
      glfwSetWindowShouldClose(window, true);

    if (events.saveRequested)
    {
      std::string path = OpenSaveFileDialog();
      if (!path.empty())
        HistoryStorage::Save(model.GetStocks(), path);
    }
    if (events.loadRequested)
    {
      std::string path = OpenLoadFileDialog();
      if (!path.empty())
      {
        auto loadedData = HistoryStorage::Load(path);
        if (!loadedData.empty())
          model.LoadFromHistory(loadedData);
      }
    }

    // --- Handle Timeframe Changes ---
    if (!events.changeResolution.empty())
    {
      for (const std::string &ticker : activeTickers)
      {
        model.RemoveStock(ticker);                                           // Wipe the old graph
        dataService.EnqueueFetch(ticker, currentAppTime, TaskType::History); // Fetch new resolution
      }
    }

    if (!events.addTicker.empty())
    {
      if (std::find(activeTickers.begin(), activeTickers.end(), events.addTicker) == activeTickers.end())
      {
        activeTickers.push_back(events.addTicker);
        Logger::GetInstance().Log("[WATCHLIST] Added ticker: " + events.addTicker);
        ConfigManager::GetInstance().Save();

        // Fetch historical background data, and instantly connect to the live WebSocket!
        dataService.EnqueueFetch(events.addTicker, currentAppTime, TaskType::History);
        dataService.Subscribe(events.addTicker);
      }
    }

    if (!events.removeTicker.empty())
    {
      activeTickers.erase(std::remove(activeTickers.begin(), activeTickers.end(), events.removeTicker), activeTickers.end());
      Logger::GetInstance().Log("[WATCHLIST] Removed ticker: " + events.removeTicker);
      model.RemoveStock(events.removeTicker);
      ConfigManager::GetInstance().Save();

      // Stop receiving trades for this ticker
      dataService.Unsubscribe(events.removeTicker);
    }

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // 5. Clean Shutdown
  dataService.Stop();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);

  // Clean up Native File Dialogs (REQUIRED)
  NFD_Quit();
  glfwTerminate();

  return 0;
}