#pragma once
#include "StockModel.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

inline void to_json(json &j, const StockData &s)
{
  j = json{{"symbol", s.symbol}, {"prices", s.prices}, {"timeAxis", s.timeAxis}, {"isLive", s.isLive}};
}

inline void from_json(const json &j, StockData &s)
{
  j.at("symbol").get_to(s.symbol);
  j.at("prices").get_to(s.prices);
  j.at("timeAxis").get_to(s.timeAxis);
  s.isLive = false;
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