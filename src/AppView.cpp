#include "AppView.h"
#include <imgui.h>
#include <imgui_internal.h> // REQUIRED for the DockBuilder API
#include <implot.h>
#include <algorithm>
#include <cstring>

AppEvents AppView::Render(const std::unordered_map<std::string, StockData> &stocks)
{
  AppEvents events;

  // --- 1. NORTH PANEL: The Main Menu Bar ---
  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
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

  // --- 2. LAYOUT MANAGER: Force the UI Grid ---
  ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
  ImGuiViewport *viewport = ImGui::GetMainViewport();

  // This block only runs ONCE on the very first frame to build the strict layout
  static bool init_layout = true;
  if (init_layout)
  {
    init_layout = false;

    // Wipe any old floating windows from memory
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

    ImGuiID dock_main_id = dockspace_id;

    // Split the screen into regions (West, East, South)
    ImGuiID dock_id_west = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
    ImGuiID dock_id_east = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.20f, nullptr, &dock_main_id);
    ImGuiID dock_id_south = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.15f, nullptr, &dock_main_id);

    // Snap specific windows to those exact regions
    ImGui::DockBuilderDockWindow("Watchlist (West)", dock_id_west);
    ImGui::DockBuilderDockWindow("Details (East)", dock_id_east);
    ImGui::DockBuilderDockWindow("Console (South)", dock_id_south);

    // Save the center area ID so new charts spawn perfectly in the middle
    m_centralNodeId = dock_main_id;

    ImGui::DockBuilderFinish(dockspace_id);
  }

  // Apply the dockspace to the screen
  ImGui::DockSpaceOverViewport(dockspace_id, viewport, ImGuiDockNodeFlags_None);

  // --- 3. WEST PANEL: Watchlist ---
  ImGui::Begin("Watchlist (West)");
  ImGui::Text("Add New Stock:");
  ImGui::SetNextItemWidth(-FLT_MIN);
  ImGui::InputTextWithHint("##TickerInput", "Symbol (e.g. MSFT)", m_tickerInput, IM_ARRAYSIZE(m_tickerInput));

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

  // --- 4. EAST PANEL: Details & Config ---
  ImGui::Begin("Details (East)");
  ImGui::Text("API Status:");
  ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "CONNECTED");
  ImGui::Separator();
  ImGui::Text("Provider: Finnhub.io");
  ImGui::Text("Rate Limit: 60/min");
  ImGui::Text("Active Streams: %d", (int)stocks.size());
  ImGui::End();

  // --- 5. SOUTH PANEL: Footer ---
  RenderFooter();

  // --- 6. CENTER PANEL: Workspace (Charts) ---
  RenderWorkspace(stocks, events);

  return events;
}

void AppView::RenderFooter()
{
  ImGui::Begin("Console (South)");
  ImGui::Text("System successfully initialized.");
  ImGui::Text("Data Service Background Worker is tracking data safely...");
  ImGui::End();
}

void AppView::RenderWorkspace(const std::unordered_map<std::string, StockData> &stocks, AppEvents &events)
{
  for (const auto &[symbol, data] : stocks)
  {
    std::string windowName = symbol + " Chart";

    // Force the chart to spawn in the center panel the very first time it is opened!
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

    if (ImPlot::BeginPlot(symbol.c_str(), ImVec2(-1, -1))) // ImVec2(-1,-1) tells the chart to fill the window space!
    {
      ImPlot::SetupAxes("Time (s)", "Price ($)");
      if (data.prices.size() > 1)
      {
        ImPlot::PlotLine(symbol.c_str(), data.timeAxis.data(), data.prices.data(), (int)data.prices.size());
      }
      else if (data.prices.size() == 1)
      {
        ImPlot::PlotScatter(symbol.c_str(), data.timeAxis.data(), data.prices.data(), 1);
      }
      ImPlot::EndPlot();
    }
    ImGui::End();
  }
}