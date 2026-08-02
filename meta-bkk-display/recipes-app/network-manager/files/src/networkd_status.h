#ifndef NETWORKD_STATUS_H
#define NETWORKD_STATUS_H
#include <stdint.h>

int networkd_check_status(const uint32_t timeout_s);
int reload_networkd_config();

#endif /* NETWORKD_STATUS_H */