# Screen Info Bar

`bkk-screen-info-bar` is the display companion responsible for periodically
updating shared information-bar state, including the clock and online status.
It publishes its updates through the screen-owner client interface rather than
drawing a separate UI itself.

## Integration

The recipe builds `bkk_screen_info_bar` from the clock and connectivity update
modules. It links to `bkk-screen-client` from `bkk-screen-owner`,
`bkk-common-utils` timing support, libcurl, and the rbuflogd producer library.
At runtime it is normally launched by the application-manager configuration;
the recipe does not register a systemd service of its own.