#ifndef EVENTSYSTEM_H
#define EVENTSYSTEM_H

#include "AquilaPCH.h"
#include "Engine/Events/Event.h"
#include "Utilities/Singleton.h"
#include "Utilities/Timer.h"

namespace Engine {

class EventMetrics {
private:
  mutable std::mutex metricsMutex;
  std::unordered_map<std::string, size_t> eventCounts;
  std::unordered_map<std::string, std::chrono::microseconds> totalTimes;
  std::unordered_map<std::string, size_t> errorCounts;

public:
  void RecordEvent(const std::string &eventName,
                   std::chrono::microseconds duration, bool wasError = false) {
    std::lock_guard<std::mutex> lock(metricsMutex);
    eventCounts[eventName]++;
    totalTimes[eventName] += duration;
    if (wasError) {
      errorCounts[eventName]++;
    }
  }

  void RecordEvent(const std::string &eventName, double durationMs,
                   bool wasError = false) {
    auto microDuration =
        std::chrono::microseconds(static_cast<int64_t>(durationMs * 1000));
    RecordEvent(eventName, microDuration, wasError);
  }

  void LogMetrics() const {
    std::lock_guard<std::mutex> lock(metricsMutex);
    AQUILA_LOG_DEBUG("=== Event Metrics ===");
    for (const auto &[name, count] : eventCounts) {
      std::chrono::microseconds avgTime =
          count > 0 ? std::chrono::microseconds(totalTimes.at(name) / count)
                    : std::chrono::microseconds(0);
      auto errors = errorCounts.count(name) ? errorCounts.at(name) : 0;
      AQUILA_LOG_DEBUG("Event {}: {} times, avg {}μs, {} errors", name, count,
                       avgTime.count(), errors);
    }
  }

  void Reset() {
    std::lock_guard<std::mutex> lock(metricsMutex);
    eventCounts.clear();
    totalTimes.clear();
    errorCounts.clear();
  }
};

class EventBus : public Utility::Singleton<EventBus> {
  friend class Utility::Singleton<EventBus>;

private:
  struct EventWrapper {
    Unique<Event> event;
    Utility::TimePoint queueTime;

    EventWrapper(Unique<Event> e)
        : event(std::move(e)), queueTime(Utility::Now()) {}
  };

  std::unordered_map<std::type_index,
                     std::vector<Delegate<void(const Event &)>>>
      handlers;
  mutable std::mutex handlersMutex;

  std::queue<EventWrapper> eventQueue;
  std::mutex queueMutex;
  std::condition_variable queueCV;
  std::thread workerThread;
  std::atomic<bool> running{false};

  Unique<EventMetrics> metrics;
  bool metricsEnabled = false;

public:
  EventBus() : metrics(CreateUnique<EventMetrics>()) {}

  ~EventBus() { Clean(); }

  void Initialize() {
    if (!running.load()) {
      running.store(true);
      workerThread = std::thread(&EventBus::ProcessEventsAsync, this);
    }
  }

  void Clean() {
    if (running.load()) {
      running.store(false);
      queueCV.notify_all();
      if (workerThread.joinable()) {
        workerThread.join();
      }
    }
  }

  void EnableMetrics(bool enable = true) { metricsEnabled = enable; }

  const EventMetrics &GetMetrics() const { return *metrics; }

  template <typename EventType>
  using Handler = Delegate<void(const EventType &)>;

  template <typename EventType>
  void RegisterHandler(Handler<EventType> handler) {
    std::lock_guard<std::mutex> lock(handlersMutex);
    auto type = std::type_index(typeid(EventType));

    auto wrapper = [handler](const Event &event) {
      const auto &typedEvent = static_cast<const EventType &>(event);
      handler(typedEvent);
    };

    handlers[type].push_back(wrapper);
  }

  template <typename EventType>
  EventResult DispatchSync(const EventType &event) {
    auto startTime = Utility::Now();

    auto validationResult = EventValidator<EventType>::Validate(event);
    if (!validationResult.IsSuccess()) {
      if (metricsEnabled) {
        double durationMs =
            Utility::ElapsedMilliseconds(startTime, Utility::Now());
        metrics->RecordEvent(event.GetName(), durationMs, true);
      }
      return validationResult;
    }

    std::lock_guard<std::mutex> lock(handlersMutex);
    auto type = std::type_index(typeid(EventType));
    auto it = handlers.find(type);

    if (it != handlers.end()) {
      for (auto &fn : it->second) {
        try {
          fn(event);
        } catch (const std::exception &e) {
          if (metricsEnabled) {
            double durationMs =
                Utility::ElapsedMilliseconds(startTime, Utility::Now());
            metrics->RecordEvent(event.GetName(), durationMs, true);
          }
          return EventResult::Error(EventResult::Status::InternalError,
                                    "Handler exception: " +
                                        std::string(e.what()));
        }
      }
    }

    if (metricsEnabled) {
      double durationMs =
          Utility::ElapsedMilliseconds(startTime, Utility::Now());
      metrics->RecordEvent(event.GetName(), durationMs, false);
    }

    return EventResult::Success();
  }

  template <typename EventType>
  EventResult DispatchSync(Unique<EventType> event) {
    if (!event) {
      return EventResult::Error(EventResult::Status::InvalidParameters,
                                "Event pointer is null");
    }
    return DispatchSync(*event);
  }

  template <typename EventType>
  std::future<EventResult> DispatchAsync(Unique<EventType> event) {
    auto promise = CreateRef<std::promise<EventResult>>();
    auto future = promise->get_future();

    event->callback = [promise](const EventResult &result) {
      promise->set_value(result);
    };

    {
      std::lock_guard<std::mutex> lock(queueMutex);
      eventQueue.emplace(std::move(event));
    }
    queueCV.notify_one();

    return future;
  }

  template <typename EventType>
  void DispatchAsyncNoResult(Unique<EventType> event) {
    {
      std::lock_guard<std::mutex> lock(queueMutex);
      eventQueue.emplace(std::move(event));
    }
    queueCV.notify_one();
  }

private:
  void ProcessEventsAsync() {
    while (running.load()) {
      std::unique_lock<std::mutex> lock(queueMutex);
      queueCV.wait(lock,
                   [this] { return !eventQueue.empty() || !running.load(); });

      while (!eventQueue.empty() && running.load()) {
        auto wrapper = std::move(eventQueue.front());
        eventQueue.pop();
        lock.unlock();
        double queueWaitMs =
            Utility::ElapsedMilliseconds(wrapper.queueTime, Utility::Now());

        if (queueWaitMs > 100.0) {
          AQUILA_LOG_WARNING("Event {} waited {}ms in queue",
                             wrapper.event->GetName(), queueWaitMs);
        }

        EventResult result;
        auto processingStart = Utility::Now();
        try {
          result = DispatchSyncInternal(*wrapper.event);
        } catch (const std::exception &e) {
          result = EventResult::Error(EventResult::Status::InternalError,
                                      "Async processing error: " +
                                          std::string(e.what()));
        }

        if (metricsEnabled) {
          double processingMs =
              Utility::ElapsedMilliseconds(processingStart, Utility::Now());
          metrics->RecordEvent(wrapper.event->GetName(), processingMs,
                               !result.IsSuccess());
        }

        if (wrapper.event->callback) {
          wrapper.event->callback(result);
        }

        lock.lock();
      }
    }
  }

  EventResult DispatchSyncInternal(const Event &event) {
    auto startTime = Utility::Now();

    std::lock_guard<std::mutex> lock(handlersMutex);
    auto type = std::type_index(typeid(event));
    auto it = handlers.find(type);

    if (it != handlers.end()) {
      for (auto &fn : it->second) {
        fn(event);
      }
    }

    double processingMs =
        Utility::ElapsedMilliseconds(startTime, Utility::Now());
    if (processingMs > 50.0) {
      AQUILA_LOG_WARNING("Slow event processing: {} took {}ms", event.GetName(),
                         processingMs);
    }

    return EventResult::Success();
  }

public:
  void Clear() {
    std::lock_guard<std::mutex> lock(handlersMutex);
    handlers.clear();
  }

  size_t GetQueueSize() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return eventQueue.size();
  }

  void LogPerformanceStats() {
    auto queueSize = GetQueueSize();
    if (queueSize > 0) {
      AQUILA_LOG_DEBUG("EventBus queue size: {} events", queueSize);
    }

    if (metricsEnabled) {
      metrics->LogMetrics();
    }
  }
};

} // namespace Engine

#endif // EVENTSYSTEM_H