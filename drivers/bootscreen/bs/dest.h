/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __BS_DESTINATION_H__
#define __BS_DESTINATION_H__

#include "cmd.h"

int destination_create(void);
void destination_handle_cmd(const struct cmd *cmd);
void destination_destroy(void);

#endif // __BS_DESTINATION_H__


