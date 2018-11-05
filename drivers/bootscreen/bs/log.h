/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __LOG_H__
#define __LOG_H__

#define LOG(level, fmt, ...) \
	printk(level KBUILD_MODNAME ": " fmt "\n", ##__VA_ARGS__)

#endif // __LOG_H__
