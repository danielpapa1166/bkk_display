#ifndef HTTP_SERVER_RESOURCE_HANDLER_H
#define HTTP_SERVER_RESOURCE_HANDLER_H

#include <stddef.h>

/* Build the HTTP response for a resource request.
 * On success, sets *out_buf to a malloc'd buffer containing the full response,
 * sets *out_len to its byte length, and returns 0. The caller must free(*out_buf).
 * On failure returns -1 and *out_buf is NULL. */
int http_server_handle_resource_request(const char *request_text,
                                        char **out_buf, size_t *out_len);

#endif /* HTTP_SERVER_RESOURCE_HANDLER_H */