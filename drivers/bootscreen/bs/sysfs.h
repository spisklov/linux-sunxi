/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __BS_SYSFS_H__
#define __BS_SYSFS_H__

typedef void (*bs_callback_t)(void);

int bs_create_sysfs_entry(bs_callback_t callback);
void bs_remove_sysfs_entry(void);

#endif // __BS_SYSFS_H__
