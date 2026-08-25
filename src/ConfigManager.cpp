#include "ConfigManager.h"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

// JSON serializers for  modular config
inline void to_json(json &j, const IndicatorConfig &c)
{
  j = json{{"type", static_cast<int>(c.type)}, {"period", c.period}, {"color", {c.color[0], c.color[1], c.color[2], c.color[3]}}};
}
inline void from_json(const json &j, IndicatorConfig &c)
{
  c.type = static_cast<IndicatorType>(j.at("type").get<int>());
  j.at("period").get_to(c.period);
  auto col = j.at("color").get<std::vector<float>>();
  if (col.size() == 4)
  {
    for (int i = 0; i < 4; ++i)
      c.color[i] = col[i];
  }
}

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
    if (j.contains("activeTickers"))
      m_settings.activeTickers = j["activeTickers"].get<std::vector<std::string>>();
    if (j.contains("indicators"))
      m_settings.indicators = j["indicators"].get<std::unordered_map<std::string, std::vector<IndicatorConfig>>>();
  }
}

void ConfigManager::Save()
{
  json j;
  j["chartStyle"] = static_cast<int>(m_settings.chartStyle);
  j["pollingIntervalMs"] = m_settings.pollingIntervalMs;
  j["useLocalTime"] = m_settings.useLocalTime;
  j["use24HourClock"] = m_settings.use24HourClock;
  j["activeTickers"] = m_settings.activeTickers;
  j["indicators"] = m_settings.indicators;

  std::ofstream file(m_configFilePath);
  if (file.is_open())
  {
    file << j.dump(4);
  }
  j["apiKey"] = m_settings.apiKey;
}