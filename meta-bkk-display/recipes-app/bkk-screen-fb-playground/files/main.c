#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>


static int set_screen_pwr(int enable) {
  const int framebuffer = open("/dev/fb0", O_RDWR | O_CLOEXEC);
  if (framebuffer < 0) {
    return -1;
  }

  const int mode = enable ? FB_BLANK_UNBLANK : FB_BLANK_POWERDOWN;
  const int result = ioctl(framebuffer, FBIOBLANK, mode);
  close(framebuffer);
  return result;
}


int main()
{

  while(1) {
    set_screen_pwr(1);
    sleep(5);
    set_screen_pwr(0);
    sleep(5);
  }

  int fbfd;
  char *fbp;
  struct fb_var_screeninfo vinfo;
  struct fb_fix_screeninfo finfo;

  /* Open the file for reading and writing. */
  fbfd = open("/dev/fb0", O_RDWR);
  if (fbfd == -1)
  {
    perror("Failed to open framebuffer device");
    exit(1);
  }
  printf("The framebuffer device was opened successfully.\n");

  /* Get fixed screen information. */
  if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1)
  {
    close(fbfd);
    perror("Failed to read fixed information");
    exit(2);
  }

  /* Get variable screen information. */
  if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1)
  {
    close(fbfd);
    perror("Failed to read variable information");
    exit(3);
  }

  printf("%dx%d, %d bpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
  printf("rotate=%d\n", vinfo.rotate);
  printf("activate=%d\n", vinfo.activate);

  /* Figure out the size of the screen in bytes. */
  long int screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

  /* Map the device to memory. */
  fbp = (char *) mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
  if ((long) fbp == -1)
  {
    close(fbfd);
    perror("Failed to map framebuffer device to memory");
    exit(4);
  }
  printf("The framebuffer device was mapped to memory successfully.\n");

  /* Paint a pretty screen. */
  unsigned int x, y;
  long int location;

  const int sq_size = 16; 

  while(1) {
    for (y = 0; y < vinfo.yres; y += sq_size)
    {
      for (x = 0; x < vinfo.xres; x += sq_size)
      {
        int rd = rand() % 2; 
        
        for (int dy = 0; dy < sq_size && (y + dy) < vinfo.yres; ++dy) {
          for (int dx = 0; dx < sq_size && (x + dx) < vinfo.xres; ++dx) {
            location = ((x + dx + vinfo.xoffset) * (vinfo.bits_per_pixel / 8)) +
                       ((y + dy + vinfo.yoffset) * finfo.line_length);

            int r = (rd == 0) ? 6 : 0;  
            int g = (rd == 0) ? 2 : 0;    
            int b = (rd == 0) ? 4 : 0;       
            unsigned short int t = r << 11 | g << 5 | b;
            *((unsigned short int*)(fbp + location)) = t;
          }
        }
      }
    }
    usleep(100000);
  }
  
  printf("The framebuffer device was painted successfully.\n");

  /* Close memory mapped and file descriptor. */
  munmap(fbp, screensize);
  close(fbfd);

  return 0;
}