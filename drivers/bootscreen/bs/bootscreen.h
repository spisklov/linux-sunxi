/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __BOOTSCREEN_H__
#define __BOOTSCREEN_H__

#include <linux/device.h>
#include <linux/types.h>


struct bs_resolution {
	u8 height;
	u8 width;
};

struct bs_client {
	struct device *device;
	struct bs_resolution resolution;
	void (*display)(const void *data, size_t size);
	void (*set_contrast)(u8 contrast);
};


int bs_register_client(const struct bs_client *client);
int bs_unregister_client(const struct bs_client *client);

#endif //__BOOTSCREEN_H__
