# ADS7846 Controller

`ads7846-controller` is a shared userspace library for the ADS7846 touchscreen
controller. It encapsulates SPI access and interrupt-driven callback handling,
so display code does not need to manage the device protocol directly.

## Integration

The recipe builds `libads7846_controller.so` from the C implementation and
installs its versioned library, public header under
`/usr/include/ads7846_controller`, and pkg-config metadata. It only requires
thread support. `bkk-screen-owner` links it to process user touch input, while
`ads7846-demo` uses it as a hardware diagnostic client.