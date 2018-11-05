// SPDX-License-Identifier: GPL-2.0

#include "client_manager.h"
#include "log.h"

#include <linux/err.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>


struct client {
	const struct bs_client *client;
	struct list_head node;
};

struct listener {
	struct cm_listener listener;
	int id;
	struct list_head node;
};

struct cm {
	spinlock_t lock;
	struct list_head clients;
	size_t clients_size;
	struct list_head listeners;
	size_t listeners_size;
	int listener_id;
};


static struct cm cm;


static void destroy_clients(void)
{
	struct client *client;
	struct list_head *pos, *n;

	list_for_each_safe(pos, n, &cm.clients) {
		client = list_entry(pos, struct client, node);
		list_del(pos);
		kfree(client);
	}
}


static void destroy_listeners(void)
{
	struct listener *listener;
	struct list_head *pos, *n;

	list_for_each_safe(pos, n, &cm.listeners) {
		listener = list_entry(pos, struct listener, node);
		list_del(pos);
		kfree(listener);
	}
}


int cm_initialize(void)
{
	if (cm.clients_size || cm.listeners_size || cm.listener_id)
		return -EALREADY;

	spin_lock_init(&cm.lock);
	INIT_LIST_HEAD(&cm.clients);
	INIT_LIST_HEAD(&cm.listeners);
	cm.listener_id = 0;
	return 0;
}


void cm_destroy(void)
{
	destroy_clients();
	destroy_listeners();
	cm.listener_id = 0;
}


void cm_add_client(const struct bs_client *client)
{
	struct client *c;
	struct list_head *pos;
	struct listener *l;

	if (!client)
		return;

	spin_lock(&cm.lock);

	do {
		c = (struct client *)kmalloc(sizeof(*c), GFP_KERNEL);
		if (!c)
			break;

		c->client = client;
		list_add_tail(&c->node, &cm.clients);

		//Deadlock may happen here
		list_for_each(pos, &cm.listeners) {
			l = list_entry(pos, struct listener, node);
			l->listener.on_client_added(client, l->listener.data);
		}
	} while (0);
	spin_unlock(&cm.lock);
}


int cm_add_listener(const struct cm_listener *listener)
{
	int res = 0;
	struct listener *l;

	if (!listener)
		return -EINVAL;

	spin_lock(&cm.lock);

	do {
		l = (struct listener *)kmalloc(sizeof(*l), GFP_KERNEL);
		if (!l) {
			res = -ENOMEM;
			break;
		}

		res = ++cm.listener_id;
		memcpy(&l->listener, listener, sizeof(*listener));
		l->id = res;
		list_add_tail(&l->node, &cm.listeners);
	} while (0);

	spin_unlock(&cm.lock);

	return res;
}


void cm_remove_listener(int listener_id)
{
	struct listener *l;
	struct list_head *pos, *n;

	spin_lock(&cm.lock);
	list_for_each_safe(pos, n, &cm.listeners) {
		l = list_entry(pos, struct listener, node);
		if (l->id == listener_id) {
			list_del(pos);
			kfree(l);
			break;
		}
	}
	spin_unlock(&cm.lock);
}
