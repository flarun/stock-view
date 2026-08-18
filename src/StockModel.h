#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

struct StockData
{
  std::string symbol;
  std::vector<float> prices;
  std::vector<float> timeAxis;
  bool isLive = true;
};

class StockModel
{
public:
  void AddPrice(const std::string &symbol, float price, float time);
  std::unordered_map<std::string, StockData> GetStocks() const;

private:
  mutable std::mutex m_mutex;
  std::unordered_map<std::string, StockData> m_stocks;
};