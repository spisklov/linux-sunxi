// SPDX-License-Identifier: GPL-2.0

#include "ssd1306.h"

#include "bs/bootscreen.h"
#include "bs/log.h"

#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/init.h>
#include <linux/i2c.h>


#define DEVICE_NAME	"lcd_ssd1306"


struct screen {
	u8 width;
	u8 height;
};


struct ssd1306 {
	struct device *dev;
	struct i2c_client *i2c;
	struct screen screen;
};


static struct ssd1306 lcd = {
	.dev = NULL,
	.i2c = NULL,
	.screen = {
		.width = 128,
		.height = 64,
	},
};

static struct ssd1306_data {
	u8 type;
	u8 data[1024];
} lcd_data;


static void init_lcd(struct i2c_client *drv_client)
{
	dev_info(lcd.dev, "initialazing ssd1306\n");

	SSD1306_SEND_COMMAND(drv_client, DISPLAY_OFF);

	SSD1306_SEND_COMMAND(drv_client, MEMORY_MODE);
	SSD1306_SEND_COMMAND(drv_client, MEM_MODE_HORIZONTAL);

	SSD1306_SEND_COMMAND(drv_client, COLUMN_ADDR);
	SSD1306_SEND_COMMAND(drv_client, 0);
	SSD1306_SEND_COMMAND(drv_client, 127);

	SSD1306_SEND_COMMAND(drv_client, PAGE_ADDR);
	SSD1306_SEND_COMMAND(drv_client, 0);
	SSD1306_SEND_COMMAND(drv_client, 7);

	SSD1306_SEND_COMMAND(drv_client, COM_SCAN_DEC);

	SSD1306_SEND_COMMAND(drv_client, SET_CONTRAST);
	SSD1306_SEND_COMMAND(drv_client, 0xFF); /*Max contrast*/

	SSD1306_SEND_COMMAND(drv_client, 0xA1); /*set segment re-map 0 to 127*/
	SSD1306_SEND_COMMAND(drv_client, SET_NORMAL_DISPLAY);
	SSD1306_SEND_COMMAND(drv_client, 0xA8); /*set multiplex ratio(1 to 64)*/
	SSD1306_SEND_COMMAND(drv_client, 63);
	SSD1306_SEND_COMMAND(drv_client, 0xA4); /*0xA4 => follows RAM content*/
	SSD1306_SEND_COMMAND(drv_client, SET_DISPLAY_OFFSET);
	SSD1306_SEND_COMMAND(drv_client, 0x00); /*no offset*/
	SSD1306_SEND_COMMAND(drv_client, SET_DISPLAY_CLOCK_DIV);
	SSD1306_SEND_COMMAND(drv_client, 0x80); /*set divide ratio*/

	SSD1306_SEND_COMMAND(drv_client, SET_PRECHARGE);
	SSD1306_SEND_COMMAND(drv_client, 0x22);

	SSD1306_SEND_COMMAND(drv_client, SET_COM_PINS);
	SSD1306_SEND_COMMAND(drv_client, 0x12);

	SSD1306_SEND_COMMAND(drv_client, SET_VCOM_DETECT);
	SSD1306_SEND_COMMAND(drv_client, 0x20); /*0x20, 0.77x Vcc*/


	SSD1306_SEND_COMMAND(drv_client, 0x8D);
	SSD1306_SEND_COMMAND(drv_client, 0x14);
	SSD1306_SEND_COMMAND(drv_client, DISPLAY_ON);
}


static void lcd_display(const void *data, size_t size)
{
	int i, j, k, ret;

	if (!data) {
		dev_warn(lcd.dev, "not a data\n");
		return;
	}

	if (size != (lcd.screen.height * lcd.screen.width)) {
		dev_warn(lcd.dev, "wrong data size (%zu)\n", size);
		return;
	}

	for (i = 0; i < (lcd.screen.height / 8); i++) {
		for (j = 0; j < lcd.screen.width; j++) {
			u32 array_idx = i * lcd.screen.width + j;

			lcd_data.data[array_idx] = 0;
			for (k = 0; k < 8; k++) {
				u32 page_length = lcd.screen.width * i;
				u32 index = page_length
					+ (lcd.screen.width * k + j) / 8;
				u8 byte = *(((u8 *)data) + index);
				u8 bit = byte & (1 << (j % 8));

				bit = bit >> (j % 8);
				lcd_data.data[array_idx] |= bit << k;
			}
		}
	}

	lcd_data.type = 0x40;
	ret = i2c_master_send(lcd.i2c, (u8 *) &lcd_data, sizeof(lcd_data));
	if (unlikely(ret < 0))
		dev_warn(lcd.dev, "i2c_master_send() failed with error (%d)\n"
			, ret);
}


static void lcd_set_contrast(u8 contrast)
{
	SSD1306_SEND_COMMAND(lcd.i2c, SET_CONTRAST);
	SSD1306_SEND_COMMAND(lcd.i2c, contrast);
}


static int ssd1306_probe(struct i2c_client *drv_client,
	const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter;
	struct bs_client client = {
		.device = &drv_client->dev,
		.resolution = {
			.width = lcd.screen.width,
			.height = lcd.screen.height,
		},
		.display = lcd_display,
		.set_contrast = lcd_set_contrast,
	};

	lcd.dev = &drv_client->dev;
	lcd.i2c = drv_client;

	dev_info(lcd.dev, "init I2C driver\n");

	i2c_set_clientdata(drv_client, &lcd);

	adapter = drv_client->adapter;

	if (!adapter) {
		dev_err(lcd.dev, "adapter indentification error\n");
		return -ENODEV;
	}

	dev_info(lcd.dev, "I2C client address %d\n", drv_client->addr);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(lcd.dev, "operation not supported\n");
		return -ENODEV;
	}

	i2c_set_clientdata(drv_client, &lcd);

	init_lcd(drv_client);

	bs_register_client(&client);

	dev_info(lcd.dev, "ssd1306 driver successfully loaded\n");

	return 0;
}



static int ssd1306_remove(struct i2c_client *drv_client)
{
	SSD1306_SEND_COMMAND(drv_client, DISPLAY_OFF);
	dev_info(lcd.dev, "Goodbye, world!\n");
	return 0;
}


static const struct of_device_id ssd1306_match[] = {
	{ .compatible = "bs,lcd_ssd1306", },
	{ },
};

MODULE_DEVICE_TABLE(of, ssd1306_match);


static const struct i2c_device_id ssd1306_id[] = {
	{ "lcd_ssd1306", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ssd1306_id);


static struct i2c_driver ssd1306_driver = {
	.driver = {
		.name	= DEVICE_NAME,
		.of_match_table = ssd1306_match,
	},
	.probe    = ssd1306_probe,
	.remove   = ssd1306_remove,
	.id_table = ssd1306_id,
};

module_i2c_driver(ssd1306_driver);


MODULE_AUTHOR("Sergey Pisklov <sergey.pisklov@gmail.com>");
MODULE_DESCRIPTION("ssd1306 I2C");
MODULE_LICENSE("GPL");
