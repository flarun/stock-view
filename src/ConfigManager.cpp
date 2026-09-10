#include "ConfigManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <keychain/keychain.h>

using json = nlohmann::json;

// JSON serializers for modular config
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
    if (j.contains("theme"))
      m_settings.theme = static_cast<AppTheme>(j["theme"].get<int>());
    if (j.contains("pollingIntervalMs"))
      m_settings.pollingIntervalMs = j["pollingIntervalMs"].get<int>();
    if (j.contains("useLocalTime"))
      m_settings.useLocalTime = j["useLocalTime"].get<bool>();
    if (j.contains("use24HourClock"))
      m_settings.use24HourClock = j["use24HourClock"].get<bool>();
    if (j.contains("historyResolution"))
      m_settings.historyResolution = j["historyResolution"].get<std::string>();
    if (j.contains("activeTickers"))
      m_settings.activeTickers = j["activeTickers"].get<std::vector<std::string>>();
    if (j.contains("indicators"))
      m_settings.indicators = j["indicators"].get<std::unordered_map<std::string, std::vector<IndicatorConfig>>>();
  }

  // --- SECURE KEYCHAIN LOAD ---
  keychain::Error err;
  std::string savedKey = keychain::getPassword("StockView", "finnhub_api", "default_user", err);
  if (!err)
  {
    m_settings.apiKey = savedKey;
  }
}

void ConfigManager::Save()
{
  json j;
  j["chartStyle"] = static_cast<int>(m_settings.chartStyle);
  j["theme"] = static_cast<int>(m_settings.theme);
  j["pollingIntervalMs"] = m_settings.pollingIntervalMs;
  j["useLocalTime"] = m_settings.useLocalTime;
  j["use24HourClock"] = m_settings.use24HourClock;
  j["historyResolution"] = m_settings.historyResolution;
  j["activeTickers"] = m_settings.activeTickers;
  j["indicators"] = m_settings.indicators;

  std::ofstream file(m_configFilePath);
  if (file.is_open())
  {
    file << j.dump(4);
  }

  // --- SECURE KEYCHAIN SAVE ---
  keychain::Error err;
  if (!m_settings.apiKey.empty())
  {
    keychain::setPassword("StockView", "finnhub_api", "default_user", m_settings.apiKey, err);
  }
  else
  {
    keychain::deletePassword("StockView", "finnhub_api", "default_user", err);
  }
}