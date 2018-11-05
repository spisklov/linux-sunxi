/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __SSD1306_H__
#define __SSD1306_H__

#define SSD1306_SEND_COMMAND(c, d) i2c_smbus_write_byte_data((c), 0x00, (d))

#define SET_LOW_COLUMN        0x00
#define SET_HIGH_COLUMN       0x10
#define COLUMN_ADDR           0x21
#define PAGE_ADDR             0x22
#define SET_START_PAGE        0xB0
#define CHARGE_PUMP           0x8D
#define DISPLAY_OFF           0xAE
#define DISPLAY_ON            0xAF

#define MEMORY_MODE           0x20
#define SET_CONTRAST          0x81
#define SET_NORMAL_DISPLAY    0xA6
#define SET_INVERT_DISPLAY    0xA7
#define COM_SCAN_INC          0xC0
#define COM_SCAN_DEC          0xC8
#define SET_DISPLAY_OFFSET    0xD3
#define SET_DISPLAY_CLOCK_DIV 0xD5
#define SET_PRECHARGE         0xD9
#define SET_COM_PINS          0xDA
#define SET_VCOM_DETECT       0xDB

#define MEM_MODE_VERTICAL     0x01;

#endif // __SSD1306_H__
