#ifndef SD_SPI_H
#define SD_SPI_H

#include "main.h"
#include "fatfs.h"

extern SPI_HandleTypeDef hspi5;

#define SD_SPI_HANDLE     hspi5

#define SD_CS_PORT        GPIOB
#define SD_CS_PIN         GPIO_PIN_2   /* <-- changed from PB1 */

#define SD_SELECT()       HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET)
#define SD_DESELECT()     HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET)

#define CMD0     (0x40+0)
#define CMD1     (0x40+1)
#define CMD8     (0x40+8)
#define CMD9     (0x40+9)
#define CMD10    (0x40+10)
#define CMD12    (0x40+12)
#define CMD16    (0x40+16)
#define CMD17    (0x40+17)
#define CMD24    (0x40+24)
#define CMD55    (0x40+55)
#define CMD58    (0x40+58)
#define ACMD41   (0xC0+41)

#define CT_MMC      0x01
#define CT_SD1      0x02
#define CT_SD2      0x04
#define CT_SDC      (CT_SD1|CT_SD2)
#define CT_BLOCK    0x08

uint8_t SD_Init(void);
uint8_t SD_ReadBlock(uint8_t *buff, uint32_t lba);
uint8_t SD_WriteBlock(const uint8_t *buff, uint32_t lba);

#endif
