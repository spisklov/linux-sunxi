// SPDX-License-Identifier: GPL-2.0

#include "anim_src.h"
#include "anim_logo_64x128.h"
#include "client_manager.h"
#include "cmd.h"
#include "log.h"

#include "linux/list.h"
#include "linux/sched.h"
#include "linux/slab.h"
#include "linux/spinlock.h"
#include "linux/kthread.h"


struct anim_item {
	struct anim_logo *logo;
	struct list_head node;
};

struct anim_source {
	struct bs_source ds;
	struct list_head anim_src_list;
	struct task_struct *worker;
	cmd_handler_t handler;
	int listener_id;
	spinlock_t data_lock;
	spinlock_t api_lock;
};


static void destroy_item(struct anim_item *item)
{
	item->logo->destroy(item->logo);
	kfree(item);
}


static void lock(spinlock_t *lock)
{
	spin_lock(lock);
}


static void unlock(spinlock_t *lock)
{
	spin_unlock(lock);
}


static int read_data_worker(void *data)
{
	struct anim_source *thiz = (struct anim_source *)data;
	struct bs_data frame;
	struct anim_item *item;
	struct list_head *pos;
	const struct cmd *cmd;

	LOG(KERN_DEBUG, "animation thread started...");
	while (!kthread_should_stop()) {
		lock(&thiz->data_lock);
		list_for_each(pos, &thiz->anim_src_list) {
			item = list_entry(pos, struct anim_item, node);
			item->logo->next(item->logo, &frame);
			cmd = cmd_create_display(item->logo->client, &frame);
			thiz->handler(cmd);
		}
		unlock(&thiz->data_lock);

		schedule_timeout(msecs_to_jiffies(200));
	}

	LOG(KERN_DEBUG, "animation thread stopped...");
	return 0;
}


static int fade_worker(void *data)
{
	struct anim_source *thiz = (struct anim_source *)data;
	u8 contrast = 100;
	struct anim_item *item;
	struct list_head *pos;
	const struct cmd *cmd;

	LOG(KERN_DEBUG, "fade thread started...");
	while (contrast) {
		lock(&thiz->data_lock);
		list_for_each(pos, &thiz->anim_src_list) {
			item = list_entry(pos, struct anim_item, node);
			cmd = cmd_create_set_contrast(item->logo->client
				, contrast);
			thiz->handler(cmd);
		}
		unlock(&thiz->data_lock);

		schedule_timeout(msecs_to_jiffies(100));
		contrast -= 10;
	}

	LOG(KERN_DEBUG, "fade thread stopped...");
	return 0;
}


static struct anim_logo *create_anim_logo(const struct bs_client *client)
{
	if (128 == client->resolution.width && 64 == client->resolution.height)
		return create_animation_logo_64x128(client);

	return NULL;
}


static void on_client_added(const struct bs_client *client, void *data)
{
	struct anim_source *thiz = (struct anim_source *)data;
	struct anim_logo *logo;
	struct anim_item *item;

	if (!client)
		return;

	lock(&thiz->api_lock);
	lock(&thiz->data_lock);

	do {
		logo = create_anim_logo(client);
		if (!logo) {
			LOG(KERN_WARNING, "unsupported client (res: %ux%u)"
			, client->resolution.width
			, client->resolution.height);
			break;
		}

		item = kmalloc(sizeof(*item), GFP_KERNEL);
		if (!item) {
			LOG(KERN_ERR, "can't allocate memory got anim item");
			break;
		}

		item->logo = logo;
		list_add_tail(&item->node, &thiz->anim_src_list);
	} while (0);

	unlock(&thiz->data_lock);
	unlock(&thiz->api_lock);
}


static void start(struct bs_source *src)
{
	struct anim_source *thiz = (struct anim_source *)src;

	lock(&thiz->api_lock);
	if (!thiz->worker)
		thiz->worker = kthread_run(read_data_worker
			, thiz, "animation thread");
	unlock(&thiz->api_lock);
}


static void finalize(struct bs_source *src)
{
	struct anim_source *thiz = (struct anim_source *)src;

	lock(&thiz->api_lock);
	if (thiz->worker) {
		kthread_stop(thiz->worker);
		free_kthread_struct(thiz->worker);
	}

	thiz->worker = kthread_run(fade_worker, thiz, "fade thread");
	unlock(&thiz->api_lock);
}


static void destroy(struct bs_source *src)
{
	struct anim_source *thiz = (struct anim_source *)src;
	struct list_head *pos, *n;
	struct anim_item *item;

	cm_remove_listener(thiz->listener_id);

	if (thiz->worker) {
		kthread_stop(thiz->worker);
		free_kthread_struct(thiz->worker);
	}

	list_for_each_safe(pos, n, &thiz->anim_src_list) {
		item = list_entry(pos, struct anim_item, node);
		list_del(pos);
		destroy_item(item);
	}

	kfree(thiz);
}


struct bs_source *create_animation_source(cmd_handler_t handler)
{
	struct anim_source *source;
	struct cm_listener listener = {
		.on_client_added = on_client_added
	};

	if (!handler)
		return NULL;

	source = kmalloc(sizeof(*source), GFP_KERNEL);
	if (!source)
		return NULL;

	source->ds.start = start;
	source->ds.finalize = finalize;
	source->ds.destroy = destroy;

	INIT_LIST_HEAD(&source->anim_src_list);

	source->worker = NULL;
	source->handler = handler;

	spin_lock_init(&source->data_lock);
	spin_lock_init(&source->api_lock);

	listener.data = source;
	source->listener_id = cm_add_listener(&listener);

	return (struct bs_source *)source;
}
