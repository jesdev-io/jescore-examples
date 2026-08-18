#ifndef TINYUSB_AUDIO_H753ZI_PORT_H
#define TINYUSB_AUDIO_H753ZI_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include <stdbool.h>

/// @brief User/application setup hook called by the STM32 port main after HAL,
///        clock, and base GPIO initialization.
void port_setup(void);

/// @brief User/application loop hook. For jescore examples this is usually not
///        reached because port_setup() starts the scheduler.
void port_loop(void);

/// @brief Initialize the STM32 USB FS pins, peripheral clock, voltage detector,
///        and OTG_FS interrupt for TinyUSB device mode.
void port_usb_peripheral_init(void);

/// @brief Drive the example status LED from high-level app state.
void port_usb_set_mounted_led(bool mounted);

#ifdef __cplusplus
}
#endif

#endif /* TINYUSB_AUDIO_H753ZI_PORT_H */
