#ifndef ADS7846_CONTROLLER_H
#define ADS7846_CONTROLLER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ads7846_controller ads7846_controller_t;

/*
 * Userspace ADS7846 access helper.
 *
 * IMPORTANT: this library expects direct access to SPI and IRQ GPIO.
 * If the kernel ads7846 driver (dtoverlay=ads7846) is active, it already owns
 * those resources, so this userspace controller should not be used at the same
 * time.
 */

typedef enum {
    ADS7846_IRQ_EVENT_FALLING = 0,
    ADS7846_IRQ_EVENT_RISING = 1
} ads7846_irq_event_t;

typedef void (*ads7846_irq_callback_t)(ads7846_irq_event_t event,
    uint64_t timestamp_ns,
    void *user_data);

typedef struct {
    const char *spidev_path;
    uint32_t spi_speed_hz;
    uint8_t spi_mode;
    uint8_t spi_bits_per_word;
    unsigned int irq_gpio_number;
} ads7846_config_t;

int ads7846_controller_init(ads7846_controller_t **out_controller,
    const ads7846_config_t *config);

void ads7846_controller_deinit(ads7846_controller_t *controller);

int ads7846_controller_spi_transfer(
    ads7846_controller_t *controller,
    const uint8_t *tx,
    uint8_t *rx,
    size_t len);

int ads7846_controller_fetch_touch_coords(
    ads7846_controller_t * controller, uint16_t * x_pos, uint16_t * y_pos);

int ads7846_controller_set_irq_callback(
    ads7846_controller_t *controller, 
    ads7846_irq_callback_t callback,
    void *user_data);

int ads7846_controller_start_irq_listener(ads7846_controller_t *controller);
int ads7846_controller_stop_irq_listener(ads7846_controller_t *controller);

#ifdef __cplusplus
}
#endif

#endif
