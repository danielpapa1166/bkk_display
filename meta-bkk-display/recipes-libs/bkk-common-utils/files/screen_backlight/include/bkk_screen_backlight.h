#ifndef BKK_SCREEN_BACKLIGHT_H
#define BKK_SCREEN_BACKLIGHT_H


int set_screen_pwr(int enable);

#define SCREEN_PWR_ON()         set_screen_pwr(1)
#define SCREEN_PWR_OFF()        set_screen_pwr(0)

#endif /* BKK_SCREEN_BACKLIGHT_H */