#include <imgui.h>
#include <imgui_internal.h>
#include <implot.h>
#include <algorithm>
#include <cstring>
#include <memory>

#include "AppView.h"
#include "ConfigManager.h"
#include "ChartRenderers.h"
#include "Logger.h"
#include "Indicators.h"

void AppView::ApplyTheme(AppTheme theme)
{
  ImGuiStyle &style = ImGui::GetStyle();

  switch (theme)
  {
  case AppTheme::Dark:
    ImGui::StyleColorsDark();
    break;
  case AppTheme::Light:
    ImGui::StyleColorsLight();
    break;
  case AppTheme::Classic:
    ImGui::StyleColorsClassic();
    break;
  case AppTheme::Nord:
  {
    ImGui::StyleColorsDark(); // Base it on dark mode
    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.17f, 0.21f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.26f, 0.30f, 0.36f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.17f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.23f, 0.26f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.33f, 0.38f, 0.47f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.17f, 0.21f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.23f, 0.26f, 0.32f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.33f, 0.38f, 0.47f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.45f, 0.55f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.50f, 0.55f, 0.65f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.33f, 0.38f, 0.47f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.40f, 0.45f, 0.55f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.50f, 0.55f, 0.65f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.17f, 0.21f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.33f, 0.38f, 0.47f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.23f, 0.26f, 0.32f, 1.00f);

    // Modernize the geometry
    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    break;
  }
  }
}

AppEvents AppView::Render(const std::unordered_map<std::string, StockData> &stocks, bool hasApiError)
{
  AppEvents events;

  // --- NORTH: The Main Menu Bar ---
  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      if (ImGui::MenuItem("Settings..."))
        m_showSettingsModal = true;
      ImGui::Separator();
      if (ImGui::MenuItem("Save History"))
        events.saveRequested = true;
      if (ImGui::MenuItem("Load History"))
        events.loadRequested = true;
      ImGui::Separator();
      if (ImGui::MenuItem("Exit"))
        events.quit = true;
      ImGui::EndMenu();
    }
    float fps = ImGui::GetIO().Framerate;
    std::string fpsText = "FPS: " + std::to_string((int)fps);
    ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(fpsText.c_str()).x - 20.0f);
    ImGui::TextDisabled("%s", fpsText.c_str());
    ImGui::EndMainMenuBar();
  }

  // --- FULLSCREEN ROOT DOCKSPACE ---
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGuiWindowFlags host_window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                       ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  ImGui::Begin("MainRootWindow", nullptr, host_window_flags);
  ImGui::PopStyleVar(3);

  ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
  m_centralNodeId = dockspace_id;

  if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
  {
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

    ImGuiID dock_main_id = dockspace_id;
    ImGuiID dock_id_west = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
    ImGuiID dock_id_east = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.20f, nullptr, &dock_main_id);
    ImGuiID dock_id_south = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);

    ImGui::DockBuilderDockWindow("Watchlist", dock_id_west);
    ImGui::DockBuilderDockWindow("Details", dock_id_east);
    ImGui::DockBuilderDockWindow("Console", dock_id_south);

    ImGui::DockBuilderFinish(dockspace_id);
  }

  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
  ImGui::End();

  // --- WEST: Watchlist ---
  ImGui::Begin("Watchlist");
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
      events.removeTicker = symbol;
    ImGui::SameLine();
    ImGui::Text("%s", symbol.c_str());
    ImGui::PopID();
  }
  ImGui::End();

  // --- EAST: Details ---
  ImGui::Begin("Details");
  std::string currentKey = ConfigManager::GetInstance().GetSettings().apiKey;
  if (currentKey.empty())
  {
    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "API Status: MISSING KEY");
    ImGui::TextWrapped("Go to File -> Settings to enter your Finnhub API Key.");
  }
  else if (hasApiError)
  {
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "API Status: INVALID KEY / ERROR");
    ImGui::TextWrapped("The API rejected the request. Check your key or internet connection.");
  }
  else if (stocks.empty())
  {
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "API Status: READY (Idle)");
  }
  else
  {
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "API Status: CONNECTED");
  }
  ImGui::Separator();
  ImGui::Text("Provider: Finnhub.io");
  ImGui::Text("Active Streams: %d", (int)stocks.size());
  ImGui::End();

  // --- SOUTH: Console/Footer ---
  ImGui::Begin("Console");
  if (ImGui::Button("Clear Logs"))
    Logger::GetInstance().Clear();
  ImGui::SameLine();
  ImGui::Checkbox("Auto-scroll", &m_autoScrollConsole);
  ImGui::Separator();
  ImGui::BeginChild("ConsoleLogsRegion", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
  for (const std::string &log : Logger::GetInstance().GetLogs())
  {
    ImGui::TextUnformatted(log.c_str());
  }
  if (m_autoScrollConsole && (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
  {
    ImGui::SetScrollHereY(1.0f);
  }
  ImGui::EndChild();
  ImGui::End();

  // Render the charts into the center area
  RenderWorkspace(stocks, events);
  RenderSettingsModal(events);

  return events;
}

void AppView::RenderWorkspace(const std::unordered_map<std::string, StockData> &stocks, AppEvents &events)
{
  auto &settings = ConfigManager::GetInstance().GetSettings();
  auto currentStyle = settings.chartStyle;

  ImPlot::GetStyle().UseLocalTime = settings.useLocalTime;
  ImPlot::GetStyle().Use24HourClock = settings.use24HourClock;

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
      ImGui::SameLine(ImGui::GetWindowWidth() - 150);
      if (ImGui::Button("+ Add Indicator"))
      {
        ImGui::OpenPopup("IndicatorPopup");
      }
    }

    if (ImGui::BeginPopup("IndicatorPopup"))
    {
      if (ImGui::MenuItem("Simple Moving Average (SMA)"))
      {
        settings.indicators[symbol].push_back(IndicatorConfig{IndicatorType::SMA, 10});
        ConfigManager::GetInstance().Save();
      }
      ImGui::EndPopup();
    }

    auto &activeIndicators = settings.indicators[symbol];
    for (int i = 0; i < activeIndicators.size(); ++i)
    {
      auto &ind = activeIndicators[i];
      ImGui::PushID(i);
      ImGui::SetNextItemWidth(80);

      if (ImGui::InputInt("Period", &ind.period))
        ConfigManager::GetInstance().Save();
      ImGui::SameLine();
      if (ImGui::ColorEdit4("Color", ind.color, ImGuiColorEditFlags_NoInputs))
        ConfigManager::GetInstance().Save();
      ImGui::SameLine();
      if (ImGui::Button("X"))
      {
        activeIndicators.erase(activeIndicators.begin() + i);
        ConfigManager::GetInstance().Save();
        ImGui::PopID();
        break;
      }
      ImGui::PopID();
    }
    if (ImPlot::BeginPlot(symbol.c_str(), ImVec2(-1, -1)))
    {
      ImPlot::SetupAxes("Time", "Price ($)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
      ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);

      renderers[currentStyle]->Render(symbol, data);

      if (settings.indicators.count(symbol))
      {
        for (const auto &indConfig : settings.indicators[symbol])
        {
          if (auto ind = IndicatorRegistry::Get(indConfig.type))
          {
            ind->Render(symbol, data, indConfig);
          }
        }
      }
      ImPlot::EndPlot();
    }
    ImGui::End();
  }
}

void AppView::RenderSettingsModal(AppEvents &events)
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

        // --- THEME SELECTOR ---
        int currentTheme = static_cast<int>(settings.theme);
        const char *themeItems[] = {"Dark", "Light", "Classic", "Nord (Premium)"};
        if (ImGui::Combo("Application Theme", &currentTheme, themeItems, IM_ARRAYSIZE(themeItems)))
        {
          settings.theme = static_cast<AppTheme>(currentTheme);
          ApplyTheme(settings.theme); // Instantly apply the theme!
          settingsChanged = true;
        }

        ImGui::Spacing();
        ImGui::Text("Historical Data Timeframe:");
        const char *resOptions[] = {"1", "5", "15", "30", "60", "D", "W", "M"};
        const char *resLabels[] = {"1 Minute", "5 Minutes", "15 Minutes", "30 Minutes", "1 Hour", "Daily", "Weekly", "Monthly"};

        int currentResIdx = 4;
        for (int i = 0; i < 8; ++i)
        {
          if (settings.historyResolution == resOptions[i])
          {
            currentResIdx = i;
            break;
          }
        }

        if (ImGui::Combo("##Timeframe", &currentResIdx, resLabels, IM_ARRAYSIZE(resLabels)))
        {
          settings.historyResolution = resOptions[currentResIdx];
          settingsChanged = true;
          events.changeResolution = resOptions[currentResIdx];
        }
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Time Formatting:");

        int tzIndex = settings.useLocalTime ? 0 : 1;
        const char *tzItems[] = {"Local OS Time", "UTC (Coordinated Universal Time)"};
        if (ImGui::Combo("Timezone", &tzIndex, tzItems, IM_ARRAYSIZE(tzItems)))
        {
          settings.useLocalTime = (tzIndex == 0);
          settingsChanged = true;
        }

        if (ImGui::Checkbox("Use 24-Hour Clock", &settings.use24HourClock))
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

        if (ImGui::InputText("##APIKey", keyBuffer, IM_ARRAYSIZE(keyBuffer), ImGuiInputTextFlags_Password))
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