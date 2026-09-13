# ADS7846 Demo

`ads7846-demo` is a command-line diagnostic for the ADS7846 touch controller.
It subscribes to touch events through the `ads7846-controller` library and
prints them, providing a simple way to verify touchscreen wiring and driver
behavior on a target device.

## Integration

The `ads7846-demo_1.0.bb` recipe builds the C source with CMake and depends on
`ads7846-controller`. The resulting `ads7846_demo` executable is installed in
`/usr/bin`. It is a manual diagnostic tool, not a system service and not part
of the normal display startup path.