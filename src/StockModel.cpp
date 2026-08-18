#include "StockModel.h"

void StockModel::AddPrice(const std::string &symbol, float price, float time)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_stocks[symbol].symbol = symbol;
  m_stocks[symbol].prices.push_back(price);
  m_stocks[symbol].timeAxis.push_back(time);
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