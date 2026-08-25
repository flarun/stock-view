#pragma once
#include <string>
#include <vector>
#include <unordered_map>

enum class ChartStyle
{
  Line,
  Candlestick
};
enum class IndicatorType
{
  SMA
};

struct IndicatorConfig
{
  IndicatorType type = IndicatorType::SMA;
  int period = 10;
  float color[4] = {1.0f, 0.65f, 0.0f, 1.0f}; // Default Orange
};

struct AppSettings
{
  ChartStyle chartStyle = ChartStyle::Line;
  int pollingIntervalMs = 5000;
  std::string apiKey = "";
  bool useLocalTime = true;
  bool use24HourClock = false;
  std::vector<std::string> activeTickers = {};
  std::unordered_map<std::string, std::vector<IndicatorConfig>> indicators;
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