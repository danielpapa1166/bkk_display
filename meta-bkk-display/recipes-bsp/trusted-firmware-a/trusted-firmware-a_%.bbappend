# TF-A (Trusted Firmware-A) for Raspberry Pi 4 (AArch64)
# TF-A acts as the Secure Monitor (EL3): loads OP-TEE as BL32, U-Boot as BL33.
#
# TFA_SPD = "opteed"  tells TF-A to load OP-TEE OS as the Secure Payload Dispatcher.
# TFA_UBOOT = "1"     tells the recipe to embed u-boot.bin as BL33 (normal world entry).
# The output bl31.bin is placed in DEPLOYDIR and must be included in the SD card image
# as the ARMSTUB replacement (set via ARMSTUB in config.txt).

COMPATIBLE_MACHINE:raspberrypi4-64 = "raspberrypi4-64"
TFA_PLATFORM:raspberrypi4-64 = "rpi4"
TFA_SPD:raspberrypi4-64 = "opteed"
TFA_BUILD_TARGET:raspberrypi4-64 = "all"
TFA_INSTALL_TARGET:raspberrypi4-64 = "bl31.bin"

# Pass OP-TEE OS binaries as BL32 so TF-A embeds them into the secure world.
# tee-header_v2.bin is the main paged binary; EXTRA1/EXTRA2 are the pager
# and pageable sections respectively.
EXTRA_OEMAKE:append:raspberrypi4-64 = " \
    BL32=${DEPLOY_DIR_IMAGE}/optee/tee-header_v2.bin \
    BL32_EXTRA1=${DEPLOY_DIR_IMAGE}/optee/tee-pager_v2.bin \
    BL32_EXTRA2=${DEPLOY_DIR_IMAGE}/optee/tee-pageable_v2.bin \
"

# Ensure OP-TEE is deployed before TF-A compiles
do_compile[depends] += "optee-os:do_deploy"

# Deploy bl31.bin into the bootfiles directory so sdcard_image-rpi.bbclass
# picks it up and copies it into the FAT boot partition root.
# config.txt references it via: armstub=bl31.bin
do_deploy:append:raspberrypi4-64() {
    install -d ${DEPLOYDIR}/bootfiles
    install -m 0644 ${B}/rpi4/release/bl31.bin ${DEPLOYDIR}/bootfiles/bl31.bin
}
