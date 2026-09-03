#include "StockModel.h"
#include <algorithm> // Required for std::max and std::min

void StockModel::AddPrice(const std::string &symbol, double price, double time)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // 1. Keep storing the raw lines
  m_stocks[symbol].symbol = symbol;
  m_stocks[symbol].prices.push_back(price);
  m_stocks[symbol].timeAxis.push_back(time);

  // 2. Aggregate the OHLC Candlestick data
  auto &data = m_stocks[symbol];
  constexpr double BUCKET_SIZE = 5.0;

  if (data.candles.empty())
  {
    data.candles.push_back({time, price, price, price, price});
  }
  else
  {
    auto &last = data.candles.back();
    if (time - last.time >= BUCKET_SIZE)
    {
      data.candles.push_back({last.time + BUCKET_SIZE, price, price, price, price});
    }
    else
    {
      last.high = std::max(last.high, price);
      last.low = std::min(last.low, price);
      last.close = price;
    }
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