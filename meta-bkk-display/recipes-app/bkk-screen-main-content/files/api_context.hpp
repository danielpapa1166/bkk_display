#ifndef API_CONTEXT_HPP
#define API_CONTEXT_HPP

#include <string>
#include <vector>
#include <pthread.h>
typedef struct {
  pthread_mutex_t mutex;
  std::string api_key;
  std::vector<std::string> station_id_list;
  std::vector<std::string> station_name_list;
} api_fetch_context_t;


int init_api_context();
int load_api_context();
std::string get_api_key(void);
std::vector<std::string> get_station_id_list(void);
std::vector<std::string> get_station_name_list(void);


#endif // API_CONTEXT_HPP