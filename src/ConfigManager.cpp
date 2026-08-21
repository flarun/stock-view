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
  }
}

void ConfigManager::Save()
{
  json j;
  j["chartStyle"] = static_cast<int>(m_settings.chartStyle);
  j["pollingIntervalMs"] = m_settings.pollingIntervalMs;

  std::ofstream file(m_configFilePath);
  if (file.is_open())
  {
    file << j.dump(4);
  }
}