#ifndef BKK_API_WORKER_HPP
#define BKK_API_WORKER_HPP

#include <QMutex>
#include <QThread>

#include <atomic>
#include <bkk_api/bkk_api.hpp>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>


enum class BkkApiError {
  None = 0,
  ApiUninitialized, 
  InitializationFailed,
  FetchFailed
};

struct StationArrival {
  Arrival arrival;
  std::string station_id;
  std::string station_name;
};


class BkkApiWorker : public QThread {
  Q_OBJECT
public:
  explicit BkkApiWorker(QObject *parent = nullptr);

  // request the worker to perform a fetch as soon as possible:
  void requestFetch(); 
  
  // data getters: 
  std::vector<StationArrival> getArrivals() const;
  uint64_t getLastFetchDurationMs() const;
  BkkApiError getErrorCode() const;


signals: 
  // signals for the parent: 
  void fetchCompleted();

protected: 
  // parent class method override:
  void run() override;


private:
  // internal helper methods:
  bool ensureApi();
  void fetchData();

  void loadStationList(); 

  std::vector<std::string> stationIdList;

  // BKK API interface: 
  std::unique_ptr<BkkApi> api = nullptr;

  // fetch results and sync: 
  mutable std::mutex arrivalsMutex;
  std::vector<StationArrival> arrivals;
  uint64_t lastFetchDurationMs;
  BkkApiError errorCode;

  // fetch request flag:
  std::atomic<bool> fetchRequested = false; 
}; 


#endif // BKK_API_WORKER_HPP