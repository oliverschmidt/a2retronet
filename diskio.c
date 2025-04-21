/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"         /* Obtains integer types */
#include "diskio.h"     /* Declarations of disk functions */
#include "glue.h"       /* Declarations of SD card functions */

/* Definitions of physical drive number for each drive */
#define DEV_SD      0   /* Example: Map MMC/SD card to physical drive 0 */
#define DEV_USB     1   /* Example: Map USB MSD to physical drive 1 */


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
    BYTE pdrv       /* Physical drive nmuber to identify the drive */
)
{
    switch (pdrv) {
    case DEV_SD :
        return sd_disk_status(DEV_SD);
    case DEV_USB :
//        return usb_disk_status(DEV_USB);
    }

    return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
    BYTE pdrv               /* Physical drive nmuber to identify the drive */
)
{
    switch (pdrv) {
    case DEV_SD :
        return sd_disk_initialize(DEV_SD);
    case DEV_USB :
//        return usb_disk_initialize(DEV_USB);
    }

    return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
    BYTE pdrv,      /* Physical drive nmuber to identify the drive */
    BYTE *buff,     /* Data buffer to store read data */
    LBA_t sector,   /* Start sector in LBA */
    UINT count      /* Number of sectors to read */
)
{
    switch (pdrv) {
    case DEV_SD :
        return sd_disk_read(DEV_SD, buff, sector, count);
    case DEV_USB :
//        return usb_disk_read(DEV_USB, buff, sector, count);
    }

    return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
    BYTE pdrv,          /* Physical drive nmuber to identify the drive */
    const BYTE *buff,   /* Data to be written */
    LBA_t sector,       /* Start sector in LBA */
    UINT count          /* Number of sectors to write */
)
{
    switch (pdrv) {
    case DEV_SD :
        return sd_disk_write(DEV_SD, buff, sector, count);
    case DEV_USB :
//        return usb_disk_write(DEV_USB, buff, sector, count);
    }

    return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
    BYTE pdrv,      /* Physical drive nmuber (0..) */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{
    switch (pdrv) {
    case DEV_SD :
        return sd_disk_ioctl(DEV_SD, cmd, buff);
    case DEV_USB :
//        return usb_disk_ioctl(DEV_USB, cmd, buff);
    }

    return RES_PARERR;
}
