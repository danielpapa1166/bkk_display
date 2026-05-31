# config-server

Minimal HTTP server written in C that guides the user through initial device
setup. Runs on port 8080 and is managed by systemd. Once setup is complete
it does not run again.

---

## Phases

Boot mode is determined at startup by `bkk-boot-mode.sh` based on two flag
files under `/etc/bkk-display-config/`:

| Flag file | Meaning |
|---|---|
| _(neither exists)_ | Phase 1 — WiFi not yet configured |
| `wifi-configured` | Phase 2 — WiFi done, API not yet configured |
| `api-configured` | Normal operation — setup complete, server does not start |

### Phase 1 — WiFi config (`--mode=wifi`)

The device starts a local WiFi access point. The user connects to it and
opens the setup page at `http://<ap-ip>:8080`. The user enters SSID and
password; on submit the server validates the credentials, writes a
`wpa_supplicant` config, and creates the `wifi-configured` flag. On the
next boot the device connects to the configured network.

Service: `bkk-setup-web.service`

### Phase 2 — API config (`--mode=api`)

The device is connected to the LAN. The user opens the setup page from the
same network. The user enters the BKK API key and selects stop IDs. On
finish the server writes the config and creates the `api-configured` flag.
Normal operation begins on the next boot.

Service: `bkk-setup-api.service`

### Normal mode

Neither service starts. The display application runs directly.

---

## HTTP Routes

| Method | Path | Handler | Notes |
|---|---|---|---|
| `GET` | `/` | static file | redirects to `index.html` |
| `GET` | `/index.html` | static file | setup UI |
| `GET` | `/styles.css` | static file | |
| `GET` | `/app.js` | static file | |
| `GET` | `/api/mode` | `http_server_handle_get_api` | returns `{"mode":"wifi"}` or `{"mode":"api"}` |
| `POST` | `/api/button` | `http_server_handle_button_post` | page navigation + data submission |
| `POST` | `/api/finish` | `http_server_handle_finish_post` | signals setup complete |

Static files are served from `/usr/share/config-server/www/`.

---

## POST body (`/api/button`)

```json
{
  "action":       "next" | "back",
  "from_page":    "wifi" | "api-key" | "stations",
  "to_page":      "<page name>",
  "wifi_ssid":    "<ssid>",
  "wifi_password":"<password>",
  "api_key":      "<key>",
  "station_ids":  "<ids>"
}
```

Fields other than `action`, `from_page`, and `to_page` are optional and
only read when relevant to the current page.

---

## Dependencies

| Library | Role |
|---|---|
| `chttp` | HTTP server (socket, parsing, routing, response) |
| `cjson` | JSON body parsing |
| `rbuflogd_producer` | Structured logging |
