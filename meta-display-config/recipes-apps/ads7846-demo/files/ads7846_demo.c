#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ads7846_controller.h"

/* ADS7846 control byte: START | channel | 12-bit | differential */
#define ADS7846_CMD_X  0xD0  /* channel 1 (X), 12-bit, differential */
#define ADS7846_CMD_Y  0x90  /* channel 0 (Y), 12-bit, differential */
#define ADS7846_CMD_Z1 0xB0  /* Z1 pressure */
#define ADS7846_CMD_Z2 0xC0  /* Z2 pressure */

static volatile sig_atomic_t g_running = 1;
static ads7846_controller_t *g_controller;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

static int read_adc(ads7846_controller_t *ctrl, uint8_t cmd, uint16_t *out)
{
    /*
     * 3-byte SPI transaction:
     *   TX: [cmd] [0x00] [0x00]
     *   RX: [xxx] [MSB ] [LSB ]
     * Result is a 12-bit value in bits [14:3] of the 16-bit response.
     */
    uint8_t tx[3] = { cmd, 0x00, 0x00 };
    uint8_t rx[3] = { 0 };

    int ret = ads7846_controller_spi_transfer(ctrl, tx, rx, sizeof(tx));
    if (ret < 0) {
        return ret;
    }

    *out = (uint16_t)(((rx[1] << 8) | rx[2]) >> 3) & 0x0FFF;
    return 0;
}

static void irq_callback(ads7846_irq_event_t event,
                          uint64_t timestamp_ns,
                          void *user_data)
{
    ads7846_controller_t *ctrl = (ads7846_controller_t *)user_data;
    (void)timestamp_ns;

    if (event == ADS7846_IRQ_EVENT_FALLING) {
        /* Pen down — read touch coordinates */
        uint16_t x = 0, y = 0, z1 = 0, z2 = 0;

        if (read_adc(ctrl, ADS7846_CMD_X, &x) < 0 ||
            read_adc(ctrl, ADS7846_CMD_Y, &y) < 0 ||
            read_adc(ctrl, ADS7846_CMD_Z1, &z1) < 0 ||
            read_adc(ctrl, ADS7846_CMD_Z2, &z2) < 0) {
            fprintf(stderr, "SPI read failed\n");
            return;
        }

        printf("TOUCH  X=%4u  Y=%4u  Z1=%4u  Z2=%4u\n", x, y, z1, z2);
    } else {
        printf("RELEASE\n");
    }
}

int main(int argc, char *argv[])
{
    const char *spidev = "/dev/spidev0.0";
    unsigned int irq_gpio = 25;

    if (argc >= 2) {
        spidev = argv[1];
    }
    if (argc >= 3) {
        irq_gpio = (unsigned int)atoi(argv[2]);
    }

    printf("ads7846 demo app\n");
    printf("  SPI device : %s\n", spidev);
    printf("  IRQ GPIO   : %u\n", irq_gpio);
    printf("  Press Ctrl+C to exit\n\n");

    const ads7846_config_t config = {
        .spidev_path       = spidev,
        .spi_speed_hz      = 1000000,
        .spi_mode          = 0,
        .spi_bits_per_word = 8,
        .irq_gpio_number   = irq_gpio,
    };

    int ret = ads7846_controller_init(&g_controller, &config);
    if (ret != 0) {
        fprintf(stderr, "ads7846_controller_init failed: %d\n", ret);
        return EXIT_FAILURE;
    }

    ret = ads7846_controller_set_irq_callback(g_controller, irq_callback, g_controller);
    if (ret != 0) {
        fprintf(stderr, "set_irq_callback failed: %d\n", ret);
        ads7846_controller_deinit(g_controller);
        return EXIT_FAILURE;
    }

    ret = ads7846_controller_start_irq_listener(g_controller);
    if (ret != 0) {
        fprintf(stderr, "start_irq_listener failed: %d\n", ret);
        ads7846_controller_deinit(g_controller);
        return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    while (g_running) {
        pause();
    }

    printf("\nShutting down...\n");
    ads7846_controller_stop_irq_listener(g_controller);
    ads7846_controller_deinit(g_controller);

    return EXIT_SUCCESS;
}
