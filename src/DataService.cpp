#include "DataService.h"
#include "Logger.h"
#include <nlohmann/json.hpp>

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

    // 1. Initialize OS Socket Networking (Required for Windows!)
    ix::initNetSystem();

    std::string apiKey = ConfigManager::GetInstance().GetSettings().apiKey;
    if (!apiKey.empty())
    {
      m_webSocket.setUrl("wss://ws.finnhub.io?token=" + apiKey);

      // 2. Define how to handle incoming Live Trades
      m_webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr &msg)
                                       {
            if (msg->type == ix::WebSocketMessageType::Open) {
                Logger::GetInstance().Log("[WEBSOCKET] Connected to Finnhub Live Stream.");
                
                // Automatically resubscribe to all active tickers on connect
                auto active = ConfigManager::GetInstance().GetSettings().activeTickers;
                for (const auto& t : active) {
                    this->Subscribe(t);
                }
            } else if (msg->type == ix::WebSocketMessageType::Message) {
                try {
                    auto j = nlohmann::json::parse(msg->str);
                    if (j["type"] == "trade") {
                        for (const auto& trade : j["data"]) {
                            std::string sym = trade["s"];
                            double price = trade["p"];
                            double time = trade["t"].get<double>() / 1000.0; // Convert UNIX ms to seconds
                            m_model.AddPrice(sym, price, time);
                        }
                    }
                } catch(...) {}
            } else if (msg->type == ix::WebSocketMessageType::Error) {
                Logger::GetInstance().Log("[WEBSOCKET ERROR] " + msg->errorInfo.reason);
            } });
      m_webSocket.start();
    }

    m_workerThread = std::thread(&DataService::WorkerLoop, this);
  }
}

void DataService::Stop()
{
  if (m_running)
  {
    m_running = false;
    m_webSocket.stop();
    ix::uninitNetSystem();
    m_cv.notify_all();
    if (m_workerThread.joinable())
      m_workerThread.join();
  }
}

void DataService::Subscribe(const std::string &symbol)
{
  nlohmann::json j = {{"type", "subscribe"}, {"symbol", symbol}};
  m_webSocket.send(j.dump());
}

void DataService::Unsubscribe(const std::string &symbol)
{
  nlohmann::json j = {{"type", "unsubscribe"}, {"symbol", symbol}};
  m_webSocket.send(j.dump());
}

void DataService::EnqueueFetch(const std::string &symbol, double appTime, TaskType type)
{
  {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_queue.push({symbol, appTime, type});
  }
  m_cv.notify_one();
}
void DataService::WorkerLoop()
{
  while (m_running)
  {
    FetchTask task;
    {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      m_cv.wait(lock, [this]()
                { return !m_queue.empty() || !m_running; });
      if (!m_running && m_queue.empty())
        break;
      task = m_queue.front();
      m_queue.pop();
    }

    if (m_provider->CanFetch())
    {
      // --- THE WORKER LOOP IS NOW STRICTLY FOR HISTORICAL BACKFILLING ---
      if (task.type == TaskType::History)
      {
        std::string res = ConfigManager::GetInstance().GetSettings().historyResolution;
        Logger::GetInstance().Log("[NETWORK] Fetching history for " + task.symbol + " (Res: " + res + ")...");

        long long toTime = static_cast<long long>(std::chrono::system_clock::now().time_since_epoch() / std::chrono::seconds(1));
        long long fromTime = toTime - (86400 * 7);

        HistoryChunk chunk = m_provider->FetchHistory(task.symbol, fromTime, toTime, res);
        if (!chunk.prices.empty())
        {
          m_model.SetApiError(false);
          for (size_t i = 0; i < chunk.prices.size(); ++i)
          {
            m_model.AddPrice(task.symbol, chunk.prices[i], chunk.timestamps[i]);
          }
          Logger::GetInstance().Log("[SYSTEM] Loaded " + std::to_string(chunk.prices.size()) + " historical points.");
        }
        else
        {
          Logger::GetInstance().Log("[WARNING] History denied for " + task.symbol + ".");
        }
      }
    }
    else
    {
      Logger::GetInstance().Log("[NETWORK] Rate limit hit. Waiting for REST token...");
      {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_queue.push(task);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}