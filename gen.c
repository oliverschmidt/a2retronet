/*

MIT License

Copyright (c) 2025 Oliver Schmidt (https://a2retro.de/)

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

#include "firmware.h"

#include "gen.h"

static void gen_read_page(uint16_t addr, const uint8_t *buffer, int offset) {
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

void gen_read_block(uint16_t addr, const uint8_t *buffer) {
//    printf("GEN Read(Addr=$%04X)\n", addr);
    gen_read_page(addr + 0x0000, buffer + 0x0000, OFFSET_BANK_2);
    gen_read_page(addr + 0x0100, buffer + 0x0100, OFFSET_BANK_3);
}
