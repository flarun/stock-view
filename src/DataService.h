#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include "StockModel.h"
#include "DataProvider.h"

struct FetchTask
{
  std::string symbol;
  float timeRequested;
};

class DataService
{
public:
  DataService(StockModel &model, std::shared_ptr<IDataProvider> provider);
  ~DataService();

  void Start();
  void Stop();

  // UI thread calls this to request data
  void EnqueueFetch(const std::string &symbol, float appTime);

private:
  void WorkerLoop();

  StockModel &m_model;
  std::shared_ptr<IDataProvider> m_provider;

  std::thread m_workerThread;
  std::mutex m_queueMutex;
  std::condition_variable m_cv;
  std::queue<FetchTask> m_queue;
  std::atomic<bool> m_running{false};
};