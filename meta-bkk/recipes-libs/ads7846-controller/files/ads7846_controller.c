#include "ads7846_controller.h"

#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <unistd.h>

#ifndef O_CLOEXEC
// to silence clangd warnings
#define O_CLOEXEC 0
#endif

#define SYSFS_GPIO_PATH "/sys/class/gpio"
#define ADS7846_CMD_X  0xD0  /* channel 1 (X), 12-bit, differential */
#define ADS7846_CMD_Y  0x90  /* channel 0 (Y), 12-bit, differential */



struct ads7846_controller {
    int spi_fd;
    uint32_t spi_speed_hz;
    uint8_t spi_mode;
    uint8_t spi_bits_per_word;

    unsigned int irq_gpio_number;
    int irq_value_fd;
    int stop_event_fd;
    bool irq_gpio_exported_by_us;

    ads7846_irq_callback_t callback;
    void *callback_user_data;

    pthread_t irq_thread;
    bool irq_thread_running;
    bool stop_requested;
    pthread_mutex_t lock;
};


static int read_adc(ads7846_controller_t *controller, uint8_t cmd, uint16_t * out) {

  uint8_t tx[] = { cmd, 0x00, 0x00 };
  uint8_t rx[3] = { 0 };

  int ret = ads7846_controller_spi_transfer(
    controller, tx, rx, sizeof(tx));
  if (ret != 0) {
    return ret;
  }

  *out = (uint16_t)((((rx[1] << 8) | rx[2]) >> 3) & 0x0FFF);
  return 0;
} 


static int write_text_file(const char *path, const char *value) {
    // local implementation of writing a string to a sysfs file
    // open, write and close
    const size_t len = strlen(value);
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        return -errno;
    }

    if (write(fd, value, len) != (ssize_t)len) {
        const int err = errno;
        close(fd);
        return -err;
    }

    close(fd);
    return 0;
}

static int gpio_write_attr(
    unsigned int gpio, const char *attr, const char *value) {
    // set a specific attribute of a GPIO 
    char path[128];
    int retval = snprintf(
        path, 
        sizeof(path), 
        SYSFS_GPIO_PATH "/gpio%u/%s", 
        gpio, attr);

    if (retval < 0) {
        return -EINVAL;
    }

    retval = write_text_file(path, value);
    return retval;
}

static int export_gpio(ads7846_controller_t *controller) {
    // export gpio device if not already exported
    char buf[16];
    int status;

    // put the interrupt pin into the buffer: 
    int retval = snprintf(
        buf, 
        sizeof(buf), 
        "%u", 
        controller->irq_gpio_number);

    if (retval < 0) {
        return -EINVAL;
    }

    // export gpio file: 
    status = write_text_file(
        SYSFS_GPIO_PATH "/export", 
        buf);

    if (status == -EBUSY) {
        // already exported, not an error: 
        return 0;
    }
    if (status < 0) {
        // other error:
        return status;
    }

    // store to later know if its need to be unexported: 
    controller->irq_gpio_exported_by_us = true;
    return 0;
}

static int gpio_unexport_if_owned(ads7846_controller_t *controller) {

    char buf[16];

    if (!controller->irq_gpio_exported_by_us) {
        // we did not export it, do nothing: 
        return 0;
    }

    // assemble string to unexport the gpio:
    int retval = snprintf(
        buf, 
        sizeof(buf), 
        "%u", 
        controller->irq_gpio_number);

    if (retval < 0) {
        return -EINVAL;
    }

    // unexport gpio file:
    retval = write_text_file(
        SYSFS_GPIO_PATH "/unexport", 
        buf);

    return retval;
}

static int gpio_prepare_irq_line(ads7846_controller_t *controller)
{
    char value_path[128];
    char foo;
    int status;

    status = export_gpio(controller);
    if (status < 0) {
        return status;
    }

    // set direction to input:
    status = gpio_write_attr(
        controller->irq_gpio_number, 
        "direction", 
        "in");

    if (status < 0) {
        return status;
    }

    // set trigger edge to both:
    status = gpio_write_attr(
        controller->irq_gpio_number, 
        "edge", 
        "both");
    if (status < 0) {
        return status;
    }

    // assemble string to access the value file:
    status = snprintf(
        value_path, 
        sizeof(value_path), 
        SYSFS_GPIO_PATH "/gpio%u/value", 
        controller->irq_gpio_number);

    if (status < 0) {
        return -EINVAL;
    }

    // connect the value file to be monitored for interrupts:
    controller->irq_value_fd = open(
        value_path, 
        O_RDONLY | O_CLOEXEC | O_NONBLOCK);

    if (controller->irq_value_fd < 0) {
        return -errno;
    }


    // reset and drain any pending interrupt: 
    (void)lseek(controller->irq_value_fd, 0, SEEK_SET);
    (void)read(controller->irq_value_fd, &foo, 1);
    return 0;
}

static int ads7846_configure_spi(ads7846_controller_t *controller) {
    int retval = 0;
    // set CPOL and CPHA: (to be checked in datasheet): 
    retval = ioctl(controller->spi_fd, 
        SPI_IOC_WR_MODE, &controller->spi_mode);
    if (retval < 0) {
        return -errno;
    }

    // set bits per word:
    retval = ioctl(controller->spi_fd, 
        SPI_IOC_WR_BITS_PER_WORD, &controller->spi_bits_per_word);
    if (retval < 0) {
        return -errno;
    }

    // set speed:
    retval = ioctl(controller->spi_fd, 
        SPI_IOC_WR_MAX_SPEED_HZ, &controller->spi_speed_hz);
    if (retval < 0) {
        return -errno;
    }

    return 0;
}

static void *ads7846_irq_thread_main(void *arg) {
    // monitor the interrupt line and call the user callback: 

    ads7846_controller_t *controller = (ads7846_controller_t *)arg;

    while (true) {
        struct pollfd pfds[2];
        int poll_result;
        char value_char = '1';

        // one fd for the interrupt line: 
        pfds[0].fd = controller->irq_value_fd;
        pfds[0].events = POLLPRI | POLLERR;
        pfds[0].revents = 0;

        // one fd for the stop event:
        pfds[1].fd = controller->stop_event_fd;
        pfds[1].events = POLLIN;
        pfds[1].revents = 0;

        // poll both 
        poll_result = poll(
            pfds, 
            2, 
            -1); // infinite block

        if (poll_result < 0) {
            if (errno == EINTR) {
                // interrupted, just retry:
                continue;
            }

            // error: 
            break;
        }

        if ((pfds[1].revents & POLLIN) != 0) {
            // interrupt request thread stopped: 
            uint64_t wake_counter;

            // drain the stop event and exit: 
            (void)read(
                controller->stop_event_fd, 
                &wake_counter, 
                sizeof(wake_counter));
            break;
        }

        if ((pfds[0].revents & (POLLPRI | POLLERR)) == 0) {
            // no relevant event, just continue polling:
            continue;
        }

        if (lseek(controller->irq_value_fd, 0, SEEK_SET) < 0) {
            // failed to reset the file offset, just continue and try again:
            continue;
        }
        if (read(controller->irq_value_fd, &value_char, 1) < 0) {
            // failed to read the value, just continue and try again:
            continue;
        }

        pthread_mutex_lock(&controller->lock);
        // copy the callback and user data under the lock: 
        ads7846_irq_callback_t callback = controller->callback;
        void *user_data = controller->callback_user_data;
        pthread_mutex_unlock(&controller->lock);

        if (callback != NULL) {
            // if non-null, call it with the event type: 
            const ads7846_irq_event_t mapped_event =
                (value_char == '0') 
                ? ADS7846_IRQ_EVENT_FALLING 
                : ADS7846_IRQ_EVENT_RISING;
            callback(mapped_event, 0, user_data);
        }
    }

    return NULL;
}

int ads7846_controller_init(ads7846_controller_t **out_controller,
                            const ads7846_config_t *config) {
    ads7846_controller_t *controller;
    int status;

    if (out_controller == NULL || config == NULL || config->spidev_path == NULL) {
        return -EINVAL;
    }

    // allocate and zero the controller structure:
    controller = (ads7846_controller_t *)calloc(1, sizeof(*controller));
    if (controller == NULL) {
        return -ENOMEM;
    }

    // init fds to invalid: 
    controller->spi_fd = -1;
    controller->irq_value_fd = -1;
    controller->stop_event_fd = -1;

    controller->spi_fd = open(config->spidev_path, O_RDWR | O_CLOEXEC);
    if (controller->spi_fd < 0) {
        // cannot open spi, cleanup: 
        free(controller);
        return -errno;
    }

    // store config: 
    controller->spi_speed_hz = config->spi_speed_hz;
    controller->spi_mode = config->spi_mode;
    controller->spi_bits_per_word = config->spi_bits_per_word;

    // apply config: 
    status = ads7846_configure_spi(controller);
    if (status < 0) {
        // if failes, cleanup and exit:
        close(controller->spi_fd);
        free(controller);
        return status;
    }

    // strore interrupt gpio number:
    controller->irq_gpio_number = config->irq_gpio_number;

    // config and cleanup and return if fails: 
    status = gpio_prepare_irq_line(controller);
    if (status < 0) {
        close(controller->spi_fd);
        free(controller);
        return status;
    }

    // create the eventfd for stopping the irq thread:
    controller->stop_event_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (controller->stop_event_fd < 0) {
        close(controller->irq_value_fd);
        close(controller->spi_fd);
        (void)gpio_unexport_if_owned(controller);
        free(controller);
        return -errno;
    }

    // initialize the mutex:
    status = pthread_mutex_init(&controller->lock, NULL);
    if (status != 0) {
        close(controller->stop_event_fd);
        close(controller->irq_value_fd);
        close(controller->spi_fd);
        (void)gpio_unexport_if_owned(controller);
        free(controller);
        return -status;
    }

    // success, return the controller:
    *out_controller = controller;
    return 0;
}

void ads7846_controller_deinit(ads7846_controller_t *controller) {
    // cleanup and free everything: 
    if (controller == NULL) {
        return;
    }

    ads7846_controller_stop_irq_listener(controller);
    pthread_mutex_destroy(&controller->lock);

    if (controller->irq_value_fd >= 0) {
        close(controller->irq_value_fd);
    }

    if (controller->stop_event_fd >= 0) {
        close(controller->stop_event_fd);
    }

    (void)gpio_unexport_if_owned(controller);

    if (controller->spi_fd >= 0) {
        close(controller->spi_fd);
    }

    free(controller);

    return; 
}

int ads7846_controller_spi_transfer(ads7846_controller_t *controller,
        const uint8_t *tx,
        uint8_t *rx,
        size_t len) {
    
    // use standard linux spidev interface: 
    struct spi_ioc_transfer transfer;

    if (controller == NULL || (tx == NULL && rx == NULL) || len == 0) {
        // invalid input: 
        return -EINVAL;
    }

    memset(&transfer, 0, sizeof(transfer));
    transfer.tx_buf = (unsigned long)tx;
    transfer.rx_buf = (unsigned long)rx;
    transfer.len = (uint32_t)len;
    transfer.speed_hz = controller->spi_speed_hz;
    transfer.bits_per_word = controller->spi_bits_per_word;

    // start trx: 
    int status = ioctl(
        controller->spi_fd, 
        SPI_IOC_MESSAGE(1), 
        &transfer);

    if (status < 0) {
        return -errno;
    }

    return 0;
}


int ads7846_controller_fetch_touch_coords(
        ads7846_controller_t * controller, uint16_t * x_pos, uint16_t * y_pos) {

    if(controller == NULL) {
        return -EINVAL;
    }

    uint16_t x_raw = 0, y_raw = 0;

    int retVal; 

    retVal = read_adc(controller, ADS7846_CMD_X, &x_raw);
    retVal |= read_adc(controller, ADS7846_CMD_Y, &y_raw);

    if (retVal != 0) {
        return retVal;
    }

    *x_pos = x_raw;
    *y_pos = y_raw;

    return 0;
}

int ads7846_controller_set_irq_callback(ads7846_controller_t *controller,
                                        ads7846_irq_callback_t callback,
                                        void *user_data) {
    if (controller == NULL) {
        return -EINVAL;
    }

    pthread_mutex_lock(&controller->lock);
    // store callback from user: 
    controller->callback = callback;
    controller->callback_user_data = user_data;
    pthread_mutex_unlock(&controller->lock);
    return 0;
}

int ads7846_controller_start_irq_listener(ads7846_controller_t *controller) {
    // start listening thread: 
    uint64_t stale;

    if (controller == NULL) {
        return -EINVAL;
    }

    // drain any stale stop signal before starting a fresh listener thread
    while (read(controller->stop_event_fd, &stale, sizeof(stale)) == (ssize_t)sizeof(stale)) {
        // read till empty
    }

    pthread_mutex_lock(&controller->lock);
    if (controller->irq_thread_running) {
        // if already running, do nothing:
        pthread_mutex_unlock(&controller->lock);
        return 0;
    }
    controller->stop_requested = false;
    pthread_mutex_unlock(&controller->lock);

    {
        // create thread: 
        const int ret = pthread_create(
            &controller->irq_thread, 
            NULL, 
            ads7846_irq_thread_main, 
            controller);
        if (ret != 0) {
            return -ret;
        }
    }

    pthread_mutex_lock(&controller->lock);
    // set to running: 
    controller->irq_thread_running = true;
    pthread_mutex_unlock(&controller->lock);
    return 0;
}

int ads7846_controller_stop_irq_listener(ads7846_controller_t *controller) {
    const uint64_t wake_signal = 1;

    if (controller == NULL) {
        return -EINVAL;
    }

    pthread_mutex_lock(&controller->lock);
    if (!controller->irq_thread_running) {
        // not running, do nothing:
        pthread_mutex_unlock(&controller->lock);
        return 0;
    }
    controller->stop_requested = true;
    pthread_mutex_unlock(&controller->lock);

    // send signal to the thread to wake and stop: 
    if (write(controller->stop_event_fd, 
        &wake_signal, 
        sizeof(wake_signal)) < 0 && errno != EAGAIN) {
        return -errno;
    }

    // wait for thread to exit:
    pthread_join(controller->irq_thread, NULL);

    pthread_mutex_lock(&controller->lock);
    controller->irq_thread_running = false;
    pthread_mutex_unlock(&controller->lock);

    return 0;
}
