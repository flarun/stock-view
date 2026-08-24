#pragma once
#include <string>

enum class ChartStyle
{
  Line,
  Candlestick
};

struct AppSettings
{
  ChartStyle chartStyle = ChartStyle::Line;
  int pollingIntervalMs = 5000;
  std::string apiKey = "";
  bool useLocalTime = true;
  bool use24HourClock = false;
};

class ConfigManager
{
public:
  // Standard Meyer's Singleton for global config access
  static ConfigManager &GetInstance()
  {
    static ConfigManager instance;
    return instance;
  }

  AppSettings &GetSettings() { return m_settings; }

  void Load();
  void Save();

private:
  ConfigManager() = default;
  AppSettings m_settings;
  const std::string m_configFilePath = "settings.json";
};