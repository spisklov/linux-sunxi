/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __BS_ANIM_LOGO_H__
#define __BS_ANIM_LOGO_H__

#include "bootscreen.h"
#include "bs_types.h"


struct anim_logo {
	const struct bs_client *client;
	void (*next)(struct anim_logo *logo, struct bs_data *data);
	void (*destroy)(struct anim_logo *logo);
};

#endif // __BS_ANIM_LOGO_H__
