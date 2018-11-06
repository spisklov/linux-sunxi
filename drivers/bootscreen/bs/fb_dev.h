/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __BS_FRAMEBUFFER_DEVICE_H__
#define __BS_FRAMEBUFFER_DEVICE_H__

#include "bootscreen.h"

struct bs_fb_device {
	const struct bs_client *cleint;
	void (*destroy)(struct bs_fb_device *fb_dev);
};

#endif //__BS_FRAMEBUFFER_DEVICE_H__
