#pragma once
#include <string>
#include <deque>
#include <vector>
#include <mutex>
#include <chrono>
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

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "[%H:%M:%S] ") << message;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.push_back(ss.str());

    // Keep memory light by holding a maximum of 100 messages
    if (m_logs.size() > 100)
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
};