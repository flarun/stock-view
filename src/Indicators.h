#pragma once
#include <string>
#include <vector>
#include <limits>
#include <implot.h>
#include "StockModel.h"
#include "ConfigManager.h"

// 1. The pure Interface
class IIndicator
{
public:
  virtual ~IIndicator() = default;
  virtual void Render(const std::string &symbol, const StockData &data, const IndicatorConfig &config) = 0;
};

// 2. A modular implementation
class SMAIndicator : public IIndicator
{
public:
  void Render(const std::string &symbol, const StockData &data, const IndicatorConfig &config) override
  {
    if (data.prices.size() < (size_t)config.period || config.period <= 0)
      return;

    // Calculate math on-the-fly. We fill empty spaces with NAN so ImPlot ignores them.
    std::vector<double> sma(data.prices.size(), std::numeric_limits<double>::quiet_NaN());
    double sum = 0.0;

    for (size_t i = 0; i < data.prices.size(); ++i)
    {
      sum += data.prices[i];
      if (i >= (size_t)config.period)
        sum -= data.prices[i - config.period];
      if (i >= (size_t)config.period - 1)
        sma[i] = sum / config.period;
    }

    // THE FIX: We removed the strict color override.
    // ImPlot will now safely auto-assign a vibrant color from its global colormap!

    std::string label = symbol + " SMA (" + std::to_string(config.period) + ")###SMA" + std::to_string(config.period) + symbol;
    ImPlot::PlotLine(label.c_str(), data.timeAxis.data(), sma.data(), (int)sma.size());
  }
};

// 3. The Factory Registry
class IndicatorRegistry
{
public:
  static IIndicator *Get(IndicatorType type)
  {
    static SMAIndicator sma;
    if (type == IndicatorType::SMA)
      return &sma;
    return nullptr;
  }
};