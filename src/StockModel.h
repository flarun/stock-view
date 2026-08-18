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
  void LoadFromHistory(const std::unordered_map<std::string, StockData> &historyData)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stocks = historyData; // Overwrite current state with the loaded file
  }

private:
  mutable std::mutex m_mutex;
  std::unordered_map<std::string, StockData> m_stocks;
};
