# BKK Common Utilities

`bkk-common-utils` is the shared utility-library bundle used across display,
network, and setup components. It centralizes small platform capabilities that
would otherwise be duplicated by application recipes.

## Architecture

The top-level CMake build exports separate shared libraries for:

- timing and timer-related helpers (`bkk_utils_timing`)
- online-status checks using libcurl (`bkk_utils_online_status`)
- screen-backlight control (`bkk_screen_backlight`)
- Unix-domain-socket IPC client/server support (`bkk_utils_ipc_uds`)
- D-Bus integration (`bkk-utils-dbus`)

Public headers are installed under `/usr/include/bkk_utils`.

## Integration

`bkk-common-utils_1.0.bb` builds all modules with CMake and depends on curl and
D-Bus. Its libraries are direct dependencies of the screen owner and clients,
the main-content application, network manager, and setup services. Recipe
consumers use its development package when compiling and its runtime package
for the installed shared libraries.