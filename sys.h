/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2009-2018  B.A.T.M.A.N. contributors:
 *
 * Marek Lindner <mareklindner@neomailbox.ch>
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

#ifndef _BATCTL_SYS_H
#define _BATCTL_SYS_H

#include "main.h"

#include <linux/genetlink.h>
#include <netlink/genl/genl.h>
#include <stdbool.h>

#include "batman_adv.h"
#include "netlink.h"

#define SYS_BATIF_PATH_FMT	"/sys/class/net/%s/mesh/"
#define SYS_LOG_LEVEL		"log_level"
#define SYS_LOG			"log"
#define SYS_BLA			"bridge_loop_avoidance"
#define SYS_DAT			"distributed_arp_table"
#define SYS_NETWORK_CODING	"network_coding"
#define SYS_MULTICAST_MODE	"multicast_mode"
#define SYS_IFACE_PATH		"/sys/class/net"
#define SYS_IFACE_DIR		SYS_IFACE_PATH"/%s/"
#define SYS_MESH_IFACE_FMT	SYS_IFACE_PATH"/%s/batman_adv/mesh_iface"
#define SYS_IFACE_STATUS_FMT	SYS_IFACE_PATH"/%s/batman_adv/iface_status"
#define SYS_VLAN_PATH		SYS_IFACE_PATH"/%s/mesh/vlan%d/"
#define SYS_ROUTING_ALGO_FMT	SYS_IFACE_PATH"/%s/mesh/routing_algo"
#define VLAN_ID_MAX_LEN		4

struct settings_data {
	const char *sysfs_name;
	void *data;
	int (*parse)(struct state *state, int argc, char *argv[]);
	int (*netlink_get)(struct state *state);
	int (*netlink_set)(struct state *state);
};

struct simple_boolean_data {
	bool val;
};

int handle_sys_setting(struct state *state, int argc, char **argv);
int parse_simple_boolean(struct state *state, int argc, char *argv[]);


int sys_simple_nlquery(struct state *state, enum batadv_nl_commands nl_cmd,
		       nl_recvmsg_msg_cb_t attribute_cb,
		       nl_recvmsg_msg_cb_t callback);
int sys_simple_print_boolean(struct nl_msg *msg, void *arg,
			     enum batadv_nl_attrs attr);

#endif
