#pragma once
#include <string>
#include "StockModel.h"
#include <implot.h>

class IChartRenderer
{
public:
  virtual ~IChartRenderer() = default;

  // Every chart style must implement this function
  virtual void Render(const std::string &symbol, const StockData &data) = 0;
};