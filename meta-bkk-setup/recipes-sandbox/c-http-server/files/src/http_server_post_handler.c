#include "http_server_post_handler.h"
#include "http_server_utils.h"




typedef int (http_post_handler_fn)(void); 

typedef struct {
  char request_path[256];
  http_post_handler_fn *handler_fn; 
} post_handler_table_entry_t;


static int handle_button_api_post(void) {
  printf("Button API POST handler called!\n");
  return 0;
}

static post_handler_table_entry_t post_handler_table[] = {
    { .request_path = "/api/button", 
          .handler_fn = (http_post_handler_fn *)&handle_button_api_post 
        }, 
}; 

int http_server_handle_post(const char *request_text,
    char **out_buf, size_t *out_len) {

  char method[8] = { 0 };
  char request_path[256] = { 0 };
  char http_version[16] = { 0 };

  if (sscanf(request_text, "%7s %255s %15s", method, request_path, http_version) != 3) {
    return build_simple_response(out_buf, out_len,
      "400 Bad Request",
      "text/plain; charset=utf-8",
      "Bad Request\n");
  }

  printf("Handling POST request for path: %s\n", request_path);

  for (size_t i = 0; i < sizeof(post_handler_table) / sizeof(post_handler_table_entry_t); i++) {
    if (strcmp(request_path, post_handler_table[i].request_path) == 0) {
      int handler_result = post_handler_table[i].handler_fn();
      if (handler_result != 0) {
        return build_simple_response(out_buf, out_len,
          "500 Internal Server Error", 
          "text/plain; charset=utf-8",         
          "Internal Server Error\n");
       }
       return build_simple_response(out_buf, out_len,
        "200 OK",
        "application/json; charset=utf-8",
        "{\"status\":\"ok\",\"message\":\"button handled\"}\n");
    }
  }

  return build_simple_response(out_buf, out_len,
    "501 Not Implemented",
    "text/plain; charset=utf-8",
    "Not Implemented\n");
}