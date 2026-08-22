#include <imgui.h>
#include <implot.h>
#include <algorithm>
#include <cstring>
#include <memory>

#include "AppView.h"
#include "ConfigManager.h"
#include "ChartRenderers.h"

AppEvents AppView::Render(const std::unordered_map<std::string, StockData> &stocks)
{
  AppEvents events;

  // --- NORTH: The Main Menu Bar ---
  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      if (ImGui::MenuItem("Settings..."))
      {
        m_showSettingsModal = true;
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Save History"))
      {
        events.saveRequested = true;
      }
      if (ImGui::MenuItem("Load History"))
      {
        events.loadRequested = true;
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Exit"))
      {
        events.quit = true;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  // Get the usable screen space
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImVec2 workPos = viewport->WorkPos;
  ImVec2 workSize = viewport->WorkSize;

  // Fixed sizes
  float westWidth = 250.0f;
  float eastWidth = 250.0f;
  float southHeight = 120.0f;

  // THE FIX: NoDocking + NoSavedSettings makes ImGui ignore imgui.ini entirely for these panels.
  ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking |
                                ImGuiWindowFlags_NoSavedSettings;

  // --- WEST: Watchlist ---
  ImGui::SetNextWindowPos(ImVec2(workPos.x, workPos.y), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(westWidth, workSize.y - southHeight), ImGuiCond_Always);
  ImGui::Begin("Watchlist", nullptr, panelFlags);

  ImGui::Text("Add New Stock:");
  ImGui::SetNextItemWidth(-FLT_MIN);
  ImGui::InputTextWithHint("##TickerInput", "Symbol", m_tickerInput, IM_ARRAYSIZE(m_tickerInput));
  if (ImGui::Button("Add Ticker", ImVec2(-FLT_MIN, 0)))
  {
    if (strlen(m_tickerInput) > 0)
    {
      std::string newSymbol = m_tickerInput;
      std::transform(newSymbol.begin(), newSymbol.end(), newSymbol.begin(), ::toupper);
      events.addTicker = newSymbol;
      m_tickerInput[0] = '\0';
    }
  }
  ImGui::Separator();
  ImGui::Text("Active Tickers:");
  for (const auto &[symbol, data] : stocks)
  {
    ImGui::PushID(symbol.c_str());
    if (ImGui::Button("X"))
    {
      events.removeTicker = symbol;
    }
    ImGui::SameLine();
    ImGui::Text("%s", symbol.c_str());
    ImGui::PopID();
  }
  ImGui::End();

  // --- EAST: Details ---
  ImGui::SetNextWindowPos(ImVec2(workPos.x + workSize.x - eastWidth, workPos.y), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(eastWidth, workSize.y - southHeight), ImGuiCond_Always);
  ImGui::Begin("Details", nullptr, panelFlags);
  ImGui::Text("API Status: CONNECTED");
  ImGui::Separator();
  ImGui::Text("Provider: Finnhub.io");
  ImGui::Text("Active Streams: %d", (int)stocks.size());
  ImGui::End();

  // --- SOUTH: Console/Footer ---
  ImGui::SetNextWindowPos(ImVec2(workPos.x, workPos.y + workSize.y - southHeight), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(workSize.x, southHeight), ImGuiCond_Always);
  ImGui::Begin("Console", nullptr, panelFlags);
  ImGui::Text("System initialized securely.");
  ImGui::Text("Data Service worker is running in the background...");
  ImGui::End();

  // --- CENTER: The Workspace Container ---
  ImGui::SetNextWindowPos(ImVec2(workPos.x + westWidth, workPos.y), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(workSize.x - westWidth - eastWidth, workSize.y - southHeight), ImGuiCond_Always);

  ImGuiWindowFlags centerFlags = panelFlags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus;
  ImGui::Begin("WorkspaceArea", nullptr, centerFlags);

  m_centralNodeId = ImGui::GetID("CenterDockSpace");
  ImGui::DockSpace(m_centralNodeId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

  ImGui::End();

  // Render the charts
  RenderWorkspace(stocks, events);

  RenderSettingsModal();

  return events;
}

void AppView::RenderWorkspace(const std::unordered_map<std::string, StockData> &stocks, AppEvents &events)
{
  // 1. Get the current global chart style
  auto currentStyle = ConfigManager::GetInstance().GetSettings().chartStyle;

  // 2. The Strategy Registry (Polymorphism in action)
  static std::unordered_map<ChartStyle, std::unique_ptr<IChartRenderer>> renderers;
  if (renderers.empty())
  {
    renderers[ChartStyle::Line] = std::make_unique<LineChartRenderer>();
    renderers[ChartStyle::Candlestick] = std::make_unique<CandlestickRenderer>();
  }

  for (const auto &[symbol, data] : stocks)
  {
    std::string windowName = symbol + " Chart";

    ImGui::SetNextWindowDockID(m_centralNodeId, ImGuiCond_FirstUseEver);

    bool isOpen = true;
    ImGui::Begin(windowName.c_str(), &isOpen);
    if (!isOpen)
    {
      events.removeTicker = symbol;
    }

    if (!data.prices.empty())
    {
      ImGui::Text("Last price: %.2f", data.prices.back());
    }

    if (ImPlot::BeginPlot(symbol.c_str(), ImVec2(-1, -1)))
    {
      ImPlot::SetupAxes("Time (s)", "Price ($)");

      // 3. Render dynamically with zero branching
      renderers[currentStyle]->Render(symbol, data);

      ImPlot::EndPlot();
    }
    ImGui::End();
  }
}

void AppView::RenderSettingsModal()
{
  if (m_showSettingsModal)
  {
    ImGui::OpenPopup("Preferences");
    m_showSettingsModal = false;
  }

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(500, 350), ImGuiCond_FirstUseEver);

  if (ImGui::BeginPopupModal("Preferences", nullptr, ImGuiWindowFlags_NoCollapse))
  {
    auto &settings = ConfigManager::GetInstance().GetSettings();
    bool settingsChanged = false;

    if (ImGui::BeginTabBar("SettingsTabs"))
    {
      // TAB 1: CHARTING
      if (ImGui::BeginTabItem("Charting"))
      {
        ImGui::Spacing();
        int currentChart = static_cast<int>(settings.chartStyle);
        const char *items[] = {"Line Chart", "Candlestick"};
        if (ImGui::Combo("Global Chart Style", &currentChart, items, IM_ARRAYSIZE(items)))
        {
          settings.chartStyle = static_cast<ChartStyle>(currentChart);
          settingsChanged = true;
        }
        ImGui::EndTabItem();
      }

      // TAB 2: NETWORK
      if (ImGui::BeginTabItem("Network"))
      {
        ImGui::Spacing();
        if (ImGui::SliderInt("Polling Rate (ms)", &settings.pollingIntervalMs, 1000, 10000))
        {
          settingsChanged = true;
        }
        ImGui::EndTabItem();
      }
      // TAB 3: API
      if (ImGui::BeginTabItem("API"))
      {
        ImGui::Spacing();
        ImGui::Text("Finnhub API Key:");

        static char keyBuffer[128] = "";
        if (keyBuffer[0] == '\0' && !settings.apiKey.empty())
        {
          strncpy(keyBuffer, settings.apiKey.c_str(), sizeof(keyBuffer) - 1);
        }

        if (ImGui::InputText("##APIKey", keyBuffer, IM_ARRAYSIZE(keyBuffer)))
        {
          settings.apiKey = std::string(keyBuffer);
          settingsChanged = true;
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Get a free API key at finnhub.io");
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }

    ImGui::Separator();
    if (ImGui::Button("Close", ImVec2(120, 0)))
    {
      if (settingsChanged)
        ConfigManager::GetInstance().Save();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}