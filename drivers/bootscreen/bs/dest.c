// SPDX-License-Identifier: GPL-2.0

#include "client_manager.h"
#include "bootscreen.h"
#include "dest.h"
#include "log.h"

#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/slab.h>

struct cmd_node {
	struct list_head node;
	const struct cmd *cmd;
};

struct dest_device {
	const struct bs_client *client;
	struct completion completion;
	struct list_head node;
	struct task_struct *worker;

	spinlock_t cmd_lock;
	struct list_head cmd_head;
};

struct dest {
	struct list_head devices_head;
	spinlock_t device_lock;
	int listener_id;
};

static struct dest dest;


static struct cmd_node *create_cmd_node(const struct cmd *cmd
	, struct dest_device *device)
{
	struct cmd_node *cmd_node = kmalloc(sizeof(*cmd_node), GFP_KERNEL);

	if (!cmd_node)
		return NULL;

	cmd_node->cmd = cmd;
	list_add(&cmd_node->node, &device->cmd_head);
	complete(&device->completion);

	return cmd_node;
}


static void destroy_cmd_node(const struct cmd_node *cmd_node)
{
	cmd_node->cmd->destroy(cmd_node->cmd);
	kfree(cmd_node);
}


static int dest_device_worker(void *data)
{
	struct dest_device *device = (struct dest_device *)data;
	struct cmd_node *cmd_node;
	struct cmd_node *cmd1 = NULL, *cmd2 = NULL;
	struct list_head *pos, *n;

	LOG(KERN_DEBUG, "dest device worker starts(%dx%d)..."
		, (int) device->client->resolution.width
		, (int) device->client->resolution.height);

	while (!kthread_should_stop()) {
		wait_for_completion(&device->completion);

		spin_lock(&device->cmd_lock);
		list_for_each_safe(pos, n, &device->cmd_head) {
			cmd_node = list_entry(pos, struct cmd_node, node);
			if (cmd1) {
				if (cmd1->cmd->id == cmd_node->cmd->id) {
					list_del(pos);
					destroy_cmd_node(cmd_node);
					continue;
				}
			} else {
				list_del(pos);
				cmd1 = cmd_node;
				continue;
			}

			if (cmd2) {
				if (cmd2->cmd->id == cmd_node->cmd->id) {
					list_del(pos);
					destroy_cmd_node(cmd_node);
				}
			} else {
				list_del(pos);
				cmd2 = cmd_node;
			}
		}
		spin_unlock(&device->cmd_lock);

		if (cmd1) {
			cmd1->cmd->execute(cmd1->cmd);
			cmd1->cmd->destroy(cmd1->cmd);
		}

		if (cmd2) {
			cmd2->cmd->execute(cmd2->cmd);
			cmd2->cmd->destroy(cmd2->cmd);
		}
	}

	LOG(KERN_DEBUG, "dest device worker finishes(%dx%d)..."
		, (int)device->client->resolution.width
		, (int)device->client->resolution.height);

	return 0;
}


static struct dest_device *dest_create_device(const struct bs_client *client)
{
	struct dest_device *device = kmalloc(sizeof(*device), GFP_KERNEL);

	if (!device)
		return NULL;

	init_completion(&device->completion);
	spin_lock_init(&device->cmd_lock);
	INIT_LIST_HEAD(&device->cmd_head);
	device->client = client;
	device->worker = kthread_run(dest_device_worker
		, device, "device worker");

	return device;
}


static void on_client_added(const struct bs_client *client, void *data)
{
	struct dest_device *device = dest_create_device(client);

	if (!device) {
		LOG(KERN_ERR, "failed to create dest device");
		return;
	}

	list_add_tail(&device->node, &dest.devices_head);
}


int destination_create(void)
{
	struct cm_listener listener = {
		.on_client_added = on_client_added,
		.data = NULL,
	};

	INIT_LIST_HEAD(&dest.devices_head);
	spin_lock_init(&dest.device_lock);
	dest.listener_id = cm_add_listener(&listener);

	return dest.listener_id >= 0 ? 0 : dest.listener_id;
}


void destination_handle_cmd(const struct cmd *cmd)
{
	struct list_head *pos;
	struct dest_device *device;

	list_for_each(pos, &dest.devices_head) {
		device = list_entry(pos, struct dest_device, node);

		if (device->client == cmd->client) {
			spin_lock(&device->cmd_lock);
			create_cmd_node(cmd, device);
			spin_unlock(&device->cmd_lock);
			break;
		}
	}
}
