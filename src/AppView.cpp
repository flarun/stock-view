#include "AppView.h"
#include <imgui.h>
#include <implot.h>
#include <algorithm>
#include <cstring>

AppEvents AppView::Render(const std::unordered_map<std::string, StockData> &stocks)
{
  AppEvents events;

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

    ImGui::Separator();
    ImGui::SetNextItemWidth(120);
    ImGui::InputTextWithHint("##TickerInput", "Symbol (e.g. MSFT)", m_tickerInput, IM_ARRAYSIZE(m_tickerInput));

    if (ImGui::Button("Add Ticker"))
    {
      if (strlen(m_tickerInput) > 0)
      {
        std::string newSymbol = m_tickerInput;
        std::transform(newSymbol.begin(), newSymbol.end(), newSymbol.begin(), ::toupper);
        events.addTicker = newSymbol;
        m_tickerInput[0] = '\0';
      }
    }

    ImGui::EndMainMenuBar();
  }

  RenderWorkspace(stocks, events);
  RenderFooter();

  return events;
}

void AppView::RenderWorkspace(const std::unordered_map<std::string, StockData> &stocks, AppEvents &events)
{
  for (const auto &[symbol, data] : stocks)
  {
    std::string windowName = symbol + " Chart";

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

    if (ImPlot::BeginPlot(symbol.c_str()))
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

void AppView::RenderFooter()
{
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 30));
  ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 30));

  ImGuiWindowFlags footerFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings;

  ImGui::Begin("Footer", nullptr, footerFlags);
  ImGui::Text("API Status: Connected | Polling Interval: 5.0s");
  ImGui::End();
}