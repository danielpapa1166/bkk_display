#ifndef HTTP_SERVER_RESOURCE_HANDLER_H
#define HTTP_SERVER_RESOURCE_HANDLER_H

void http_server_handle_resource_request(int client_fd, const char *request_text);

#endif /* HTTP_SERVER_RESOURCE_HANDLER_H */