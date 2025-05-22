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

#include <pico/time.h>
#include <pico/multicore.h>
#include <a2pico.h>

#include "sp.h"

#include "board.h"

#define IOSEL_BANK0  0x0000
#define IOSEL_BANK1  0x0800
#define IOSTRB_OFF   0x0000
#define IOSTRB_BANK0 0x0800
#define IOSTRB_BANK1 0x1000

extern const __attribute__((aligned(4))) uint8_t firmware[];

static volatile uint32_t iosel;
static volatile uint32_t iostrb;

static volatile uint32_t self;

static volatile uint32_t ser_command;
static volatile uint32_t ser_control;

static uint8_t ser_bits[] = {
    0b11111111,
    0b01111111,
    0b00111111,
    0b00011111};

static volatile uint8_t ser_mask;

static volatile uint8_t output_mask;

static void __time_critical_func(reset)(bool asserted) {
    static absolute_time_t assert_time;

    if (asserted) {
        iostrb = IOSTRB_OFF;

        ser_command = 0b00000000;
        ser_control = 0b00000000;
        ser_mask = ser_bits[0b00];

        output_mask = 0b11111111;

        multicore_fifo_drain();
        sp_reset();

        assert_time = get_absolute_time();
    } else {
        if (absolute_time_diff_us(assert_time, get_absolute_time()) > 200000) {
            iosel = IOSEL_BANK0;
        }
    }
}

static void __time_critical_func(nop_get)(void) {
}

static void __time_critical_func(ser_dipsw1_get)(void) {
                        // 0001     9600 baud
                        //     00   <zero>
                        //       01 printer mode
    a2pico_putdata(pio0, 0b11101110);
}

static void __time_critical_func(ser_dipsw2_get)(void) {
                        // 1        1 stop bit
                        //  0       <zero>
                        //   0      no delay
                        //    0     <zero>
                        //     11   40 cols
                        //       1  auto-lf
                        //        0 cts line (not dipsw2)   
    a2pico_putdata(pio0, 0b01110000);
}

static void __time_critical_func(ser_data_get)(void) {
    a2pico_putdata(pio0, sio_hw->fifo_rd & ser_mask);
}

static void __time_critical_func(ser_status_get)(void) {
    // SIO_FIFO_ST_VLD_BITS _u(0x00000001)
    // SIO_FIFO_ST_RDY_BITS _u(0x00000002)
    a2pico_putdata(pio0, (sio_hw->fifo_st & 3) << 3);
}

static void __time_critical_func(ser_command_get)(void) {
    a2pico_putdata(pio0, ser_command);
}

static void __time_critical_func(ser_control_get)(void) {
    a2pico_putdata(pio0, ser_control);
}

void (*devsel_get[])(void) = {
    nop_get,      ser_dipsw1_get, ser_dipsw2_get,  nop_get,
    nop_get,      nop_get,        nop_get,         nop_get,
    ser_data_get, ser_status_get, ser_command_get, ser_control_get,
    nop_get,      nop_get,        nop_get,         nop_get
};

static void __time_critical_func(nop_put)(uint32_t data) {
}

static void __time_critical_func(ser_data_put)(uint32_t data) {
    sio_hw->fifo_wr = data & ser_mask & output_mask;
}

static void __time_critical_func(ser_reset_put)(uint32_t data) {
    ser_command &= 0b11100000;
}

static void __time_critical_func(ser_command_put)(uint32_t data) {
    ser_command = data;
}

static void __time_critical_func(ser_control_put)(uint32_t data) {
    ser_control = data;
    ser_mask = ser_bits[(data >> 5) & 0b11];
}

void (*devsel_put[])(uint32_t) = {
    nop_put,      nop_put,       nop_put,         nop_put,
    nop_put,      nop_put,       nop_put,         nop_put,
    ser_data_put, ser_reset_put, ser_command_put, ser_control_put,
    nop_put,      nop_put,       nop_put,         nop_put
};

static void __time_critical_func(sp_data_get)(void) {
    if (!iostrb) {
        return;
    }
    a2pico_putdata(pio0, sp_buffer[sp_read_offset]);
    sp_read_offset++;
}

static void __time_critical_func(sp_control_get)(void) {
    if (!iostrb) {
        return;
    }
    a2pico_putdata(pio0, sp_control);
}

static void __time_critical_func(basic_enter_get)(void) {
    if (!iostrb) {
        return;
    }
    output_mask = 0b01111111;
}

static void __time_critical_func(basic_leave_get)(void) {
    if (!iostrb) {
        return;
    }
    output_mask = 0b11111111;
}

static void __time_critical_func(iosel_bank0_get)(void) {
    if (!iostrb) {
        return;
    }
    iosel = IOSEL_BANK0;
}

static void __time_critical_func(iosel_bank1_get)(void) {
    if (!iostrb) {
        return;
    }
    iosel = IOSEL_BANK1;
}

static void __time_critical_func(iostrb_off_get)(void) {
    iostrb = IOSTRB_OFF;
}

static void __time_critical_func(iostrb_bank0_get)(void) {
    if (!iostrb) {
        return;
    }
    iostrb = IOSTRB_BANK0;
}

static void __time_critical_func(iostrb_bank1_get)(void) {
    if (!iostrb) {
        return;
    }
    iostrb = IOSTRB_BANK1;
}

void (*cffx_get[])(void) = {
    sp_data_get,      sp_control_get,  nop_get,         nop_get,
    nop_get,          nop_get,         nop_get,         nop_get,
    nop_get,          basic_enter_get, basic_leave_get, iostrb_bank0_get,
    iostrb_bank1_get, iosel_bank0_get, iosel_bank1_get, iostrb_off_get
};

static void __time_critical_func(sp_data_put)(uint32_t data) {
    if (!iostrb) {
        return;
    }
    sp_buffer[sp_write_offset++] = data;
}

static void __time_critical_func(sp_control_put)(uint32_t data) {
    if (!iostrb) {
        return;
    }
    sp_control = data;
}

static void __time_critical_func(basic_enter_put)(uint32_t data) {
    if (!iostrb) {
        return;
    }
    output_mask = 0b01111111;
}

static void __time_critical_func(basic_leave_put)(uint32_t data) {
    if (!iostrb) {
        return;
    }
    output_mask = 0b11111111;
}

static void __time_critical_func(iosel_bank0_put)(uint32_t data) {
    if (!iostrb) {
        return;
    }
    iosel = IOSEL_BANK0;
}

static void __time_critical_func(iosel_bank1_put)(uint32_t data) {
    if (!iostrb) {
        return;
    }
    iosel = IOSEL_BANK1;
}

static void __time_critical_func(iostrb_off_put)(uint32_t data) {
    iostrb = IOSTRB_OFF;
}

static void __time_critical_func(iostrb_bank0_put)(uint32_t data) {
    if (!iostrb) {
        return;
    }
    iostrb = IOSTRB_BANK0;
}

static void __time_critical_func(iostrb_bank1_put)(uint32_t data) {
    if (!iostrb) {
        return;
    }
    iostrb = IOSTRB_BANK1;
}

void (*cffx_put[])(uint32_t) = {
    sp_data_put,      sp_control_put,  nop_put,         nop_put,
    nop_put,          nop_put,         nop_put,         nop_put,
    nop_put,          basic_enter_put, basic_leave_put, iostrb_bank0_put,     
    iostrb_bank1_put, iosel_bank0_put, iosel_bank1_put, iostrb_off_put
};

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
            if (addr >= 0x0FF0) {           // IOSTRB
                cffx_get[addr & 0xF]();
            } else if (!io) {               // DEVSEL
                devsel_get[addr & 0xF]();
            } else if (!strb) {             // IOSEL
                a2pico_putdata(pio0, firmware[addr + iosel]);
                iostrb = IOSTRB_BANK0;
                self   = addr;
            } else if (iostrb) {            // IOSTRB
                a2pico_putdata(pio0, firmware[addr + iostrb]);
            }
        } else {
            uint32_t data = a2pico_getdata(pio0);
            if (addr >= 0x0FF0) {           // IOSTRB
                cffx_put[addr & 0xF](data);
            } else if (!io) {               // DEVSEL
                devsel_put[addr & 0xF](data);
            } else if (!strb) {             // IOSEL
                iostrb = IOSTRB_BANK0;
            }
        }
    }
}

uint8_t board_slot(void) {
    return self >> 8;
}
