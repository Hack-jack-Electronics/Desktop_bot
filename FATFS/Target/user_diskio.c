/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    user_diskio.c
  * @brief   User Disk I/O driver (links FatFs to sd_spi.c)
  ******************************************************************************
  */
 /* USER CODE END Header */

#ifdef USE_OBSOLETE_USER_CODE_SECTION_0
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
#endif

/* USER CODE BEGIN DECL */
#include <string.h>
#include "ff_gen_drv.h"
#include "user_diskio.h"
#include "sd_spi.h"

static volatile DSTATUS Stat = STA_NOINIT;
/* USER CODE END DECL */

DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status (BYTE pdrv);
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
  DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif
#if _USE_IOCTL == 1
  DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif

Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read,
#if  _USE_WRITE
  USER_write,
#endif
#if  _USE_IOCTL == 1
  USER_ioctl,
#endif
};

DSTATUS USER_initialize(BYTE pdrv)
{
    Stat = STA_NOINIT;
    if (SD_Init() == 0) {
        Stat &= ~STA_NOINIT;
    }
    return Stat;
}

DSTATUS USER_status(BYTE pdrv)
{
    return Stat;
}

DRESULT USER_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    if (Stat & STA_NOINIT) return RES_NOTRDY;
    if (!count) return RES_PARERR;

    for (UINT i = 0; i < count; i++) {
        if (SD_ReadBlock(buff + (i * 512), sector + i) != 0) {
            return RES_ERROR;
        }
    }
    return RES_OK;
}

#if _USE_WRITE == 1
DRESULT USER_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    if (Stat & STA_NOINIT) return RES_NOTRDY;
    if (!count) return RES_PARERR;

    for (UINT i = 0; i < count; i++) {
        if (SD_WriteBlock(buff + (i * 512), sector + i) != 0) {
            return RES_ERROR;
        }
    }
    return RES_OK;
}
#endif

#if _USE_IOCTL == 1
DRESULT USER_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    switch (cmd) {
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_COUNT:
        *(DWORD *)buff = 0;
        return RES_OK;
    case GET_SECTOR_SIZE:
        *(WORD *)buff = 512;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *(DWORD *)buff = 1;
        return RES_OK;
    default:
        return RES_PARERR;
    }
}
#endif
