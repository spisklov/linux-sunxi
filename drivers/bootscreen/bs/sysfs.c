// SPDX-License-Identifier: GPL-2.0

#include "sysfs.h"

#include "log.h"

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>


struct sysfs_nodes {
	struct class *fs_class;
	struct class_attribute sw2fb_attr;
};


struct sysfs_data {
	struct sysfs_nodes fs_nodes;
	bs_callback_t callback;
};

typedef ssize_t (*show_callback_t)(struct class *class
	, struct class_attribute *attr, char *buf);


static struct sysfs_data *fs_data;


static ssize_t show_bs(struct class *class
	, struct class_attribute *attr, char *buf)
{
	fs_data->callback();
	return 0;
}


int bs_create_sysfs_entry(bs_callback_t callback)
{
	int res = 0;

	do {
		WARN_ON(fs_data);

		if (!callback)
			return -EINVAL;

		fs_data = kmalloc(sizeof(*fs_data), GFP_KERNEL);
		if (!fs_data) {
			LOG(KERN_ERR, "failed to init data for sysfs entries");
			break;
		}

		fs_data->fs_nodes.fs_class = class_create(THIS_MODULE
			, KBUILD_MODNAME);
		if (IS_ERR(fs_data->fs_nodes.fs_class)) {
			LOG(KERN_ERR, "bad sysfs class");
			break;
		}

		memset(&fs_data->fs_nodes.sw2fb_attr, 0
			, sizeof(struct class_attribute));
		fs_data->fs_nodes.sw2fb_attr.attr.mode = 0444;
		fs_data->fs_nodes.sw2fb_attr.attr.name = "sw2fb";
		fs_data->fs_nodes.sw2fb_attr.show = show_bs;

		res = class_create_file(fs_data->fs_nodes.fs_class
			, &fs_data->fs_nodes.sw2fb_attr);
		if (res) {
			LOG(KERN_ERR, "failed to create sysfs attribute entry");
			break;
		}

		fs_data->callback = callback;
		return res;
	} while (0);

	bs_remove_sysfs_entry();
	return res;
}


void bs_remove_sysfs_entry(void)
{
	if (!fs_data)
		return;

	class_remove_file(fs_data->fs_nodes.fs_class
		, &fs_data->fs_nodes.sw2fb_attr);
	class_destroy(fs_data->fs_nodes.fs_class);
	kfree(fs_data);
	fs_data = NULL;
}
