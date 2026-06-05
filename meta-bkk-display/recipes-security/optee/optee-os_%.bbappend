# Allow optee-os to build for raspberrypi4-64 and set the correct OP-TEE platform name.
# The default COMPATIBLE_MACHINE in meta-arm is "invalid" - we must whitelist our machine.
# OP-TEE 3.16.0 has no plat-rpi4; the RPi4 is covered by plat-rpi3 (same ARMv8 core).

COMPATIBLE_MACHINE:raspberrypi4-64 = "raspberrypi4-64"
OPTEEMACHINE:raspberrypi4-64 = "rpi3"
