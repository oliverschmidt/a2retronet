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

#include <tusb.h>

#include <ff.h>
#include <diskio.h>

#include "hdd.h"

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
//    printf("MSC Read(LBA=$%08X)\n", lba);

    if (offset || bufsize % FF_MAX_SS) {
        printf("read param error\n");
        return -1;
    }

    if (disk_read(0, buffer, lba, bufsize / FF_MAX_SS) != RES_OK) {
        printf("disk_read() error\n");
        return -1;
    }

    return bufsize;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
//    printf("MSC Write(LBA=$%08X)\n", lba);

    if (offset || bufsize % FF_MAX_SS) {
        printf("write param error\n");
        return -1;
    }

    // Avoid inconsistency in local FAT implementation
    hdd_reset();

    if (disk_write(0, buffer, lba, bufsize / FF_MAX_SS) != RES_OK) {
        printf("disk_write() error\n");
        return -1;
    }

    return bufsize;
}

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
    printf("MSC Inquiry\n");

    const char vid[] = "A2retro";
    const char pid[] = "A2retroNET";
    const char rev[] = "1.0";

    memcpy(vendor_id,   vid, strlen(vid));
    memcpy(product_id,  pid, strlen(pid));
    memcpy(product_rev, rev, strlen(rev));
}

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
//    printf("MSC Ready\n");
    return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
    printf("MSC Capacity\n");

    *block_size = FF_MAX_SS;

    LBA_t sector_count;
    if (disk_ioctl(0, GET_SECTOR_COUNT, &sector_count) != RES_OK) {
        printf("disk_ioctl() error\n");
        *block_count = 0;
        return;
    }

    *block_count = sector_count;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
    if (scsi_cmd[0] == SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL) {

        printf("MSC Medium\n");

        // Host is about to read/write etc... better not to disconnect disk
        return 0;
    }

    printf("MSC Other\n");

    // Set Sense = Invalid Command Operation
    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

    // Negative means error -> TinyUSB could stall and/or response with failed status
    return -1;
}
