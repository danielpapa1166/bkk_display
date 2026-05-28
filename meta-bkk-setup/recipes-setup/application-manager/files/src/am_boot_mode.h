#ifndef AM_BOOT_MODE_H
#define AM_BOOT_MODE_H

#include "am_types.h"
boot_mode_t get_boot_mode();
boot_mode_t determine_boot_mode(const char * flags_dir);

#endif // AM_BOOT_MODE_H