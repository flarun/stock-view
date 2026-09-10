#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include "StockModel.h"
#include "DataProvider.h"
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

enum class TaskType
{
  Live,
  History
};
struct FetchTask
{
  std::string symbol;
  double timeRequested;
  TaskType type = TaskType::Live;
};

class DataService
{
public:
  DataService(StockModel &model, std::shared_ptr<IDataProvider> provider);
  ~DataService();

  void Start();
  void Stop();

  // UI thread calls this to request data
  // Updated to accept a task type
  void EnqueueFetch(const std::string &symbol, double appTime, TaskType type = TaskType::Live);

  void Subscribe(const std::string &symbol);
  void Unsubscribe(const std::string &symbol);

private:
  void WorkerLoop();

  StockModel &m_model;
  std::shared_ptr<IDataProvider> m_provider;

  ix::WebSocket m_webSocket;

  std::thread m_workerThread;
  std::mutex m_queueMutex;
  std::condition_variable m_cv;
  std::queue<FetchTask> m_queue;
  std::atomic<bool> m_running{false};
};