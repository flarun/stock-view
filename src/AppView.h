#pragma once
#include <unordered_map>
#include <string>
#include "StockModel.h"

class AppView
{
public:
  bool Render(const std::unordered_map<std::string, StockData> &stocks);

private:
  bool RenderHeader();
  void RenderWorkspace(const std::unordered_map<std::string, StockData> &stocks);
  void RenderFooter();
};