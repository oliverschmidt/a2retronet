/*

MIT License

Copyright (c) 2026 Oliver Schmidt (https://a2retro.de/)

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

#include <tusb.h>

#include "board.h"
#include "gen.h"
#include "sp.h"

#include "slip.h"

#define END     0xC0
#define ESC     0xDB
#define ESC_END 0xDC
#define ESC_ESC 0xDD

#define STATE_IDL   0
#define STATE_ACT   1
#define STATE_ESC   2

#define REQ_DONE    (UINT8_MAX + 1)

#define SP_CMD_STATUS   0x00
#define SP_CMD_READBLK  0x01
#define SP_CMD_WRITEBLK 0x02
#define SP_CMD_CONTROL  0x04
#define SP_CMD_INIT     0x05
#define SP_CMD_WRITE    0x09

#define SP_REQ_SIZE 10
#define BLK_SIZE    512

#define SP_I_CMD        0
#define SP_I_PARAM_CNT  1
#define SP_I_UNIT       2
#define SP_I_STS_CODE   5
#define SP_I_BLK_BUFF   3
#define SP_I_BLK_BLOCK  5
#define SP_I_BLK_BUFFER 10
#define SP_I_CTRL_CODE  5
#define SP_I_CTRL_CNT   10
#define SP_I_WR_CNT     5

#define SP_O_RETVAL     0
#define SP_O_STS_CNT    1
#define SP_O_STS_LIST   3
#define SP_O_BLK_BUFFER 1

#define SP_STATUS_STS   0x00

#define SP_SUCCESS  0x00
#define SP_BADCMD   0x01
#define SP_BUSERR   0x06
#define SP_BADCTL   0x21
#define SP_NODRIVE  0x28

#define PRODOS_I_CMD    0
#define PRODOS_I_UNIT   1
#define PRODOS_I_BUFF   2
#define PRODOS_I_BLOCK  4
#define PRODOS_I_BUFFER 6

#define PRODOS_O_RETVAL 0
#define PRODOS_O_BUFFER 1

#define PRODOS_SUCCESS      0x00
#define PRODOS_IO_ERROR     0x27
#define PRODOS_WRITE_PROT   0x2B

static volatile bool connected;

static uint8_t units;
static uint8_t command;
static uint8_t sequence_number;

void __time_critical_func(slip_reset)(void) {
    connected = false;
}

bool slip_connected(void) {
    return connected;
}

static void send_byte(uint8_t data) {
    switch (data) {
        case END:
            tud_cdc_write_char(ESC);
            tud_cdc_write_char(ESC_END);
            break;
        case ESC:
            tud_cdc_write_char(ESC);
            tud_cdc_write_char(ESC_ESC);
            break;
        default:
            tud_cdc_write_char(data);
    }
}

static void send_request(const uint8_t *buffer) {
    command = buffer[SP_I_CMD];
//    printf("SLIP Send Request(Seq=$%02X,Cmd=$%02X)\n", sequence_number, command);

    tud_cdc_write_char(END);
    send_byte(sequence_number);

    int i = 0;
    while (i < SP_REQ_SIZE) {
        send_byte(buffer[i++]);
    }

    int size = i;
    int skip = 0;
    switch (buffer[SP_I_CMD]) {
        case SP_CMD_WRITEBLK:
            size += BLK_SIZE;
            break;
        case SP_CMD_CONTROL:
            size += buffer[SP_I_CTRL_CNT + 0] +
                    buffer[SP_I_CTRL_CNT + 1] * 0x0100;
            skip = 2;  // Skip SLIP_I_CTRL_CNT
            break;
        case SP_CMD_WRITE:
            size += buffer[SP_I_WR_CNT + 0] +
                    buffer[SP_I_WR_CNT + 1] * 0x0100;
            break;
    }
    while (i < size) {
        send_byte(buffer[skip + i++]);
    }

    tud_cdc_write_char(END);
    tud_cdc_write_flush();
    printf("  %d Bytes\n", i);
}

static int recv_byte(bool idle) {
    static int state;

    if (idle) {
        state = STATE_IDL;
    }

    absolute_time_t time = get_absolute_time();

    while (true) {
        int data = tud_cdc_read_char();
        if (data == -1) {
            if (!connected) {
                state = STATE_IDL;
                printf("recv_byte() error: reset\n");
                return -1;
            }
            if (absolute_time_diff_us(time, get_absolute_time()) > (idle ? 5000 : 100) * 1000) {
                state = STATE_IDL;
                printf("recv_byte() error: timeout\n");
                return -1;
            }
            tud_task();
            continue;
        }

        switch (state) {
            case STATE_IDL:
                switch (data) {
                    case END:
                        state = STATE_ACT;
                        break;
                    default:
                        // Ignore
                }
                break;
            case STATE_ACT:
                switch (data) {
                    case END:
                        state = STATE_IDL;
                        return REQ_DONE;
                    case ESC:
                        state = STATE_ESC;
                        break;
                    default:
                        return data;
                }
                break;
            case STATE_ESC:
                switch (data) {
                    case ESC_END:
                        state = STATE_ACT;
                        return END;
                    case ESC_ESC:
                        state = STATE_ACT;
                        return ESC;
                    default:
                        state = STATE_IDL;
                        printf("recv_byte() error: slip state machine\n");
                        return -1;
                }
                break;
        }
    }
}

static void recv_response(uint8_t *buffer) {
    bool idle = true;
    int i = 0;
    int skip = 0;

//    printf("SLIP Receive Response(Seq=$%02X,Cmd=$%02X)\n", sequence_number, command);
    while (true) {
        int data = recv_byte(idle);
        switch (data) {
            case -1:
                buffer[SP_O_RETVAL] = SP_BUSERR;
                return;
            case REQ_DONE:
                if (skip) {
                    buffer[SP_O_STS_CNT + 0] = (i - SP_O_STS_CNT) % 0x0100;
                    buffer[SP_O_STS_CNT + 1] = (i - SP_O_STS_CNT) / 0x0100;
                }
                printf("  %d Bytes\n", i);
                return;
        }
        if (idle) {
            if (data == sequence_number) {
                idle = false;
            }
        } else {
            if (command == SP_CMD_STATUS && i == SP_O_STS_CNT) {
                skip = 2;  // Skip SP_O_STS_CNT
            }
            buffer[skip + i++] = data;
        }
    }
}

static void convert_protocol(uint8_t *buffer) {
    uint8_t sp_drive    = buffer[PRODOS_I_UNIT] >> 7;
    uint8_t sp_buff_lo  = buffer[PRODOS_I_BUFF + 0];
    uint8_t sp_buff_hi  = buffer[PRODOS_I_BUFF + 1];
    uint8_t sp_block_lo = buffer[PRODOS_I_BLOCK + 0];
    uint8_t sp_block_hi = buffer[PRODOS_I_BLOCK + 1];

    if ((buffer[PRODOS_I_UNIT] >> 4 & 0x07) != board_slot()) {
        sp_drive += 2;
    }

    // The 3 ProDOS commands (and their location) map 1:1 to the first 3 SmartPort commands
    switch (buffer[SP_I_CMD]) {
        case SP_CMD_STATUS:
//            printf("SLIP ProDOS Status(Drive=%d)\n", sp_drive);
            buffer[SP_I_STS_CODE] = SP_STATUS_STS;
            break;
        case SP_CMD_READBLK:
//            printf("SLIP ProDOS Read(Drive=%d,Block=$%04X)\n", sp_drive, sp_block_hi * 0x0100 + sp_block_lo);
            buffer[SP_I_BLK_BUFF + 0] = sp_buff_lo;  // For gen_read_block()
            buffer[SP_I_BLK_BUFF + 1] = sp_buff_hi;  // For gen_read_block()
            buffer[SP_I_BLK_BLOCK + 0] = sp_block_lo;
            buffer[SP_I_BLK_BLOCK + 1] = sp_block_hi;
            buffer[SP_I_BLK_BLOCK + 2] = 0;
            break;
        case SP_CMD_WRITEBLK:
//            printf("SLIP ProDOS Write(Drive=%d,Block=$%04X)\n", sp_drive, sp_block_hi * 0x0100 + sp_block_lo);
            memmove(&buffer[SP_I_BLK_BUFFER],
                    &buffer[PRODOS_I_BUFFER], BLK_SIZE);  // Needs to be first write access!
            buffer[SP_I_BLK_BLOCK + 0] = sp_block_lo;
            buffer[SP_I_BLK_BLOCK + 1] = sp_block_hi;
            buffer[SP_I_BLK_BLOCK + 2] = 0;
            break;
    }
    buffer[SP_I_PARAM_CNT] = 3;
    buffer[SP_I_UNIT     ] = sp_drive + 1;

    slip_command(CONTROL_SP, buffer);

    switch (command) {
        case SP_CMD_STATUS:
//            printf("  %u Blocks\n", buffer[SP_O_STS_LIST + 1] +
//                                    buffer[SP_O_STS_LIST + 2] * 0x0100);
            buffer[PRODOS_O_BUFFER + 0] = buffer[SP_O_STS_LIST + 1];
            buffer[PRODOS_O_BUFFER + 1] = buffer[SP_O_STS_LIST + 2];
            break;
        case SP_CMD_READBLK:
            // No memmove() here as the ProDOS buffer location maps 1:1 to the SmartPort location
            break;
    }
    // The ProDOS return value location maps 1:1 to the SmartPort location
    if (buffer[PRODOS_O_RETVAL] >= 0x50) {
        buffer[PRODOS_O_RETVAL] = PRODOS_SUCCESS;
    }
    if (buffer[PRODOS_O_RETVAL] == PRODOS_SUCCESS) {
        return;
    }
    if (buffer[PRODOS_O_RETVAL] < PRODOS_IO_ERROR ||
        buffer[PRODOS_O_RETVAL] > PRODOS_WRITE_PROT) {
        buffer[PRODOS_O_RETVAL] = PRODOS_IO_ERROR;
    }
}

void slip_command(uint8_t control, uint8_t *buffer) {
    if (control == CONTROL_PRODOS) {
        convert_protocol(buffer);
        return;
    }

    if (buffer[SP_I_UNIT]== 0) {
        switch (buffer[SP_I_CMD]) {
            case SP_CMD_STATUS:
                if (buffer[SP_I_STS_CODE] == SP_STATUS_STS) {
                    printf("SLIP CmdStatus(Device=Smartport)\n");
                    buffer[SP_O_STS_LIST + 0] = units;
                    buffer[SP_O_STS_LIST + 1] = 0b01000000;  // no interrupt sent
                    memset(&buffer[SP_O_STS_LIST + 2], 0x00, 6);
                    buffer[SP_O_STS_CNT + 0] = 8;  // size header low
                    buffer[SP_O_STS_CNT + 1] = 0;  // size header high
                    buffer[SP_O_RETVAL] = SP_SUCCESS;
                } else {
                    buffer[SP_O_RETVAL] = SP_BADCTL;
                }
                break;
            case SP_CMD_INIT:
                printf("SLIP CmdInit(Device=Smartport)\n");
                units = 0;
                while (true) {
                    units++;
                    buffer[SP_I_CMD      ] = SP_CMD_INIT;
                    buffer[SP_I_PARAM_CNT] = 1;
                    buffer[SP_I_UNIT     ] = units;
                    slip_command(control, buffer);
                    if (buffer[SP_O_RETVAL] != SP_SUCCESS) {
                        break;
                    }
                }
                units--;
                buffer[SP_O_RETVAL] = units ? SP_SUCCESS : SP_NODRIVE;
                printf("  %d units\n", units);
                break;
            default:
                buffer[SP_O_RETVAL] = SP_BADCMD;
        }
        return;
    }

    uint16_t buff = buffer[SP_I_BLK_BUFF + 0] +
                    buffer[SP_I_BLK_BUFF + 1] * 0x0100;
    
    send_request(buffer);
    recv_response(buffer);

    if (command == SP_CMD_READBLK) {
        gen_read_block(buff, &buffer[SP_O_BLK_BUFFER]);
    }

    sequence_number++;
}

void slip_task(void) {
    if (tud_cdc_connected()) {
        if (!connected) {
            connected = true;

            static uint8_t buffer[SP_REQ_SIZE];
            buffer[SP_I_CMD]       = SP_CMD_INIT;
            buffer[SP_I_PARAM_CNT] = 1;
            buffer[SP_I_UNIT]      = 0;

            printf("SLIP Connect\n");
            slip_command(CONTROL_SP, buffer);
        }
    } else {
        if (connected) {
            printf("SLIP Disconnect\n");
        }
        units = 0;
        connected = false;
    }
}
