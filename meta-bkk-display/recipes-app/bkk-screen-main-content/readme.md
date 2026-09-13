# Screen Main Content

`bkk-screen-main-content` supplies the dynamic public-transport content shown
by the display. It obtains arrival data through the BKK API client, converts it
into display-facing state, and submits screen updates through the screen-owner
client library.

## Architecture

The executable has three focused layers:

- `content_main.cpp` owns process startup and the update flow.
- `bkk_api_client.*` adapts the BKK UDS client library to the content module.
- `api_context.*` and `screen_context.*` hold the API and display state needed
  between updates.

This keeps transport-data retrieval separate from screen requests while the
screen owner remains the only process that renders the Qt UI.

## Integration

`bkk-screen-main-content.bb` builds and installs
/usr/bin/bkk_screen_main_content. Its build dependencies provide the screen
client, BKK API client, TEE client, logging, timing, D-Bus utilities, and
network-management support. Runtime dependencies include the screen owner,
BKK API daemon/client packages, rbuflogd, TEE client, and common utilities.
The application manager starts it during the normal device phase.