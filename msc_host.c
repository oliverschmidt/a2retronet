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

#include "hdd.h"

// From https://github.com/hathach/tinyusb/ examples/host/msc_file_explorer/src/msc_app.c

// Define the buffer to be place in USB/DMA memory with correct alignment/cache line size
CFG_TUH_MEM_SECTION static struct {
    TUH_EPBUF_TYPE_DEF(scsi_inquiry_resp_t, inquiry);
} scsi_resp;

static bool inquiry_complete_cb(uint8_t dev_addr, tuh_msc_complete_data_t const* cb_data) {
    msc_cbw_t const* cbw = cb_data->cbw;
    msc_csw_t const* csw = cb_data->csw;

    if (csw->status != 0) {
        printf("inquiry failed\n");
        return false;
    }

    // Print out Vendor ID, Product ID and Rev
    printf("  %.8s %.16s rev %.4s\n", scsi_resp.inquiry.vendor_id, scsi_resp.inquiry.product_id, scsi_resp.inquiry.product_rev);

    // Get capacity of device
    uint32_t const block_count = tuh_msc_get_block_count(dev_addr, cbw->lun);
    uint32_t const block_size = tuh_msc_get_block_size(dev_addr, cbw->lun);

    printf("  Block Count:%lu, Block Size:%lu\n", block_count, block_size);
    printf("  Disk Size:%" PRIu32 " MB\n", block_count / ((1024*1024) / block_size));

    hdd_mount_usb(true);
    return true;
}

void tuh_msc_mount_cb(uint8_t dev_addr) {
    printf("MSC Mount(Device=%d)\n", dev_addr);

    tuh_msc_inquiry(dev_addr, 0, &scsi_resp.inquiry, inquiry_complete_cb, 0);
}

void tuh_msc_umount_cb(uint8_t dev_addr) {
    printf("MSC Unmount(Device=%d)\n", dev_addr);

    hdd_mount_usb(false);
}
