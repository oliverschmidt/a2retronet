/*

MIT License

Copyright (c) 2024 Oliver Schmidt (https://a2retro.de/)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <stdio.h>
#include <pico/stdlib.h>

#include "hdd.h"

#include "sp.h"

volatile uint8_t  sp_status;
volatile uint8_t  sp_buffer[1024];
volatile uint16_t sp_read_offset;
volatile uint16_t sp_write_offset;

void sp_init(void) {
    hdd_init();

#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif    
}

void __time_critical_func(sp_reset)(void) {
    sp_status = 0x00;
    sp_write_offset = 0;
    sp_read_offset = 0;
    sp_buffer[0] = sp_buffer[1] = 0;
}

void sp_task(void) {
    if (sp_status == 0x01) {

#ifdef PICO_DEFAULT_LED_PIN
        gpio_put(PICO_DEFAULT_LED_PIN, true);
#endif

//        printf("SP Cmd(Bytes=$%04X)\n", sp_write_offset);
        sp_write_offset = 0;
        sp_read_offset = 0;

        switch (sp_buffer[0]) {
            case 0:
                hdd_status(sp_buffer[1]);
                break;
            case 1:
                hdd_read(sp_buffer[1], *(uint16_t *)&sp_buffer[2]);
                break;
            case 2:
                hdd_write(sp_buffer[1], *(uint16_t *)&sp_buffer[2],
                                         (uint8_t *)&sp_buffer[4]);
                break;
        }
        sp_status = 0x80;

#ifdef PICO_DEFAULT_LED_PIN
        gpio_put(PICO_DEFAULT_LED_PIN, false);
#endif

    }
}
