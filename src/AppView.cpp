#include "AppView.h"
#include <imgui.h>
#include <implot.h>

AppEvents AppView::Render(const std::unordered_map<std::string, StockData> &stocks)
{
  AppEvents events;

  // Render the Header directly here to easily fill the events struct
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

  RenderWorkspace(stocks);
  RenderFooter();

  return events;
}

void AppView::RenderWorkspace(const std::unordered_map<std::string, StockData> &stocks)
{
  for (const auto &[symbol, data] : stocks)
  {
    std::string windowName = symbol + " Chart";
    ImGui::Begin(windowName.c_str());

    if (!data.prices.empty())
    {
      ImGui::Text("Last price: %.2f", data.prices.back());
    }

    if (ImPlot::BeginPlot(symbol.c_str()))
    {
      ImPlot::SetupAxes("Time (s)", "Price ($)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
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