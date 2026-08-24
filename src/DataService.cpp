#include "DataService.h"
#include "Logger.h"

DataService::DataService(StockModel &model, std::shared_ptr<IDataProvider> provider)
    : m_model(model), m_provider(provider) {}

DataService::~DataService()
{
  Stop();
}

void DataService::Start()
{
  if (!m_running)
  {
    m_running = true;
    m_workerThread = std::thread(&DataService::WorkerLoop, this);
  }
}

void DataService::Stop()
{
  if (m_running)
  {
    m_running = false;
    m_cv.notify_all(); // Wake up the thread so it can exit
    if (m_workerThread.joinable())
    {
      m_workerThread.join();
    }
  }
}

void DataService::EnqueueFetch(const std::string &symbol, double appTime)
{
  {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_queue.push({symbol, appTime});
  }
  m_cv.notify_one(); // Tell the worker thread a new task has arrived
}

void DataService::WorkerLoop()
{
  while (m_running)
  {
    FetchTask task;

    // 1. Wait for a task to enter the queue
    {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      m_cv.wait(lock, [this]()
                { return !m_queue.empty() || !m_running; });

      if (!m_running && m_queue.empty())
        break;

      task = m_queue.front();
      m_queue.pop();
    }

    // 2. Check the Rate Limiter (Token Bucket)
    if (m_provider->CanFetch())
    {
      // --- LOG INJECTION 1: Tell the console we are fetching ---
      Logger::GetInstance().Log("[NETWORK] Fetching data for " + task.symbol + "...");

      // Token acquired! Fetch the data.
      double price = m_provider->FetchPrice(task.symbol);
      if (price > 0)
      {
        m_model.SetApiError(false); // Success! Lower the flag.
        m_model.AddPrice(task.symbol, price, task.timeRequested);
      }
      else
      {
        m_model.SetApiError(true); // Failed! Raise the error flag.

        // --- LOG INJECTION 2: Tell the console the API rejected it ---
        Logger::GetInstance().Log("[ERROR] API rejected request for " + task.symbol + ". Invalid key?");
      }
    }
    else
    {
      // --- LOG INJECTION 3: Tell the console we are rate limited ---
      Logger::GetInstance().Log("[NETWORK] Rate limit hit. Waiting for token...");

      // 3. Rate Limit hit! Put the task back at the front and wait briefly
      {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        // Push back doesn't strictly preserve perfect order for dropped packets,
        // but guarantees no data tasks are lost due to API limits.
        m_queue.push(task);
      }
      // Sleep for 100ms so we don't spam the CPU while waiting for a token
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}