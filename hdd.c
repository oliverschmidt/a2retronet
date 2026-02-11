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

#include "config.h"
#include "sp.h"

#include "hdd.h"

#define BLOCK_SIZE  512

#define SUCCESS     0x00
#define IO_ERROR    0x27
#define WRITE_PROT  0x2B

static bool sd, usb;

static struct {
    FIL      image;
    uint16_t offset;
    uint16_t blocks;
    bool     error;
    bool     prot;
    uint8_t  async;
} hdd[MAX_DRIVES];

static struct {
             bool     valid;
             int      drive;
             uint16_t block;
__aligned(4) uint8_t  data[BLOCK_SIZE];
} prefetch;

static struct {
             bool     valid;
             int      drive;
             uint16_t block;
__aligned(4) uint8_t  data[BLOCK_SIZE];
} async;

static uint16_t get_blocks(int drive) {
    if (!hdd[drive].error && f_size(&hdd[drive].image) == 0) {
        char *path = config_drivepath(drive);

        printf("HDD Open(Drive=%d,File=%s)\n", drive, path);

        FRESULT fr = f_open(&hdd[drive].image, path, FA_OPEN_EXISTING | FA_READ | FA_WRITE);
        if (fr == FR_DENIED) {
            printf("  Write-Protected\n");
            fr = f_open(&hdd[drive].image, path, FA_OPEN_EXISTING | FA_READ);
            hdd[drive].prot = true;
        }
        if (fr != FR_OK) {
            printf("f_open(%s) error: %s (%d)\n", path, FRESULT_str(fr), fr);
            hdd[drive].error = true;
        }

        char *extension = strrchr(path, '.');
        if (extension && strcasecmp(extension, ".2mg") == 0) {
            hdd[drive].offset = 0x40;
            printf("  2MG\n");
        }

        int32_t raw_blocks = (f_size(&hdd[drive].image) - hdd[drive].offset) / BLOCK_SIZE;
        hdd[drive].blocks = raw_blocks > 0xFFFF ? 0xFFFF: raw_blocks;
        printf("  %u Blocks\n", hdd[drive].blocks);
    }

    return hdd[drive].blocks;
}

static bool seek_block(int drive, uint16_t block) {
    if (block >= get_blocks(drive)) {
        return false;
    }

    FRESULT fr = f_lseek(&hdd[drive].image, hdd[drive].offset + block * BLOCK_SIZE);
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
        return;
    }

    sd = true;
}

void hdd_reset(void) {
    for (int drive = 0; drive < MAX_DRIVES; drive++) {
        if (f_size(&hdd[drive].image) == 0) {
            continue;
        }

        printf("HDD Close(Drive=%d)\n", drive);

        FRESULT fr = f_close(&hdd[drive].image);
        if (fr != FR_OK) {
            printf("f_close(%d) error: %s (%d)\n", drive, FRESULT_str(fr), fr);
        }
    }
    memset(hdd, 0, sizeof(hdd));
    prefetch.valid = false;
    async.valid    = false;
}

void hdd_mount_usb(bool mount) {
    // Make sure the upcoming new default drive is actually used
    hdd_reset();
    config_reset();

    if (mount) {
        static FATFS fatfs;

        FRESULT fr = f_mount(&fatfs, "USB:", 1);
        if (fr != FR_OK) {
            printf("f_mount(USB:) error: %s (%d)\n", FRESULT_str(fr), fr);
            return;
        }

        fr = f_chdrive("USB:");
        if (fr != FR_OK) {
            printf("f_chdrive(USB:) error: %s (%d)\n", FRESULT_str(fr), fr);
        }

        usb = true;
    } else {

        FRESULT fr = f_chdrive("SD:");
        if (fr != FR_OK) {
            printf("f_chdrive(SD:) error: %s (%d)\n", FRESULT_str(fr), fr);
        }    

        fr = f_unmount("USB:");
        if (fr != FR_OK) {
            printf("f_unmount(USB:) error: %s (%d)\n", FRESULT_str(fr), fr);
        }

        usb = false;
    }
}

bool hdd_sd_mounted(void) {
    return sd;
}

bool hdd_usb_mounted(void) {
    return usb;
}

bool hdd_protected(uint8_t drive) {
    return hdd[drive].prot;
}

uint8_t hdd_status(uint8_t drive, uint8_t *data) {
//    printf("HDD Status(Drive=%d)\n", drive);

    uint16_t blocks = get_blocks(drive);

    if (blocks == 0) {
        return IO_ERROR;
    }

    data[0] = blocks % 0x100;
    data[1] = blocks / 0x100;

    return SUCCESS;
}

void hdd_prefetch(void) {
    if (prefetch.valid) {
        return;
    }

    if (!seek_block(prefetch.drive, prefetch.block)) {
        return;
    }

    UINT br;
    FRESULT fr = f_read(&hdd[prefetch.drive].image, prefetch.data, BLOCK_SIZE, &br);
    if (fr != FR_OK || br != BLOCK_SIZE) {
        return;
    }

    prefetch.valid = true;
}

uint8_t hdd_read(uint8_t drive, uint16_t block, uint8_t *data) {
//    printf("HDD Read(Drive=%d,Block=$%04X) - ", drive, block);

    if (prefetch.valid &&
        prefetch.drive == drive &&
        prefetch.block == block) {

//        printf("Prefetch\n");
        prefetch.valid = false;
        prefetch.block++;
        memcpy(data, prefetch.data, BLOCK_SIZE);
        return SUCCESS;
    }

    if (!seek_block(drive, block)) {
        return IO_ERROR;
    }

    UINT br;
    FRESULT fr = f_read(&hdd[drive].image, data, BLOCK_SIZE, &br);
    if (fr != FR_OK || br != BLOCK_SIZE) {
        printf("f_read(%d) error: %s (%d)\n", drive, FRESULT_str(fr), fr);
        return IO_ERROR;
    }

//    printf("Realtime\n");
    prefetch.valid = false;
    prefetch.drive = drive;
    prefetch.block = block + 1;

    return SUCCESS;
}

uint8_t hdd_write(uint8_t drive, uint16_t block, const uint8_t *data) {
//    printf("HDD Write(Drive=%d,Block=$%04X)\n", drive, block);

    // Check for pending error from last async write
    if (hdd[drive].async) {
        uint8_t error = hdd[drive].async;
        hdd[drive].async = SUCCESS;
        return error;
    }

    async.valid = true;
    async.drive = drive;
    async.block = block;
    memcpy(async.data, data, BLOCK_SIZE);

    return SUCCESS;
}

void hdd_async(void) {
    if (!async.valid) {
        return;
    }
    async.valid = false;

    if (!seek_block(async.drive, async.block)) {
        hdd[async.drive].async = IO_ERROR;
        return;
    }

    UINT bw;
    FRESULT fr = f_write(&hdd[async.drive].image, async.data, BLOCK_SIZE, &bw);
    if (fr != FR_OK || bw != BLOCK_SIZE) {
        if (fr == FR_DENIED) {
            hdd[async.drive].async = WRITE_PROT;
            return;
        }
        printf("f_write(%d) error: %s (%d)\n", async.drive, FRESULT_str(fr), fr);
        hdd[async.drive].async = IO_ERROR;
        return;
    }

    fr = f_sync(&hdd[async.drive].image);
    if (fr != FR_OK) {
        printf("f_sync(%d) error: %s (%d)\n", async.drive, FRESULT_str(fr), fr);
        hdd[async.drive].async = IO_ERROR;
        return;
    }

    hdd[async.drive].async = SUCCESS;
}
