#pragma once
#include <unordered_map>
#include <string>
#include "StockModel.h"

struct AppEvents
{
  bool quit = false;
  bool saveRequested = false;
  bool loadRequested = false;
};

class AppView
{
public:
  AppEvents Render(const std::unordered_map<std::string, StockData> &stocks);

private:
  bool RenderHeader();
  void RenderWorkspace(const std::unordered_map<std::string, StockData> &stocks);
  void RenderFooter();
};