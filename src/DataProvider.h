#pragma once
#include <string>
#include <chrono>
#include <mutex>
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "config.h"

// --- Token Bucket Rate Limiter ---
class RateLimiter
{
public:
  RateLimiter(int capacity, float tokensPerSecond)
      : m_capacity(capacity), m_tokens(capacity), m_tokensPerSecond(tokensPerSecond)
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

// --- Data Provider Interface ---
class IDataProvider
{
public:
  virtual ~IDataProvider() = default;
  virtual bool CanFetch() = 0;
  virtual float FetchPrice(const std::string &symbol) = 0;
};

// --- Finnhub Concrete Implementation ---
class FinnhubProvider : public IDataProvider
{
public:
  // Finnhub allows 60 calls per minute (1 token per second)
  FinnhubProvider() : m_rateLimiter(60, 1.0f) {}

  bool CanFetch() override
  {
    return m_rateLimiter.TryConsume();
  }

  float FetchPrice(const std::string &symbol) override
  {
    CURL *curl = curl_easy_init();
    std::string response;
    if (curl)
    {
      std::string url = "https://finnhub.io/api/v1/quote?symbol=" + symbol + "&token=" + FINNHUB_API_KEY;
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

      CURLcode res = curl_easy_perform(curl);
      if (res != CURLE_OK)
      {
        std::cerr << "curl error: " << curl_easy_strerror(res) << std::endl;
      }
      curl_easy_cleanup(curl);
    }

    try
    {
      auto j = nlohmann::json::parse(response);
      return j["c"].get<float>();
    }
    catch (...)
    {
      return -1.0f;
    }
  }

private:
  static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
  {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
  }

  RateLimiter m_rateLimiter;
};