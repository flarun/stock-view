#pragma once
#include <string>
#include <deque>
#include <vector>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

class Logger
{
public:
  static Logger &GetInstance()
  {
    static Logger instance;
    return instance;
  }

  void Log(const std::string &message)
  {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    // Thread-safe localtime (Log is called from UI, worker and websocket threads)
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &in_time_t);
#else
    localtime_r(&in_time_t, &tm_buf);
#endif

    std::stringstream ss;
    ss << std::put_time(&tm_buf, "[%H:%M:%S] ") << message;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.push_back(ss.str());

    // Keep memory light by holding a maximum of 100 messages
    if (m_logs.size() > MAX_LOGS)
    {
      m_logs.pop_front();
    }
  }

  std::vector<std::string> GetLogs()
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::vector<std::string>(m_logs.begin(), m_logs.end());
  }

  void Clear()
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.clear();
  }

private:
  Logger() = default;
  std::deque<std::string> m_logs;
  std::mutex m_mutex;
  constexpr static size_t MAX_LOGS = 100;
};