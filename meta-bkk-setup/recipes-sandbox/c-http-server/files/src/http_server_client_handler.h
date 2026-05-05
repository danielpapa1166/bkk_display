#ifndef HTTP_SERVER_CLIENT_HANDLER_H
#define HTTP_SERVER_CLIENT_HANDLER_H

#include "rbuflogd/producer.h"
void client_handler(int client_fd, rbuflogd_producer_t *logger_producer);

#endif /* HTTP_SERVER_CLIENT_HANDLER_H */