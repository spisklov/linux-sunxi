/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __BS_COMMANDS_H__
#define __BS_COMMANDS_H__

#include "bootscreen.h"
#include "bs_types.h"

struct cmd {
	int id;
	const struct bs_client *client;
	void (*execute)(const struct cmd *cmd);
	void (*destroy)(const struct cmd *cmd);
};


const struct cmd *cmd_create_display(const struct bs_client *client, struct bs_data *data);
const struct cmd *cmd_create_set_contrast(const struct bs_client *client, u8 contrast);

#endif // __BS_COMMANDS_H__
