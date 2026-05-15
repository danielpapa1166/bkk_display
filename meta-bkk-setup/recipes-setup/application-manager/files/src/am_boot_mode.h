#ifndef AM_BOOT_MODE_H
#define AM_BOOT_MODE_H

typedef enum {
  BOOT_MODE_UNDEFINED,
  BOOT_MODE_WIFI_CONFIG, 
  BOOT_MODE_API_CONFIG, 
  BOOT_MODE_NORMAL
} boot_mode_t;


boot_mode_t determine_boot_mode(void);

#endif // AM_BOOT_MODE_H