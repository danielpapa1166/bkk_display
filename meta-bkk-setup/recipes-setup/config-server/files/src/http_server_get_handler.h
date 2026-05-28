#ifndef HTTP_SERVER_GET_HANDLER_H
#define HTTP_SERVER_GET_HANDLER_H

#include <chttp.h>

void http_server_handle_resource_request(
    const chttp_request_t *req, chttp_response_t *resp, void *user_data);

void http_server_handle_get_api(
    const chttp_request_t *req, chttp_response_t *resp, void *user_data);

#endif /* HTTP_SERVER_GET_HANDLER_H */