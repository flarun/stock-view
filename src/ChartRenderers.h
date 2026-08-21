#pragma once
#include "IChartRenderer.h"
#include <implot.h>
#include <algorithm>

class LineChartRenderer : public IChartRenderer
{
public:
  void Render(const std::string &symbol, const StockData &data) override
  {
    if (data.prices.size() > 1)
    {
      ImPlot::PlotLine(symbol.c_str(), data.timeAxis.data(), data.prices.data(), (int)data.prices.size());
    }
  }
};

class CandlestickRenderer : public IChartRenderer
{
public:
  void Render(const std::string &symbol, const StockData &data) override
  {
    if (data.candles.empty())
      return;

    // Register a dummy item so ImPlot creates a legend entry
    ImPlot::PlotDummy(symbol.c_str());

    // Protect our custom drawing so it doesn't spill outside the graph area
    ImPlot::PushPlotClipRect();

    // Grab the raw graphics canvas
    ImDrawList *draw_list = ImPlot::GetPlotDrawList();

    // Define the visual width of the candle body
    const float half_width = 2.0f;

    for (const auto &candle : data.candles)
    {
      // Determine Bull (Green) or Bear (Red)
      ImU32 color = (candle.close >= candle.open) ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255);

      // Convert abstract plot math (Time, Price) into literal screen pixels
      ImVec2 p_low = ImPlot::PlotToPixels(candle.time, candle.low);
      ImVec2 p_high = ImPlot::PlotToPixels(candle.time, candle.high);
      ImVec2 p_open = ImPlot::PlotToPixels(candle.time - half_width, candle.open);
      ImVec2 p_close = ImPlot::PlotToPixels(candle.time + half_width, candle.close);

      // 1. Draw the vertical Wick
      draw_list->AddLine(p_low, p_high, color, 1.0f);

      // 2. Draw the solid Body
      // (Y-axis pixels go top-to-bottom, so smaller Y is higher on the screen)
      ImVec2 p_tl = ImVec2(p_open.x, std::min(p_open.y, p_close.y));
      ImVec2 p_br = ImVec2(p_close.x, std::max(p_open.y, p_close.y));

      // If open == close, force a 1-pixel flat bar so it remains visible
      if (p_tl.y == p_br.y)
      {
        p_br.y += 1.0f;
      }

      draw_list->AddRectFilled(p_tl, p_br, color);
    }

    // Stop clipping
    ImPlot::PopPlotClipRect();
  }
};