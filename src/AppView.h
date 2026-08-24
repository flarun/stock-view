#pragma once
#include <unordered_map>
#include <string>
#include "StockModel.h"

struct AppEvents
{
  bool quit = false;
  bool saveRequested = false;
  bool loadRequested = false;
  std::string addTicker;
  std::string removeTicker;
};

class AppView
{
public:
  AppEvents Render(const std::unordered_map<std::string, StockData> &stocks, bool hasApiError);

private:
  void RenderSettingsModal();
  void RenderWorkspace(const std::unordered_map<std::string, StockData> &stocks, AppEvents &events);

  char m_tickerInput[16] = "";
  unsigned int m_centralNodeId = 0;
  bool m_showSettingsModal = false;
};