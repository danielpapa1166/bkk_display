#ifndef BKK_UTILS_ONLINE_STATUS_H
#define BKK_UTILS_ONLINE_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <arpa/inet.h>


typedef enum {
  ONLINE_STATUS_UNKNOWN = 0,
  ONLINE_STATUS_OFFLINE,
  ONLINE_STATUS_ONLINE,
} online_status_t;

online_status_t is_online();

typedef enum {
  IP_ADD_STATUS_HAS_IP,
  IP_ADD_STATUS_NO_IP,
  IP_ADD_STATUS_UNKNOWN,
} ip_add_status_t;

ip_add_status_t fetch_ip_addr(const char *interface_name, 
    char address[INET_ADDRSTRLEN]);


#ifdef __cplusplus
}
#endif

#endif // BKK_UTILS_ONLINE_STATUS_H