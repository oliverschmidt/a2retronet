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

#include <string.h>
#include <stdio.h>
#include <pico/stdlib.h>

#include "board.h"
#include "firmware.h"
#include "config.h"
#include "hdd.h"

#include "sp.h"

#define PRODOS_CMD_STATUS   0x00
#define PRODOS_CMD_READ     0x01
#define PRODOS_CMD_WRITE    0x02

#define PRODOS_I_CMD    0
#define PRODOS_I_UNIT   1
#define PRODOS_I_BUFF   2
#define PRODOS_I_BLOCK  4
#define PRODOS_I_BUFFER 6

#define PRODOS_O_RETVAL 0
#define PRODOS_O_BUFFER 1

#define SP_CMD_STATUS   0x00
#define SP_CMD_READBLK  0x01
#define SP_CMD_WRITEBLK 0x02
#define SP_CMD_FORMAT   0x03
#define SP_CMD_CONTROL  0x04
#define SP_CMD_INIT     0x05
#define SP_CMD_OPEN     0x06
#define SP_CMD_CLOSE    0x07
#define SP_CMD_READ     0x08
#define SP_CMD_WRITE    0x09

#define SP_I_CMD    0
#define SP_I_PARAMS 2
#define SP_I_BUFFER 10

#define SP_O_RETVAL 0
#define SP_O_BUFFER 1

#define SP_PARAM_UNIT   0
#define SP_PARAM_BUFF   1
#define SP_PARAM_CODE   3
#define SP_PARAM_BLOCK  3

#define SP_STATUS_STS   0x00
#define SP_STATUS_DCB   0x01
#define SP_STATUS_NLS   0x02
#define SP_STATUS_DIB   0x03

#define SP_SUCCESS  0x00
#define SP_BADCMD   0x01
#define SP_BUSERR   0x06
#define SP_BADCTL   0x21

volatile uint8_t  sp_control;
volatile uint8_t  sp_buffer[0x0400];
volatile uint16_t sp_read_offset;
volatile uint16_t sp_write_offset;

static uint8_t unit_to_drive(uint8_t unit) {
    uint8_t drive = unit >> 7;
    if ((unit >> 4 & 0x07) != board_slot()) {
        drive += 0x02;
    } 
    return drive;
}

void sp_init(void) {
    hdd_init();

#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
}

void __time_critical_func(sp_reset)(void) {
    sp_control = CONTROL_NONE;
    sp_read_offset = sp_write_offset = 0;
    sp_buffer[0] = sp_buffer[1] = 0;
}

void static sp_gen_read_page(uint16_t addr, const uint8_t *buffer, int offset) {
    bool ldx = true;
    bool ldy = true;
    int last = -1;

    for (int i = 0; i < 0x0100; i++) {
        uint8_t data = buffer[i];

        switch (data) {
            case 0x00:
                if (ldx) {
                    ldx = false;
                    firmware[offset++] = 0xA2;  // LDX
                    firmware[offset++] = 0x00;
                }
                firmware[offset++] = 0x8E;  // STX
                firmware[offset++] = addr & 0xFF;
                firmware[offset++] = addr >> 8;
                break;

            case 0x80:
                if (ldy) {
                    ldy = false;
                    firmware[offset++] = 0xA0;  // LDY
                    firmware[offset++] = 0x80;
                }
                firmware[offset++] = 0x8C;  // STY
                firmware[offset++] = addr & 0xFF;
                firmware[offset++] = addr >> 8;
                break;

            default:
                if (last != data) {
                    last = data;
                    firmware[offset++] = 0xA9;  // LDA
                    firmware[offset++] = data;
                }
                firmware[offset++] = 0x8D;  // STA
                firmware[offset++] = addr & 0xFF;
                firmware[offset++] = addr >> 8;
        }

        addr++;
    }

    firmware[offset] = 0x60;  // RTS
}

void static sp_gen_read_block(uint16_t addr, const uint8_t *buffer) {
//    printf("SP Read(Addr=$%04X)\n", addr);
    sp_gen_read_page(addr + 0x0000, buffer + 0x0000, OFFSET_BANK_2);
    sp_gen_read_page(addr + 0x0100, buffer + 0x0100, OFFSET_BANK_3);
}

static uint8_t sp_stat(uint8_t *params, uint8_t *stat_list) {
    if (!params[SP_PARAM_UNIT]) {
        if (params[SP_PARAM_CODE] == SP_STATUS_STS) {
            printf("SP CmdStatus(Device=Smartport)\n");
            stat_list[2 + 0] = config_drives();
            stat_list[2 + 1] = 0b01000000;  // no interrupt sent
            memset(&stat_list[2 + 2], 0x00, 6);
            stat_list[0] = 8;   // size header low
            stat_list[1] = 0;   // size header high
        } else {
            return SP_BADCTL;
        }
    } else {
        if (params[SP_PARAM_CODE] == SP_STATUS_STS ||
            params[SP_PARAM_CODE] == SP_STATUS_DIB) {
            bool status = params[SP_PARAM_CODE] == SP_STATUS_STS;
            if (!hdd_status(params[SP_PARAM_UNIT] - 1, &stat_list[2 + 1])) {
                stat_list[2 + 0] = hdd_protected(params[SP_PARAM_UNIT] - 1)
                                 ? 0b11110100   // block, write, read, online, protected
                                 : 0b11110000;  // block, write, read, online
            } else {
                stat_list[2 + 0] = 0b11100000;  // block, write, read
                stat_list[2 + 1] = 0x00;        // blocks low
                stat_list[2 + 2] = 0x00;        // blocks mid
            }
            stat_list[2 + 3] = 0x00;    // blocks high
            if (status) {
                stat_list[0] = 4;   // size header low
                stat_list[1] = 0;   // size header high                    
            } else {
                stat_list[2 +  4] = 0x0A;   // id string length
                memcpy(&stat_list[2 + 5], "A2RETRONET      ", 16);
                stat_list[2 + 21] = 0x02;   // hard disk
                stat_list[2 + 22] = 0x00;   // removable
                stat_list[2 + 23] = 0x01;   // firmware version low
                stat_list[2 + 24] = 0x00;   // firmware version high
                stat_list[0] = 25;  // size header low
                stat_list[1] = 0;   // size header high    
            }
        } else {
            return SP_BADCTL;
        }
    }
    return SP_SUCCESS;
}

static uint8_t sp_readblk(uint8_t *params, uint8_t *buffer) {
    return hdd_read(params[SP_PARAM_UNIT] - 1, *(uint16_t*)&params[SP_PARAM_BLOCK], buffer);
}

static uint8_t sp_writeblk(uint8_t *params, const uint8_t *buffer) {
    return hdd_write(params[SP_PARAM_UNIT] - 1, *(uint16_t*)&params[SP_PARAM_BLOCK], buffer);
}

void sp_task(void) {
    if (sp_control == CONTROL_NONE || sp_control == CONTROL_DONE) {
        hdd_prefetch();
        return;
    }

    if (!hdd_sd_mounted() && !hdd_usb_mounted()) {
        return;
    }

    if (sp_control == CONTROL_CONFIG) {
        config();
        return;
    }
    
#ifdef PICO_DEFAULT_LED_PIN
    gpio_put(PICO_DEFAULT_LED_PIN, true);
#endif

//    printf("SP Cmd(Type=$%02X,Bytes=$%04X)\n", sp_control, sp_write_offset);
    switch (sp_control) {

        case CONTROL_PRODOS:
            switch (sp_buffer[PRODOS_I_CMD]) {
                case PRODOS_CMD_STATUS:
                    sp_buffer[PRODOS_O_RETVAL] = hdd_status(unit_to_drive(sp_buffer[PRODOS_I_UNIT]),
                                                            (uint8_t*)&sp_buffer[PRODOS_O_BUFFER]);
                    break;
                case PRODOS_CMD_READ:
                    sp_buffer[PRODOS_O_RETVAL] = hdd_read(unit_to_drive(sp_buffer[PRODOS_I_UNIT]),
                                                          *(uint16_t*)&sp_buffer[PRODOS_I_BLOCK],
                                                          (uint8_t*)&sp_buffer[0x0200]);
                    sp_gen_read_block(*(uint16_t*)&sp_buffer[PRODOS_I_BUFF],
                                      (uint8_t*)&sp_buffer[0x0200]);
                    break;
                case PRODOS_CMD_WRITE:
                    sp_buffer[PRODOS_O_RETVAL] = hdd_write(unit_to_drive(sp_buffer[PRODOS_I_UNIT]),
                                                           *(uint16_t*)&sp_buffer[PRODOS_I_BLOCK],
                                                           (uint8_t*)&sp_buffer[PRODOS_I_BUFFER]);
                    break;
            }
            break;

        case CONTROL_SP:
            switch (sp_buffer[SP_I_CMD]) {
                case SP_CMD_STATUS:
//                    printf("SP CmdStatus(Device=$%02X)\n", sp_buffer[SP_I_PARAMS]);
                    sp_buffer[SP_O_RETVAL] = sp_stat((uint8_t*)&sp_buffer[SP_I_PARAMS],
                                                     (uint8_t*)&sp_buffer[SP_O_BUFFER]);
                    break;
                case SP_CMD_READBLK:
//                    printf("SP CmdReadBlock(Device=$%02X)\n", sp_buffer[SP_I_PARAMS]);
                    sp_buffer[SP_O_RETVAL] = sp_readblk((uint8_t*)&sp_buffer[SP_I_PARAMS],
                                                        (uint8_t*)&sp_buffer[0x0200]);
                    sp_gen_read_block(*(uint16_t*)&sp_buffer[SP_I_PARAMS + SP_PARAM_BUFF],
                                      (uint8_t*)&sp_buffer[0x0200]);
                    break;
                case SP_CMD_WRITEBLK:
//                    printf("SP CmdWriteBlock(Device=$%02X)\n", sp_buffer[SP_I_PARAMS]);
                    sp_buffer[SP_O_RETVAL] = sp_writeblk((uint8_t*)&sp_buffer[SP_I_PARAMS],
                                                         (uint8_t*)&sp_buffer[SP_I_BUFFER]);
                    break;
                case SP_CMD_FORMAT:
                    printf("SP CmdFormat(Device=$%02X)\n", sp_buffer[SP_I_PARAMS]);
                    sp_buffer[SP_O_RETVAL] = SP_BADCMD;
                    break;
                case SP_CMD_CONTROL:
                    printf("SP CmdControl(Device=$%02X)\n", sp_buffer[SP_I_PARAMS]);
                    sp_buffer[SP_O_RETVAL] = SP_BADCMD;
                    break;
                case SP_CMD_INIT:
                    printf("SP CmdInit(Device=$%02X)\n", sp_buffer[SP_I_PARAMS]);
                    sp_buffer[SP_O_RETVAL] = SP_SUCCESS;
                    break;
                case SP_CMD_OPEN:
                    printf("SP CmdOpen(Device=$%02X)\n", sp_buffer[SP_I_PARAMS]);
                    sp_buffer[SP_O_RETVAL] = SP_BADCMD;
                    break;
                case SP_CMD_CLOSE:
                    printf("SP CmdClose(Device=$%02X)\n", sp_buffer[SP_I_PARAMS]);
                    sp_buffer[SP_O_RETVAL] = SP_BADCMD;
                    break;
                case SP_CMD_READ:
                    printf("SP CmdRead(Device=$%02X)\n", sp_buffer[SP_I_PARAMS]);
                    sp_buffer[SP_O_RETVAL] = SP_BADCMD;
                    break;
                case SP_CMD_WRITE:
                    printf("SP CmdWrite(Device=$%02X)\n", sp_buffer[SP_I_PARAMS]);
                    sp_buffer[SP_O_RETVAL] = SP_BADCMD;
                    break;
            }
            break;
    }

    sp_read_offset = sp_write_offset = 0;
    sp_control = CONTROL_DONE;

    hdd_async();

#ifdef PICO_DEFAULT_LED_PIN
    gpio_put(PICO_DEFAULT_LED_PIN, false);
#endif
}
