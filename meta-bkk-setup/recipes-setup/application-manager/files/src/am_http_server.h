#ifndef AM_HTTP_SERVER_H
#define AM_HTTP_SERVER_H

#include <stdint.h>
#include "am_types.h"

/* Start the HTTP status server on the given port in a dedicated thread.
 * app_info_list must remain valid for the lifetime of the server.
 * www_dir is the path to the directory containing static web resources
 * (index.html, style.css, app.js); may be NULL to disable static serving.
 * Returns 0 on success, non-zero on failure. */
int  am_http_server_start(const app_info_list_t *app_info_list,
                          uint16_t port,
                          const char *www_dir);

/* Signal the server to stop and wait for its thread to exit. */
void am_http_server_stop(void);

#endif /* AM_HTTP_SERVER_H */
