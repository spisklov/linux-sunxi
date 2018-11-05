/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __BS_CLIENT_MANAGER_H__
#define __BS_CLIENT_MANAGER_H__

#include "bootscreen.h"

struct cm_listener {
	void (*on_client_added)(const struct bs_client *client, void *data);
	void *data;
};

int cm_initialize(void);
void cm_destroy(void);
void cm_add_client(const struct bs_client *client);
int cm_add_listener(const struct cm_listener *listener);
void cm_remove_listener(int listener_id);


#endif // __BS_CLIENT_MANAGER_H__
