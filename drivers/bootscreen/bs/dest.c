// SPDX-License-Identifier: GPL-2.0

#include "client_manager.h"
#include "bootscreen.h"
#include "dest.h"
#include "log.h"

#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/slab.h>


struct commands {
	const struct cmd *cmds[2];
};

struct dest_device {
	const struct bs_client *client;
	struct completion completion;
	struct list_head node;
	struct task_struct *worker;
	struct commands commands;

	spinlock_t cmd_lock;
};

struct dest {
	struct list_head devices_head;
	spinlock_t device_lock;
	int listener_id;
};


static struct dest dest;


static void on_cmd(const struct cmd *cmd, struct dest_device *device)
{
	int i = 0;
	u8 found = 0;
	struct commands *commands = &device->commands;

	if (!cmd) {
		LOG(KERN_WARNING, "command is null");
		return;
	}

	for (; i < ARRAY_SIZE(commands->cmds); ++i) {
		if (commands->cmds[i] == NULL) {
			found = 1;
			commands->cmds[i] = cmd;
			complete(&device->completion);
			break;
		} else if (commands->cmds[i]->id == cmd->id) {
			found = 1;
			commands->cmds[i]->destroy(commands->cmds[i]);
			commands->cmds[i] = cmd;
			complete(&device->completion);
			break;
		}
	}

	if (!found) {
		LOG(KERN_WARNING, "unknown cmd (%d)", cmd->id);
		cmd->destroy(cmd);
	}
}


static int dest_device_worker(void *data)
{
	struct dest_device *device = (struct dest_device *)data;
	int i;
	struct commands commands;
	const int arr_size = ARRAY_SIZE(commands.cmds);

	LOG(KERN_DEBUG, "dest device worker starts(%ux%u)..."
		, device->client->resolution.width
		, device->client->resolution.height);

	while (!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);
		wait_for_completion_interruptible(&device->completion);

		if (kthread_should_stop())
			break;

		spin_lock(&device->cmd_lock);
		for (i = 0; i < arr_size; ++i) {
			commands.cmds[i] = device->commands.cmds[i];
			device->commands.cmds[i] = NULL;
		}
		spin_unlock(&device->cmd_lock);

		for (i = 0; i < arr_size; ++i) {
			if (commands.cmds[i]) {
				commands.cmds[i]->execute(commands.cmds[i]);
				commands.cmds[i]->destroy(commands.cmds[i]);
			}
		}
	}

	LOG(KERN_DEBUG, "dest device worker finishes(%ux%u)..."
		, device->client->resolution.width
		, device->client->resolution.height);

	return 0;
}


static struct dest_device *dest_create_device(const struct bs_client *client)
{
	struct dest_device *device = kmalloc(sizeof(*device), GFP_KERNEL);

	if (!device)
		return NULL;

	init_completion(&device->completion);
	spin_lock_init(&device->cmd_lock);
	device->client = client;
	memset(device->commands.cmds, 0, sizeof(device->commands.cmds));
	device->worker = kthread_run(dest_device_worker
		, device, "device worker");

	return device;
}


static void dest_destroy_device(struct dest_device *device)
{
	int i;

	spin_lock(&device->cmd_lock);
	for (i = 0; i < ARRAY_SIZE(device->commands.cmds); ++i) {
		if (device->commands.cmds[i]) {
			device->commands.cmds[i]
				->destroy(device->commands.cmds[i]);
			device->commands.cmds[i] = NULL;
		}
	}
	spin_unlock(&device->cmd_lock);
	complete(&device->completion);
	kthread_stop(device->worker);
	free_kthread_struct(device->worker);

	kfree(device);
}


static void on_client_added(const struct bs_client *client, void *data)
{
	struct dest_device *device = dest_create_device(client);

	if (!device) {
		LOG(KERN_ERR, "failed to create dest device");
		return;
	}

	spin_lock(&dest.device_lock);
	list_add_tail(&device->node, &dest.devices_head);
	spin_unlock(&dest.device_lock);
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
	u8 found = 0;

	list_for_each(pos, &dest.devices_head) {
		device = list_entry(pos, struct dest_device, node);

		if (device->client == cmd->client) {
			spin_lock(&device->cmd_lock);
			on_cmd(cmd, device);
			found = 1;
			spin_unlock(&device->cmd_lock);
			break;
		}
	}

	if (!found) {
		LOG(KERN_WARNING, "cmd is not handled");
		cmd->destroy(cmd);
	}
}


void destination_destroy(void)
{
	struct list_head *pos, *n;

	if (-1 == dest.listener_id)
		return;

	cm_remove_listener(dest.listener_id);
	dest.listener_id = -1;

	spin_lock(&dest.device_lock);

	list_for_each_safe(pos, n, &dest.devices_head) {
		struct dest_device *device = list_entry(pos
			, struct dest_device, node);

		list_del(pos);
		dest_destroy_device(device);
	}

	spin_unlock(&dest.device_lock);
}
