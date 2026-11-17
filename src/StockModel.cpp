#include "StockModel.h"
#include <algorithm> // Required for std::max and std::min
#include <cmath>     // Required for std::floor

void StockModel::AddPrice(const std::string &symbol, double price, double time)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // 1. Keep storing the raw lines
  auto &data = m_stocks[symbol];
  data.symbol = symbol;
  data.prices.push_back(price);
  data.timeAxis.push_back(time);

  // 2. Aggregate the OHLC Candlestick data into fixed time buckets
  constexpr double BUCKET_SIZE = 5.0;
  double bucketStart = std::floor(time / BUCKET_SIZE) * BUCKET_SIZE;

  if (data.candles.empty() || bucketStart > data.candles.back().time)
  {
    data.candles.push_back({bucketStart, price, price, price, price});
  }
  else
  {
    auto &last = data.candles.back();
    last.high = std::max(last.high, price);
    last.low = std::min(last.low, price);
    last.close = price;
  }
}

std::unordered_map<std::string, StockData> StockModel::GetStocks() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_stocks;
}

void StockModel::LoadFromHistory(const std::unordered_map<std::string, StockData> &historyData)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_stocks = historyData;
}

void StockModel::RemoveStock(const std::string &symbol)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_stocks.erase(symbol);
}