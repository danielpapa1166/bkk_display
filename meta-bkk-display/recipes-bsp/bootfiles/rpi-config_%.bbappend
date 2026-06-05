# Add display and touchscreen settings to the Raspberry Pi /boot/config.txt.
RPI_EXTRA_CONFIG:append:raspberrypi4 = "\
\n\
hdmi_group=2\n\
hdmi_mode=87\n\
hdmi_cvt 800 480 60 6 0 0 0\n\
hdmi_drive=1\n\
dtparam=spi=on\n\
"

# OP-TEE boot chain for raspberrypi4-64:
# Replace the default armstub with TF-A (bl31.bin), which in turn loads
# OP-TEE OS (BL32) and U-Boot (BL33) before handing off to Linux.
RPI_EXTRA_CONFIG:append:raspberrypi4-64 = "\
\n\
armstub=bl31.bin\n\
enable_uart=1\n\
arm_64bit=1\n\
dtoverlay=optee-smc\n\
"
