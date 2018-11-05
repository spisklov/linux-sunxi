/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __BS_SOURCE_H__
#define __BS_SOURCE_H__

#include "bs_types.h"
#include "cmd.h"

typedef void (*cmd_handler_t)(const struct cmd *cmd);

struct bs_source {
	void (*start)(struct bs_source *src);
	void (*finalize)(struct bs_source *src);
	void (*destroy)(struct bs_source *src);
};

#endif // __BS_SOURCE_H__

