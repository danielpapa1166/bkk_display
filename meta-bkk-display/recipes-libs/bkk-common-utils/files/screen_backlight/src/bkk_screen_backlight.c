#include "bkk_screen_backlight.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>


#define FB_DEV "/dev/fb0"


int set_screen_pwr(int enable) {
  const int framebuffer = open(FB_DEV, O_RDWR | O_CLOEXEC);
  if (framebuffer < 0) {
    return -1;
  }

  const int mode = enable ? FB_BLANK_UNBLANK : FB_BLANK_NORMAL;
  const int result = ioctl(framebuffer, FBIOBLANK, mode);
  close(framebuffer);
  return result;
}