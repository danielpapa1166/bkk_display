# Add display and touchscreen settings to the Raspberry Pi /boot/config.txt.
RPI_EXTRA_CONFIG:append:raspberrypi4 = "\
\n\
hdmi_group=2\n\
hdmi_mode=87\n\
hdmi_cvt 800 480 60 6 0 0 0\n\
hdmi_drive=1\n\
dtparam=spi=on\n\
"
