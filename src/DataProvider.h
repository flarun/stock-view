#pragma once
#include <string>
#include <chrono>
#include <mutex>
#include <iostream>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "ConfigManager.h"
#include "Logger.h"

// --- Token Bucket Rate Limiter ---
class RateLimiter
{
public:
  RateLimiter(int capacity, float tokensPerSecond) : m_capacity(capacity), m_tokens(capacity), m_tokensPerSecond(tokensPerSecond)
  {
    m_lastRefill = std::chrono::steady_clock::now();
  }

  bool TryConsume()
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = std::chrono::steady_clock::now();
    float secondsPassed = std::chrono::duration<float>(now - m_lastRefill).count();
    m_tokens += secondsPassed * m_tokensPerSecond;
    if (m_tokens > m_capacity)
      m_tokens = m_capacity;
    m_lastRefill = now;
    if (m_tokens >= 1.0f)
    {
      m_tokens -= 1.0f;
      return true;
    }
    return false;
  }

private:
  int m_capacity;
  float m_tokens;
  float m_tokensPerSecond;
  std::chrono::steady_clock::time_point m_lastRefill;
  std::mutex m_mutex;
};

// NEW: Struct to hold historical batches
struct HistoryChunk
{
  std::vector<double> timestamps;
  std::vector<double> prices;
};

// --- Data Provider Interface ---
class IDataProvider
{
public:
  virtual ~IDataProvider() = default;
  virtual bool CanFetch() = 0;
  virtual double FetchPrice(const std::string &symbol) = 0;

  // NEW: History function
  virtual HistoryChunk FetchHistory(const std::string &symbol, long long fromTime, long long toTime) = 0;
};

// --- Finnhub Concrete Implementation ---
class FinnhubProvider : public IDataProvider
{
public:
  FinnhubProvider() : m_rateLimiter(60, 1.0f) {}

  bool CanFetch() override { return m_rateLimiter.TryConsume(); }

  double FetchPrice(const std::string &symbol) override
  {
    CURL *curl = curl_easy_init();
    std::string response;
    if (curl)
    {
      std::string apiKey = ConfigManager::GetInstance().GetSettings().apiKey;
      if (apiKey.empty())
        return -1.0;
      std::string url = "https://finnhub.io/api/v1/quote?symbol=" + symbol + "&token=" + apiKey;
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
      CURLcode res = curl_easy_perform(curl);
      curl_easy_cleanup(curl);
    }
    try
    {
      auto j = nlohmann::json::parse(response);
      return j["c"].get<double>();
    }
    catch (...)
    {
      return -1.0;
    }
  }

  HistoryChunk FetchHistory(const std::string &symbol, long long fromTime, long long toTime) override
  {
    HistoryChunk chunk;
    CURL *curl = curl_easy_init();
    std::string response;
    if (curl)
    {
      std::string apiKey = ConfigManager::GetInstance().GetSettings().apiKey;
      if (apiKey.empty())
        return chunk;

      std::string url = "https://finnhub.io/api/v1/stock/candle?symbol=" + symbol + "&resolution=60&from=" + std::to_string(fromTime) + "&to=" + std::to_string(toTime) + "&token=" + apiKey;
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

      // --- THE FIX: Disable strict SSL verification so Windows doesn't block it ---
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

      CURLcode res = curl_easy_perform(curl);
      if (res != CURLE_OK)
      {
        Logger::GetInstance().Log(std::string("[ERROR] CURL Network failure: ") + curl_easy_strerror(res));
      }
      curl_easy_cleanup(curl);
    }

    try
    {
      if (!response.empty())
      {
        auto j = nlohmann::json::parse(response);
        // Check if Finnhub explicitly says the data is OK
        if (j.contains("s") && j["s"] == "ok")
        {
          chunk.prices = j["c"].get<std::vector<double>>();
          chunk.timestamps = j["t"].get<std::vector<double>>();
        }
        else
        {
          // --- X-RAY LOGGING: Tell the console EXACTLY what Finnhub said ---
          Logger::GetInstance().Log("[API RESPONSE] " + response);
        }
      }
    }
    catch (...)
    {
      Logger::GetInstance().Log("[ERROR] Failed to parse JSON. Raw: " + response);
    }
    return chunk;
  }

private:
  static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
  {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
  }
  RateLimiter m_rateLimiter;
};