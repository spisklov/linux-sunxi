// SPDX-License-Identifier: GPL-2.0

#include "client_manager.h"
#include "fb_src.h"
#include "fb_dev_128x64.h"
#include "log.h"

#include "linux/slab.h"


struct fb_src {
	struct bs_source src;
	struct bs_fb_device *devs[1];
	int listener_id;
};


static void dummy(struct bs_source *src)
{}


static void destroy(struct bs_source *src)
{
	struct fb_src *thiz = (struct fb_src *)src;
	int i;

	cm_remove_listener(thiz->listener_id);
	for (i = 0; i < ARRAY_SIZE(thiz->devs); ++i) {
		if (thiz->devs[i])
			thiz->devs[i]->destroy(thiz->devs[i]);
	}

	kfree(src);
}


static void on_client_added(const struct bs_client *client, void *data)
{
	struct fb_src *thiz = (struct fb_src *)data;
	int i;

	for (i = 0; i < ARRAY_SIZE(thiz->devs); ++i) {
		if (thiz->devs[i])
			thiz->devs[i] = create_fb_dev_128x64(client);
	}
}


struct bs_source *create_fb_source(void)
{
	struct fb_src *src = kzalloc(sizeof(*src), GFP_KERNEL);
	struct cm_listener listener = {
		.on_client_added = on_client_added,
		.data = src,
	};

	if (!src) {
		LOG(KERN_ERR, "failed to allocate memory for fb source");
		return NULL;
	}

	src->src.start = dummy;
	src->src.finalize = dummy;
	src->src.destroy = destroy;

	src->listener_id = cm_add_listener(&listener);

	return (struct bs_source *) src;
}
