/* hw_config.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/
/*

This file should be tailored to match the hardware design.

See
https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main#customizing-for-the-hardware-configuration

*/

#include <hw_config.h>

#include <a2pico.h>

#if !PICO_RP2350

/* Configuration of hardware SPI object */
static spi_t spi = {
    .hw_inst   = spi0,                  // SPI component
    .sck_gpio  = GPIO_SPI0_SCK,         // GPIO number (not Pico pin number)
    .mosi_gpio = GPIO_SPI0_TX,
    .miso_gpio = GPIO_SPI0_RX,
    .baud_rate = 200 * 1000 * 1000 / 8  // 25 MHz
};

/* SPI Interface */
static sd_spi_if_t spi_if = {
    .spi     = &spi,          // Pointer to the SPI driving this card
    .ss_gpio = GPIO_SPI0_CSN  // The SPI slave select GPIO for this SD card
};

/* Configuration of the SD Card socket object */
static sd_card_t sd_card = {
    .type            = SD_IF_SPI,
    .spi_if_p        = &spi_if,  // Pointer to the SPI interface driving this card
    .use_card_detect = false
};

#endif

/* ********************************************************************** */

size_t sd_get_num() { return 1; }

/**
 * @brief Get a pointer to an SD card object by its number.
 *
 * @param[in] num The number of the SD card to get.
 *
 * @return A pointer to the SD card object, or @c NULL if the number is invalid.
 */
sd_card_t *sd_get_by_num(size_t num) {
#if PICO_RP2350
    return NULL;
#else
    if (0 == num) {
        // The number 0 is a valid SD card number.
        // Return a pointer to the sd_card object.
        return &sd_card;
    } else {
        // The number is invalid. Return @c NULL.
        return NULL;
    }
#endif
}

/* [] END OF FILE */
