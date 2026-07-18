#ifndef BKK_UTILS_ONLINE_STATUS_H
#define BKK_UTILS_ONLINE_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
  ONLINE_STATUS_UNKNOWN = 0,
  ONLINE_STATUS_OFFLINE,
  ONLINE_STATUS_ONLINE,
} online_status_t;

online_status_t is_online();


#ifdef __cplusplus
}
#endif

#endif // BKK_UTILS_ONLINE_STATUS_H