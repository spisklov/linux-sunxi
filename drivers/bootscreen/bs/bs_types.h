/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __BS_TYPES_H__
#define __BS_TYPES_H__

#include <linux/types.h>

struct bs_data {
	void *data;
	size_t size;
	void (*free_data)(const struct bs_data *data);
};

#endif // __BS_TYPES_H__
