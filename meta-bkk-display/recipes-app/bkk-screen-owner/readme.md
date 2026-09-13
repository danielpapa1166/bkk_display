# Screen Owner

`bkk-screen-owner` is the primary Qt display process. It owns the screen,
processes touch input, controls the backlight, and renders the UI. Other
screen-related applications request updates from it instead of competing for
direct display access.

## Architecture

The component builds two artifacts:

- `bkk-screen-owner`: the Qt Widgets executable in `files/owner`. Its request
  handlers update information-bar, component, table, and status-screen views;
  its touch handler consumes ADS7846 controller input.
- `libbkk-screen-client.so`: the shared client API in `files/client`. It
  communicates with the owner through the common IPC UDS support and exposes
  installed headers under `bkk_screen_client/` for display companions.

This split establishes a single rendering owner with IPC clients such as the
main-content and info-bar applications.

## Integration

The recipe inherits `cmake_qt5` and builds against Qt5, the BKK API client,
ADS7846 controller, rbuflogd, cJSON, BKK TEE, and common utilities. It installs
the executable, client library, and development headers. It is a dependency of
the screen-content packages; application-manager controls its runtime launch
rather than a dedicated service unit.