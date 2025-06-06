/**
 ******************************************************************************
 * @file: flash.c
 * @brief: external flash function for W25Q256JVEIQTR NOR Flash
 * @date: 27/08/2024
 * @author: GaoKong
 ******************************************************************************
**/

#include "flash.h"
#include <stdbool.h>

#include "io_cfg.h"
#include "sys_cfg.h"
#include "sys_ctrl.h"

/* WINBOND commands */
#define WINBOND_W_EN                        0x06    /* write enable */
#define WINBOND_W_DE                        0x04    /* write disable */
#define WINBOND_R_SR1                       0x05    /* read status reg 1 */
#define WINBOND_R_SR2                       0x35    /* read status reg 2 */
#define WINBOND_W_SR                        0x01    /* write status reg */
#define WINBOND_PAGE_PGM                    0x02    /* page program */
#define WINBOND_QPAGE_PGM                   0x32    /* quad input page program */
#define WINBOND_BLK_E_64K                   0xD8    /* block erase 64kb */
#define WINBOND_BLK_E_32K                   0x52    /* block erase 32kb */
#define WINBOND_SECTOR_E                    0x20    /* sector erase 4kb */
#define WINBOND_CHIP_ERASE                  0xC7    /* chip erase */
#define WINBOND_CHIP_ERASE2                 0x60    /* =chip_erase */
#define WINBOND_E_SUSPEND                   0x75    /* erase suspend */
#define WINBOND_E_RESUME                    0x7A    /* erase resume */
#define WINBOND_PDWN                        0xB9    /* power down */
#define WINBOND_HIGH_PERF_M                 0xA3    /* high performance mode */
#define WINBOND_CONT_R_RST                  0xFF    /* continuous read mode reset */
#define WINBOND_RELEASE                     0xAB    /* release power down or hpm/dev id (deprecated) */
#define WINBOND_R_MANUF_ID                  0x90    /* read manufacturer and dev id (deprecated) */
#define WINBOND_R_UNIQUE_ID                 0x4B    /* read unique id (suggested) */
#define WINBOND_R_JEDEC_ID                  0x9F    /* read jedec id = manuf+id (suggested) */
#define WINBOND_READ                        0x03
#define WINBOND_FAST_READ                   0x0B
#define WINBOND_READ_STATUS_REG_1           0x05

#define WINBOND_SR1_BUSY_MASK               0x01    /* status register 1 busy mask */
#define WINBOND_SR1_WEN_MASK                0x02    /* status register 1 write enable mask */

#define WINBOND_WINBOND_MANUF               0xEF    /* winbond manufacturer id */

#define WINBOND_DEFAULT_TIMEOUT             200     /* default timeout */

/* flash enable debug */
#define FLASH_DBG_EN						0

/******************************************************************************
* declare static function
*******************************************************************************/
static void flash_set_write_enable(bool);
static uint8_t flash_wait_to_idle();

/******************************************************************************
* define static function
*******************************************************************************/
void flash_set_write_enable(bool e) {
	flash_cs_low();

	if (e == true) {
		spi1_transfer(WINBOND_W_EN);
	}
	else {
		spi1_transfer(WINBOND_W_DE);
	}

	flash_cs_high();

	sys_ctrl_delay_us(100);
}

uint8_t flash_wait_to_idle() {
	uint8_t reg_1 = 0;
	uint32_t time_out_counter = 10000;	/* 10s */

	flash_cs_low();
	sys_ctrl_delay_us(100);

	spi1_transfer(WINBOND_READ_STATUS_REG_1);

	do {
		reg_1 = spi1_transfer(0x00);
		time_out_counter --;
		sys_ctrl_delay_us(100);
	} while ((reg_1 & 1) && time_out_counter);

	flash_cs_high();

	if (time_out_counter) {
		return FLASH_DRIVER_OK;
	}

	return FLASH_DRIVER_NG;
}

/******************************************************************************
* define public function
*******************************************************************************/
uint8_t flash_read(uint32_t address, uint8_t* pbuf, uint32_t len) {
	uint16_t i;

#if (FLASH_DBG_EN == 1)
	SYS_PRINT("[flash_read] add:0x%x\t%d\n", address, len);
#endif

	flash_wait_to_idle();

	flash_cs_low();

	spi1_transfer(WINBOND_READ);

	spi1_transfer(address >> 16);
	spi1_transfer(address >> 8);
	spi1_transfer(address);

	for(i = 0; i < len; i++) {
		pbuf[i] = spi1_transfer(0x00);
	}

	flash_cs_high();

	return FLASH_DRIVER_OK;
}

uint8_t flash_write(uint32_t address, uint8_t* pbuf, uint32_t len) {
	bool next_page_flag = true;
	uint32_t pbuf_index = 0;

#if (FLASH_DBG_EN == 1)
	SYS_PRINT("[flash_write] add:0x%x\t%d\n", address, len);
#endif

	while (len) {
		if (next_page_flag) {
			next_page_flag = false;

			flash_wait_to_idle();
			flash_set_write_enable(true);

			flash_cs_low();

			spi1_transfer(WINBOND_PAGE_PGM);

			spi1_transfer((address >> 16) & 0xff);
			spi1_transfer((address >> 8)  & 0xff);
			spi1_transfer((address >> 0)  & 0xff);
		}

		spi1_transfer(pbuf[pbuf_index++]);

		address++;
		len--;

		if ((address & 0xff) == 0) {
			next_page_flag = true;
			if (len) {
				flash_cs_high();
			}
		}
	}

	flash_cs_high();

	return flash_wait_to_idle();
}

uint8_t flash_erase_sector(uint32_t address) {

	if (address % 0x1000) {
		return FLASH_DRIVER_NG;
	}

#if (FLASH_DBG_EN == 1)
	SYS_PRINT("[flash_erase_sector] add:0x%x\n", address);
#endif

	flash_wait_to_idle();
	flash_set_write_enable(true);

	flash_cs_low();

	spi1_transfer(WINBOND_SECTOR_E);

	spi1_transfer(address >> 16);
	spi1_transfer(address >> 8);
	spi1_transfer(address);

	flash_cs_high();

	sys_ctrl_delay_us(100);

	return flash_wait_to_idle();
}

uint8_t flash_erase_block_32k(uint32_t address) {
	if (address % 0x8000) {
		return FLASH_DRIVER_NG;
	}

#if (FLASH_DBG_EN == 1)
	SYS_PRINT("[flash_erase_block_32k] add:0x%x\n", address);
#endif

	flash_wait_to_idle();
	flash_set_write_enable(true);

	flash_cs_low();

	spi1_transfer(WINBOND_BLK_E_32K);

	spi1_transfer(address >> 16);
	spi1_transfer(address >> 8);
	spi1_transfer(address);

	flash_cs_high();

	sys_ctrl_delay_us(100);

	return flash_wait_to_idle();
}

uint8_t flash_erase_block_64k(uint32_t address) {
	if (address % 0x10000) {
		return FLASH_DRIVER_NG;
	}

#if (FLASH_DBG_EN == 1)
	SYS_PRINT("[flash_erase_block_64k] add:0x%x\n", address);
#endif

	flash_wait_to_idle();
	flash_set_write_enable(true);

	flash_cs_low();

	spi1_transfer(WINBOND_BLK_E_64K);

	spi1_transfer(address >> 16);
	spi1_transfer(address >> 8);
	spi1_transfer(address);

	flash_cs_high();

	sys_ctrl_delay_us(100);

	return flash_wait_to_idle();
}

uint8_t  flash_erase_full() {
#if (FLASH_DBG_EN == 1)
	SYS_PRINT("[flash_erase_full]\n");
#endif

	flash_wait_to_idle();
	flash_set_write_enable(true);

	flash_cs_low();

	spi1_transfer(WINBOND_CHIP_ERASE2);

	flash_cs_high();

	sys_ctrl_delay_us(100);

	return flash_wait_to_idle();
}