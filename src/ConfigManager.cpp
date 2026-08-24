#include "ConfigManager.h"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

void ConfigManager::Load()
{
  std::ifstream file(m_configFilePath);
  if (file.is_open())
  {
    json j;
    file >> j;
    if (j.contains("chartStyle"))
      m_settings.chartStyle = static_cast<ChartStyle>(j["chartStyle"].get<int>());
    if (j.contains("pollingIntervalMs"))
      m_settings.pollingIntervalMs = j["pollingIntervalMs"].get<int>();
    if (j.contains("apiKey"))
      m_settings.apiKey = j["apiKey"].get<std::string>();
    if (j.contains("useLocalTime"))
      m_settings.useLocalTime = j["useLocalTime"].get<bool>();
    if (j.contains("use24HourClock"))
      m_settings.use24HourClock = j["use24HourClock"].get<bool>();
  }
}

void ConfigManager::Save()
{
  json j;
  j["chartStyle"] = static_cast<int>(m_settings.chartStyle);
  j["pollingIntervalMs"] = m_settings.pollingIntervalMs;
  j["useLocalTime"] = m_settings.useLocalTime;
  j["use24HourClock"] = m_settings.use24HourClock;

  std::ofstream file(m_configFilePath);
  if (file.is_open())
  {
    file << j.dump(4);
  }
  j["apiKey"] = m_settings.apiKey;
}