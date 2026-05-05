#ifndef HTTP_SERVER_POST_HANDLER_H
#define HTTP_SERVER_POST_HANDLER_H

#include <stddef.h>

int http_server_handle_post(const char *request_text,
                                char **out_buf, size_t *out_len);


#endif // HTTP_SERVER_POST_HANDLER_H