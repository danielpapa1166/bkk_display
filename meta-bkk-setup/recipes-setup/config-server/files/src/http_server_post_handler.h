#ifndef HTTP_SERVER_POST_HANDLER_H
#define HTTP_SERVER_POST_HANDLER_H

#include <chttp.h>

void http_server_handle_button_post(
    const chttp_request_t *req, chttp_response_t *resp, void *user_data);

void http_server_handle_finish_post(
    const chttp_request_t *req, chttp_response_t *resp, void *user_data);

#endif // HTTP_SERVER_POST_HANDLER_H