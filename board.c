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

#include <a2pico.h>

#include "sp.h"

#include "board.h"

extern const __attribute__((aligned(4))) uint8_t firmware[];

static volatile bool     active;
static volatile uint32_t self;

static void __time_critical_func(reset)(bool asserted) {
    if (asserted) {
        active = false;
        sp_reset();
    }
}

void __time_critical_func(board)(void) {

    a2pico_init(pio0);

    a2pico_resethandler(&reset);

    while (true) {
        uint32_t pico = a2pico_getaddr(pio0);
        uint32_t addr = pico & 0x0FFF;
        uint32_t io   = pico & 0x0F00;      // IOSTRB or IOSEL
        uint32_t strb = pico & 0x0800;      // IOSTRB
        uint32_t read = pico & 0x1000;      // R/W

        if (read) {
            if (io && (!strb || active)) {
                switch (addr) {
                    case 0x0FFD:    // DATA
                        a2pico_putdata(pio0, sp_buffer[sp_read_offset]);
                        sp_read_offset++;
                        break;
                    case 0x0FFE:    // CONTROL
                        a2pico_putdata(pio0, sp_control);
                        break;
                    case 0x0FFF:
                        break;
                    default:
                        a2pico_putdata(pio0, firmware[addr]);
                }
            }
        } else {
            uint32_t data = a2pico_getdata(pio0);
            switch (addr) {
                case 0x0FFD:    // DATA
                    sp_buffer[sp_write_offset++] = data;
                    break;
                case 0x0FFE:    // CONTROL
                    sp_control = data;
                    break;
            }
        }

        if (io && !strb) {
            active = true;
            self   = addr;
        } else if (addr == 0x0FFF) {
            active = false;
        }
    }
}

uint8_t board_slot(void) {
    return self >> 8;
}
