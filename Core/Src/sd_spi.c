#include "sd_spi.h"

static uint8_t CardType = 0;

static void SPI5_SetSpeed(uint8_t prescaler)
{
    HAL_SPI_DeInit(&hspi5);
    hspi5.Init.BaudRatePrescaler = prescaler;
    HAL_SPI_Init(&hspi5);
}

static uint8_t SPIx_WriteRead(uint8_t byte)
{
    uint8_t rx = 0;
    HAL_SPI_TransmitReceive(&hspi5, &byte, &rx, 1, 100);
    return rx;
}

static uint8_t SD_ReadyWait(void)
{
    uint8_t res;
    uint32_t timeout = 0xFFFF;
    do {
        res = SPIx_WriteRead(0xFF);
    } while ((res != 0xFF) && timeout--);
    return res;
}

static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg)
{
    uint8_t n, res;

    if (cmd & 0x80) {
        cmd &= 0x7F;
        res = SD_SendCmd(CMD55, 0);
        if (res > 1) return res;
    }

    SD_DESELECT();
    SPIx_WriteRead(0xFF);
    SD_SELECT();
    SPIx_WriteRead(0xFF);

    SPIx_WriteRead(cmd);
    SPIx_WriteRead((uint8_t)(arg >> 24));
    SPIx_WriteRead((uint8_t)(arg >> 16));
    SPIx_WriteRead((uint8_t)(arg >> 8));
    SPIx_WriteRead((uint8_t)arg);

    uint8_t crc = 0x01;
    if (cmd == CMD0) crc = 0x95;
    if (cmd == CMD8) crc = 0x87;
    SPIx_WriteRead(crc);

    n = 10;
    do {
        res = SPIx_WriteRead(0xFF);
    } while ((res & 0x80) && --n);

    return res;
}

uint8_t SD_Init(void)
{
    uint8_t n, cmd, ty, ocr[4];
    uint16_t tmr;

    SPI5_SetSpeed(SPI_BAUDRATEPRESCALER_256);

    SD_DESELECT();
    for (n = 0; n < 10; n++) SPIx_WriteRead(0xFF);

    ty = 0;
    if (SD_SendCmd(CMD0, 0) == 1) {
        if (SD_SendCmd(CMD8, 0x1AA) == 1) {
            for (n = 0; n < 4; n++) ocr[n] = SPIx_WriteRead(0xFF);
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
                for (tmr = 10000; tmr && SD_SendCmd(ACMD41, 0x40000000); tmr--) ;
                if (tmr && SD_SendCmd(CMD58, 0) == 0) {
                    for (n = 0; n < 4; n++) ocr[n] = SPIx_WriteRead(0xFF);
                    ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;
                }
            }
        } else {
            if (SD_SendCmd(ACMD41, 0) <= 1) {
                ty = CT_SD1;
                for (tmr = 10000; tmr && SD_SendCmd(ACMD41, 0); tmr--) ;
            } else {
                ty = CT_MMC;
                for (tmr = 10000; tmr && SD_SendCmd(CMD1, 0); tmr--) ;
            }
            if (!tmr || SD_SendCmd(CMD16, 512) != 0) ty = 0;
        }
    }

    CardType = ty;
    SD_DESELECT();
    SPIx_WriteRead(0xFF);

    if (ty) {
        SPI5_SetSpeed(SPI_BAUDRATEPRESCALER_4);
        return 0;
    }
    return 1;
}

uint8_t SD_ReadBlock(uint8_t *buff, uint32_t lba)
{
    uint8_t res;
    uint16_t i;

    if (!(CardType & CT_BLOCK)) lba *= 512;

    res = SD_SendCmd(CMD17, lba);
    if (res != 0) return res;

    for (i = 50000; i; i--) {
        res = SPIx_WriteRead(0xFF);
        if (res == 0xFE) break;
    }
    if (res != 0xFE) return 1;

    for (i = 0; i < 512; i++) buff[i] = SPIx_WriteRead(0xFF);
    SPIx_WriteRead(0xFF);
    SPIx_WriteRead(0xFF);

    SD_DESELECT();
    SPIx_WriteRead(0xFF);
    return 0;
}

uint8_t SD_WriteBlock(const uint8_t *buff, uint32_t lba)
{
    uint8_t res;
    uint16_t i;

    if (!(CardType & CT_BLOCK)) lba *= 512;

    res = SD_SendCmd(CMD24, lba);
    if (res != 0) return res;

    SPIx_WriteRead(0xFF);
    SPIx_WriteRead(0xFE);

    for (i = 0; i < 512; i++) SPIx_WriteRead(buff[i]);
    SPIx_WriteRead(0xFF);
    SPIx_WriteRead(0xFF);

    res = SPIx_WriteRead(0xFF);
    if ((res & 0x1F) != 0x05) {
        SD_DESELECT();
        return 1;
    }

    for (i = 50000; i && !SPIx_WriteRead(0xFF); i--) ;

    SD_DESELECT();
    SPIx_WriteRead(0xFF);
    return 0;
}
