#ifndef HTTP_SERVER_CONFIG_H
#define HTTP_SERVER_CONFIG_H

typedef enum {
    SERVER_MODE_WIFI = 0,   /* Phase 1: AP up, user configures WiFi credentials. */
    SERVER_MODE_API         /* Phase 2: LAN up, user configures BKK API key + stations. */
} server_mode_t;

#endif /* HTTP_SERVER_CONFIG_H */