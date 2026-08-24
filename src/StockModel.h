#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

struct OHLC
{
  double time;
  double open;
  double high;
  double low;
  double close;
};
struct StockData
{
  std::string symbol;
  std::vector<double> prices;
  std::vector<double> timeAxis;
  bool isLive = true;
  std::vector<OHLC> candles;
};

class StockModel
{
public:
  void AddPrice(const std::string &symbol, double price, double time);
  std::unordered_map<std::string, StockData> GetStocks() const;
  void LoadFromHistory(const std::unordered_map<std::string, StockData> &historyData);
  void RemoveStock(const std::string &symbol);
  void SetApiError(bool isError) { m_apiError = isError; }
  bool HasApiError() const { return m_apiError; }

private:
  mutable std::mutex m_mutex;
  std::unordered_map<std::string, StockData> m_stocks;
  std::atomic<bool> m_apiError{false};
};