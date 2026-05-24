# chttp Integration Plan

Integrating the [chttp](https://github.com/danielpapa1166/chttp) submodule
(`/data/projects/bkk_display/chttp`) into the config-server, replacing all
locally-defined socket management, HTTP parsing, and request dispatch code.

---

## What chttp Replaces

| Locally-defined concern | Current files | Replaced by |
|---|---|---|
| Socket lifecycle (`socket`, `bind`, `listen`, `accept`) | `http_server_main.c` | `chttp_server_create()` / `chttp_server_run()` |
| Raw socket read + HTTP request-line parsing | `http_server_client_handler.c` | chttp internal parser → `chttp_request_t` |
| Method/path dispatch | `http_server_client_handler.c` | `chttp_server_register_route()` |
| `write_all()` + raw HTTP response assembly | `http_server_client_handler.c`, `http_server_utils.h` (`build_simple_response`) | chttp internal `chttp_send_response()` |
| `fork()`-per-connection concurrency model | `http_server_main.c` | chttp single-threaded blocking accept loop |

---

## Step-by-step Integration

### Step 1 — Delete `http_server_client_handler.c/.h`

These files are entirely replaced by chttp internals. Nothing in them needs to be ported:

- `parse_http_method()` → replaced by chttp's internal parser populating `chttp_request_t`
- `write_all()` → replaced by chttp's internal `chttp_send_response()`
- Method/path dispatch `if/else` → replaced by `chttp_server_register_route()`
- Per-request timing log → can be added inside individual handler callbacks if still needed

---

### Step 2 — Adapt `http_server_utils.h`

Replace `build_simple_response(out_buf, out_len, status, content_type, body)`, which
builds a raw heap-allocated HTTP response buffer, with a helper that fills a
`chttp_response_t` directly:

```c
// Remove build_simple_response entirely. Add:
static void set_simple_response(chttp_response_t *resp,
    const char *status, const char *content_type, const char *body) {
    resp->status       = status;
    resp->content_type = content_type;
    resp->body         = strdup(body);
    resp->body_len     = strlen(body);
}
```

> `chttp_response_t.body` must be heap-allocated — chttp calls `free(resp.body)` after sending.

Keep `get_mime_type()` and `is_safe_request_path()` unchanged.

Add `#include <chttp.h>` to `http_server_utils.h`.

---

### Step 3 — Adapt `http_server_get_handler.c/h`

Change both function signatures from `(request_text, out_buf, out_len, ...)` to
`chttp_handler_fn`:

```c
// OLD
int http_server_handle_get_api(
    const char *request_path, server_mode_t mode,
    char **out_buf, size_t *out_len);

int http_server_handle_resource_request(
    const char *request_text,
    char **out_buf, size_t *out_len);

// NEW
void http_server_handle_get_api(
    const chttp_request_t *req, chttp_response_t *resp, void *user_data);

void http_server_handle_resource_request(
    const chttp_request_t *req, chttp_response_t *resp, void *user_data);
```

Internal changes:

- `req->path` replaces all manual `sscanf` parsing of the request line. chttp
  delivers the path already decoded and with the query string stripped, so the
  `strcspn(request_path, "?")` block in `http_server_handle_resource_request` is
  removed.
- `mode` is obtained by casting `user_data`: `server_mode_t mode = *(server_mode_t *)user_data;`
- All `build_simple_response(out_buf, out_len, ...)` calls become
  `set_simple_response(resp, ...)`.
- In the file-serving success path, replace the raw `HTTP/1.1 200 OK\r\n...` header
  construction with:
  ```c
  resp->status       = "200 OK";
  resp->content_type = mime_type;
  resp->body         = buf;       /* heap-allocated file buffer */
  resp->body_len     = file_size;
  ```

---

### Step 4 — Adapt `http_server_post_handler.c/h`

Split the single dispatcher `http_server_handle_post` into two discrete
`chttp_handler_fn` functions, one per registered route:

```c
// OLD: one function with internal routing table
int http_server_handle_post(const char *request_text,
                             char **out_buf, size_t *out_len,
                             server_mode_t mode);

// NEW: two chttp_handler_fn callbacks
void http_server_handle_button_post(
    const chttp_request_t *req, chttp_response_t *resp, void *user_data);

void http_server_handle_finish_post(
    const chttp_request_t *req, chttp_response_t *resp, void *user_data);
```

Internal changes:

- Remove `extract_http_json_body()` entirely — the request body is available
  directly as `req->body` (a null-terminated string) with length `req->body_len`.
- Remove the internal `post_handler_table` and the `sscanf` request-line parse —
  routing is now handled externally by chttp.
- `mode` comes from `user_data` as above.
- All `build_simple_response` calls become `set_simple_response`.

---

### Step 5 — Rewrite `http_server_main.c`

Replace the entire manual socket + `fork` loop with chttp lifecycle calls.

Because chttp uses **exact path matching**, register all known static file paths
explicitly (there are four: `/`, `/index.html`, `/styles.css`, `/app.js`):

```c
#include <chttp.h>
#include "http_server_get_handler.h"
#include "http_server_post_handler.h"
#include "http_server_config.h"
#include "http_server_logger.h"

#define PORT 8080

int main(int argc, char *argv[])
{
    init_logger();

    server_mode_t mode = SERVER_MODE_WIFI;
    if (parse_args(argc, argv, &mode) != 0) {
        log_error("main", "Failed to parse arguments. Exiting.");
        return 1;
    }

    chttp_server_t *srv = chttp_server_create(PORT);
    if (!srv) {
        log_error("main", "Failed to create chttp server.");
        return 1;
    }

    chttp_server_register_route(srv, "GET",  "/api/mode",   http_server_handle_get_api,          &mode);
    chttp_server_register_route(srv, "GET",  "/",           http_server_handle_resource_request, NULL);
    chttp_server_register_route(srv, "GET",  "/index.html", http_server_handle_resource_request, NULL);
    chttp_server_register_route(srv, "GET",  "/styles.css", http_server_handle_resource_request, NULL);
    chttp_server_register_route(srv, "GET",  "/app.js",     http_server_handle_resource_request, NULL);
    chttp_server_register_route(srv, "POST", "/api/button", http_server_handle_button_post,      &mode);
    chttp_server_register_route(srv, "POST", "/api/finish", http_server_handle_finish_post,      &mode);

    log_info("main", "HTTP server running.");
    chttp_server_run(srv);   /* blocking */

    chttp_server_destroy(srv);
    cleanup_logger();
    return 0;
}
```

Remove: `socket()`, `bind()`, `listen()`, `accept()`, `fork()`, `close(listen_fd)`,
`server_addr` static variable, and the `while(1)` accept loop.

---

### Step 6 — Update `CMakeLists.txt`

```cmake
# ---------------------------------------------------------------------------
# chttp static library (submodule)
# ---------------------------------------------------------------------------
set(CHTTP_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../chttp"
    CACHE PATH "Path to the chttp submodule")

add_subdirectory("${CHTTP_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/chttp")

# ---------------------------------------------------------------------------
# config-server executable
# ---------------------------------------------------------------------------
add_executable(config-server
    http_server_main.c
    # http_server_client_handler.c  ← REMOVED
    http_server_get_handler.c
    http_server_post_handler.c
    http_server_user_action_handler.c
    http_server_wifi_validation.c
    http_server_logger.c
)

target_include_directories(config-server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CJSON_INCLUDE_DIR}
    # chttp exposes include/ as PUBLIC, no extra path needed
)

target_link_libraries(config-server PRIVATE
    chttp                # ← ADD
    cjson
    rbuflogd_producer
)
```

---

### Step 7 — Update the Yocto recipe `config-server_0.1.bb`

Two options:

**Option A — Dedicated chttp recipe (preferred)**

Create `meta-bkk-setup/recipes-setup/chttp/chttp_git.bb` (or in an appropriate
layer) that fetches and builds chttp from the submodule, installing it as a static
library. Then add `chttp` to `DEPENDS` in `config-server_0.1.bb`.

**Option B — Embed sources in the recipe**

Copy the chttp `src/` and `include/` trees into `files/chttp/`, add them to
`SRC_URI`, and update `CMakeLists.txt` to reference them at the local path. This
avoids a second recipe at the cost of duplicating sources.

In either case, remove the following two entries from `SRC_URI`:

```
file://src/http_server_client_handler.c
file://src/http_server_client_handler.h
```

---

## Concurrency Model Change

| | Before | After |
|---|---|---|
| Model | `fork()` per connection; child handles one request and exits | Single-threaded blocking loop; one connection fully handled before next `accept()` |
| Isolation | Each request runs in its own process | All requests share one process |
| Suitability | General-purpose | Acceptable for the low-traffic setup-wizard use case |

Confirm this trade-off is acceptable before implementation.
