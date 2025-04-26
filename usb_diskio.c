/*

MIT License

Copyright (c) 2019 Ha Thach (tinyusb.org)

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

#include "usb_diskio.h"

// From https://github.com/hathach/tinyusb/ examples/host/msc_file_explorer/src/msc_app.c

static volatile bool disk_busy;

static void wait_for_disk_io(BYTE pdrv) {
    while(disk_busy) {
        tuh_task();
    }
}

static bool disk_io_complete(uint8_t dev_addr, tuh_msc_complete_data_t const * cb_data) {
    disk_busy = false;
    return true;
}

DSTATUS usb_disk_status(
    BYTE pdrv   // Physical drive number to identify the drive
) {
    return tuh_msc_mounted(pdrv) ? 0 : STA_NODISK;
}

DSTATUS usb_disk_initialize(
    BYTE pdrv   // Physical drive number to identify the drive
) {
    return 0;   // Nothing to do
}

DRESULT usb_disk_read(
    BYTE pdrv,      // Physical drive number to identify the drive
    BYTE *buff,     // Data buffer to store read data
    LBA_t sector,   // Start sector in LBA
    UINT count      // Number of sectors to read
) {
    disk_busy = true;

    tuh_msc_read10(pdrv, 0, buff, sector, count, disk_io_complete, 0);
    wait_for_disk_io(pdrv);

    return RES_OK;
}

#if FF_FS_READONLY == 0

DRESULT usb_disk_write(
    BYTE pdrv,          // Physical drive number to identify the drive
    const BYTE *buff,   // Data to be written
    LBA_t sector,       // Start sector in LBA
    UINT count          // Number of sectors to write
) {
    disk_busy = true;

    tuh_msc_write10(pdrv, 0, buff, sector, count, disk_io_complete, 0);
    wait_for_disk_io(pdrv);

    return RES_OK;
}

#endif

DRESULT usb_disk_ioctl(
    BYTE pdrv,  // Physical drive number to identify the drive
    BYTE cmd,   // Control code
    void *buff  // Buffer to send/receive control data
) {
    switch (cmd)
    {
      case CTRL_SYNC:
        // Nothing to do since we do block
        return RES_OK;

      case GET_SECTOR_COUNT:
        *(DWORD*)buff = tuh_msc_get_block_count(pdrv, 0);
        return RES_OK;

      case GET_SECTOR_SIZE:
        *(WORD*)buff = tuh_msc_get_block_size(pdrv, 0);
        return RES_OK;

      case GET_BLOCK_SIZE:
        *(DWORD*)buff = 1;  // Erase block size in units of sector size
        return RES_OK;

      default:
        return RES_PARERR;
    }

    return RES_OK;
}
