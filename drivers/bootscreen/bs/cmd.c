// SPDX-License-Identifier: GPL-2.0

#include "cmd.h"
#include "log.h"

#include "linux/slab.h"


struct display_cmd {
	struct cmd cmd;
	const struct bs_data data;
};

struct contrast_cmd {
	struct cmd cmd;
	const u8 contrast;
};


static void execute_display_cmd(const struct cmd *cmd)
{
	struct display_cmd *thiz = (struct display_cmd *)cmd;
	thiz->cmd.client->display(thiz->data.data, thiz->data.size);
}


static void execute_contrast_cmd(const struct cmd *cmd)
{
	struct contrast_cmd *thiz = (struct contrast_cmd *)cmd;
	thiz->cmd.client->set_contrast(thiz->contrast);
}


static void destroy_display_cmd(const struct cmd *cmd)
{
	struct display_cmd *thiz = (struct display_cmd *)cmd;
	thiz->data.free_data(&thiz->data);
	kfree(cmd);
}


static void destroy_contrast_cmd(const struct cmd *cmd)
{
	kfree(cmd);
}


const struct cmd *cmd_create_display(const struct bs_client *client, struct bs_data *data)
{
	struct display_cmd *cmd = (struct display_cmd *)kmalloc(sizeof(*cmd), GFP_KERNEL);

	if (cmd) {
		cmd->cmd.id = 1;
		cmd->cmd.client = client;
		cmd->cmd.destroy = destroy_display_cmd;
		cmd->cmd.execute = execute_display_cmd;

		memcpy((void *)&cmd->data, data, sizeof(*data));
	} else {
		LOG(KERN_ERR, "can't allocate memory for display cmd");
	}

	return (struct cmd *)cmd;
}


const struct cmd *cmd_create_set_contrast(const struct bs_client *client, u8 contrast)
{
	struct contrast_cmd *cmd = (struct contrast_cmd *)kmalloc(sizeof(*cmd), GFP_KERNEL);

	if (cmd) {
		cmd->cmd.id = 2;
		cmd->cmd.client = client;
		cmd->cmd.destroy = destroy_contrast_cmd;
		cmd->cmd.execute = execute_contrast_cmd;
		*((u8 *)&cmd->contrast) = contrast;
	} else {
		LOG(KERN_ERR, "can't allocate memory for set contrast cmd");
	}

	return (struct cmd *)cmd;
}
