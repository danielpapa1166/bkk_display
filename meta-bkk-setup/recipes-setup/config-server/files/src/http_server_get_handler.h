#ifndef HTTP_SERVER_GET_HANDLER_H
#define HTTP_SERVER_GET_HANDLER_H

#include <stddef.h>
#include "http_server_config.h"

int http_server_handle_resource_request(
    const char *request_text, char **out_buf, size_t *out_len);

int http_server_handle_get_api(
    const char *request_path, server_mode_t mode,
    char **out_buf, size_t *out_len);

#endif /* HTTP_SERVER_GET_HANDLER_H */