// SPDX-License-Identifier: GPL-2.0

#include "anim_src.h"
#include "bootscreen.h"
#include "client_manager.h"
#include "dest.h"
#include "log.h"
#include "sysfs.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>


struct bootscreen {
	struct bs_source *src;
};

static struct bootscreen *bootscreen;


static void sysfs_callback(void)
{
	bs_remove_sysfs_entry();
	bootscreen->src->finalize(bootscreen->src);
	bootscreen->src->destroy(bootscreen->src);
	bootscreen->src = NULL;
}


static void __exit bootscreen_exit(void)
{
	if (!bootscreen)
		return;

	if (bootscreen->src)
		bootscreen->src->destroy(bootscreen->src);

	bs_remove_sysfs_entry();
	cm_destroy();
	kfree(bootscreen);
	bootscreen = NULL;
}


static int __init bootscreen_init(void)
{
	int res = 0;

	BUG_ON(bootscreen);

	do {
		bootscreen = kzalloc(sizeof(struct bootscreen), GFP_KERNEL);
		if (!bootscreen) {
			LOG(KERN_ERR, "can't init bootscreen");
			return -ENOMEM;
		}

		res = cm_initialize();
		if (res) {
			LOG(KERN_ERR, "failed to init client manager (%d)", res);
			break;
		}

		res = bs_create_sysfs_entry(sysfs_callback);
		if (res) {
			LOG(KERN_ERR, "failed to init sysfs entry");
			break;
		}

		destination_create();
		bootscreen->src = create_animation_source(destination_handle_cmd);
		bootscreen->src->start(bootscreen->src);

		return res;
	} while (0);

	bootscreen_exit();
	return res;
}


int bs_register_client(const struct bs_client *client)
{
	if (!client)
		return -EINVAL;

	cm_add_client(client);
	return 0;
}


int bs_unregister_client(const struct bs_client *client)
{
	LOG(KERN_ERR, "%s not implemented", __FUNCTION__);
	return -1;
}


module_init(bootscreen_init);
module_exit(bootscreen_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("bs");
MODULE_DESCRIPTION("Manages data displaying on primitive lcds");
MODULE_VERSION("0.1");
