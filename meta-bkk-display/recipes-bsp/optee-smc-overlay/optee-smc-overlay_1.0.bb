SUMMARY = "Raspberry Pi OP-TEE firmware DT overlay"
DESCRIPTION = "Builds and deploys optee-smc.dtbo for IMAGE_BOOT_FILES mapping"
LICENSE = "CLOSED"

SRC_URI = "file://optee-smc-overlay.dts"
S = "${WORKDIR}"
B = "${WORKDIR}"

inherit deploy

DEPENDS = "dtc-native"

do_compile() {
    dtc -@ -I dts -O dtb -o optee-smc.dtbo ${WORKDIR}/optee-smc-overlay.dts
}

do_deploy() {
    install -m 0644 ${WORKDIR}/optee-smc.dtbo ${DEPLOYDIR}/optee-smc.dtbo
}

addtask deploy before do_build after do_compile
