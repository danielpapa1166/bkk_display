# Add display and touchscreen settings to the Raspberry Pi /boot/config.txt.
RPI_EXTRA_CONFIG:append:raspberrypi4 = "\
\n\
hdmi_group=2\n\
hdmi_mode=87\n\
hdmi_cvt 800 480 60 6 0 0 0\n\
hdmi_drive=1\n\
dtoverlay=ads7846,\
cs=1,penirq=25,penirq_pull=2,\
speed=50000,keep_vref_on=0,swapxy=0,\
pmax=255,xohms=150,\
xmin=200,xmax=3900,ymin=200,ymax=3900\n\
"
