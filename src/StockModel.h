#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

struct OHLC
{
  float time;
  float open;
  float high;
  float low;
  float close;
};
struct StockData
{
  std::string symbol;
  std::vector<float> prices;
  std::vector<float> timeAxis;
  bool isLive = true;
  std::vector<OHLC> candles;
};

class StockModel
{
public:
  void AddPrice(const std::string &symbol, float price, float time);
  std::unordered_map<std::string, StockData> GetStocks() const;
  void LoadFromHistory(const std::unordered_map<std::string, StockData> &historyData);
  void RemoveStock(const std::string &symbol);

private:
  mutable std::mutex m_mutex;
  std::unordered_map<std::string, StockData> m_stocks;
};