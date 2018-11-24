// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2009-2018  B.A.T.M.A.N. contributors:
 *
 * Antonio Quartulli <a@unstable.cc>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 *
 * License-Filename: LICENSES/preferred/GPL-2.0
 */

#include "main.h"

#include <errno.h>
#include <linux/genetlink.h>
#include <netlink/genl/genl.h>

#include "batman_adv.h"
#include "netlink.h"
#include "sys.h"

static struct simple_boolean_data ap_isolation;

static int print_ap_isolation(struct nl_msg *msg, void *arg)
{
	return sys_simple_print_boolean(msg, arg, BATADV_ATTR_AP_ISOLATION);
}

static int get_attrs_ap_isolation(struct nl_msg *msg, void *arg)
{
	struct state *state = arg;

	if (state->vid >= 0)
		nla_put_u16(msg, BATADV_ATTR_VLANID, state->vid);

	return 0;
}

static int get_ap_isolation(struct state *state)
{
	enum batadv_nl_commands nl_cmd = BATADV_CMD_SET_MESH;

	if (state->vid >= 0)
		nl_cmd = BATADV_CMD_GET_VLAN;

	return sys_simple_nlquery(state, nl_cmd, get_attrs_ap_isolation,
				  print_ap_isolation);
}

static int set_attrs_ap_isolation(struct nl_msg *msg, void *arg)
{
	struct state *state = arg;
	struct settings_data *settings = state->cmd->arg;
	struct simple_boolean_data *data = settings->data;

	nla_put_u8(msg, BATADV_ATTR_AP_ISOLATION, data->val);

	if (state->vid >= 0)
		nla_put_u16(msg, BATADV_ATTR_VLANID, state->vid);

	return 0;
}

static int set_ap_isolation(struct state *state)
{
	enum batadv_nl_commands nl_cmd = BATADV_CMD_SET_MESH;

	if (state->vid >= 0)
		nl_cmd = BATADV_CMD_SET_VLAN;

	return sys_simple_nlquery(state, nl_cmd, set_attrs_ap_isolation, NULL);
}

static struct settings_data batctl_settings_ap_isolation = {
	.sysfs_name = "ap_isolation",
	.data = &ap_isolation,
	.parse = parse_simple_boolean,
	.netlink_get = get_ap_isolation,
	.netlink_set = set_ap_isolation,
};

COMMAND_NAMED(SUBCOMMAND, ap_isolation, "ap", handle_sys_setting,
	      COMMAND_FLAG_MESH_IFACE| COMMAND_FLAG_NETLINK,
	      &batctl_settings_ap_isolation,
	      "[0|1]             \tdisplay or modify ap_isolation setting");
