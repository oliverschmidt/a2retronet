/*-----------------------------------------------------------------------/
/  Low level SD card interface modlue include file   (C)ChaN, 2019       /
/-----------------------------------------------------------------------*/

#ifndef _GLUE_DEFINED
#define _GLUE_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------*/
/* Prototypes for SD card control functions */


DSTATUS sd_disk_initialize(BYTE pdrv);
DSTATUS sd_disk_status(BYTE pdrv);
DRESULT sd_disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count);
DRESULT sd_disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count);
DRESULT sd_disk_ioctl(BYTE pdrv, BYTE cmd, void* buff);

#ifdef __cplusplus
}
#endif

#endif
