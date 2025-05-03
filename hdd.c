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
#include <rtc.h>
#include <f_util.h>
#include <hw_config.h>

#include "sp.h"

#include "hdd.h"

#define DRIVES      8
#define BLOCK_SIZE  512

#define SUCCESS     0x00
#define IO_ERROR    0x27
#define WRITE_PROT  0x2B

static FIL image[DRIVES];
static bool prot[DRIVES];

static uint16_t get_blocks(int drive) {
    FSIZE_t size = f_size(&image[drive]);

    if (!size) {
        static char name[12];
        sprintf(name, "drive%d.po", drive + 1);

        printf("HDD Open(Drive=%d,File=%s)\n", drive, name);

        FRESULT fr = f_open(&image[drive], name, FA_OPEN_EXISTING | FA_READ | FA_WRITE);
        if (fr == FR_DENIED) {
            printf("  Write-Protected\n");
            fr = f_open(&image[drive], name, FA_OPEN_EXISTING | FA_READ);
            prot[drive] = true;
        }
        if (fr != FR_OK) {
            printf("f_open(%s) error: %s (%d)\n", name, FRESULT_str(fr), fr);
        }

        size = f_size(&image[drive]);
    }

    return size / BLOCK_SIZE;
}

static bool seek_block(int drive, uint16_t block) {
    if (block >= get_blocks(drive)) {
        return false;
    }

    FRESULT fr = f_lseek(&image[drive], block * BLOCK_SIZE);
    if (fr != FR_OK) {
        printf("f_lseek(%d) error: %s (%d)\n", drive, FRESULT_str(fr), fr);
        return false;
    }

    return true;
}

void hdd_init(void) {
    time_init();

    sd_card_t *sd_card = sd_get_by_num(0);

    FRESULT fr = f_mount(&sd_card->fatfs, "SD:", 1);
    if (fr != FR_OK) {
        printf("f_mount(SD:) error: %s (%d)\n", FRESULT_str(fr), fr);
    }
}

void hdd_reset(void) {
    for (int drive = 0; drive < DRIVES; drive++) {
        if (!f_size(&image[drive])) {
            continue;
        }

        printf("HDD Close(Drive=%d)\n", drive);

        FRESULT fr = f_close(&image[drive]);
        if (fr != FR_OK) {
            printf("f_close(%d) error: %s (%d)\n", drive, FRESULT_str(fr), fr);
        }

        memset(&image[drive], 0, sizeof(image[drive]));
        prot[drive] = false;
    }
}

bool hdd_protected(uint8_t drive) {
    return prot[drive];
}

uint8_t hdd_status(uint8_t drive, uint8_t *data) {
//    printf("HDD Status(Drive=%d)\n", drive);

    uint16_t blocks = get_blocks(drive);

    if (!blocks) {
        return IO_ERROR;
    }

    data[0] = blocks % 0x100;
    data[1] = blocks / 0x100;

    return SUCCESS;
}

uint8_t hdd_read(uint8_t drive, uint16_t block, uint8_t *data) {
//    printf("HDD Read(Drive=%d,Block=$%04X)\n", drive, block);

    if (!seek_block(drive, block)) {
        return IO_ERROR;
    }

    UINT br;
    FRESULT fr = f_read(&image[drive], data, BLOCK_SIZE, &br);
    if (fr != FR_OK || br != BLOCK_SIZE) {
        printf("f_read(%d) error: %s (%d)\n", drive, FRESULT_str(fr), fr);
        return IO_ERROR;
    }

    return SUCCESS;
}


uint8_t hdd_write(uint8_t drive, uint16_t block, const uint8_t *data) {
//    printf("HDD Write(Drive=d,Block=$%04X)\n", drive, block);

    if (!seek_block(drive, block)) {
        return IO_ERROR;
    }

    UINT bw;
    FRESULT fr = f_write(&image[drive], data, BLOCK_SIZE, &bw);
    if (fr != FR_OK || bw != BLOCK_SIZE) {
        if (fr == FR_DENIED) {
            return WRITE_PROT;
        }
        printf("f_write(%d) error: %s (%d)\n", drive, FRESULT_str(fr), fr);
        return IO_ERROR;
    }

    fr = f_sync(&image[drive]);
    if (fr != FR_OK) {
        printf("f_sync(%d) error: %s (%d)\n", drive, FRESULT_str(fr), fr);
        return IO_ERROR;
    }

    return SUCCESS;
}
