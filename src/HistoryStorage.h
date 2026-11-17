#pragma once
#include "StockModel.h"
#include <nlohmann/json.hpp>
#include <array>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

inline void to_json(json &j, const StockData &s)
{
  std::vector<std::array<double, 5>> candles;
  candles.reserve(s.candles.size());
  for (const auto &c : s.candles)
    candles.push_back({c.time, c.open, c.high, c.low, c.close});

  j = json{{"symbol", s.symbol}, {"prices", s.prices}, {"timeAxis", s.timeAxis}, {"isLive", s.isLive}, {"candles", candles}};
}

inline void from_json(const json &j, StockData &s)
{
  j.at("symbol").get_to(s.symbol);
  j.at("prices").get_to(s.prices);
  j.at("timeAxis").get_to(s.timeAxis);
  s.isLive = false;

  s.candles.clear();
  if (j.contains("candles"))
  {
    for (const auto &k : j.at("candles").get<std::vector<std::array<double, 5>>>())
      s.candles.push_back({k[0], k[1], k[2], k[3], k[4]});
  }
}

class HistoryStorage
{
public:
  static void Save(const std::unordered_map<std::string, StockData> &data, const std::string &filepath)
  {
    try
    {
      json j = data;
      std::ofstream file(filepath);
      if (file.is_open())
      {
        file << j.dump(4);
      }
    }
    catch (const std::exception &e)
    {
      std::cerr << "Failed to save JSON: " << e.what() << '\n';
    }
  }

  static std::unordered_map<std::string, StockData> Load(const std::string &filepath)
  {
    std::unordered_map<std::string, StockData> data;
    try
    {
      std::ifstream file(filepath);
      if (file.is_open())
      {
        json j;
        file >> j;
        data = j.get<std::unordered_map<std::string, StockData>>();
      }
    }
    catch (const std::exception &e)
    {
      std::cerr << "Failed to load JSON: " << e.what() << '\n';
    }
    return data;
  }
};