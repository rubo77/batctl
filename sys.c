// SPDX-License-Identifier: GPL-2.0
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


#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <net/if.h>
#include <linux/if_link.h>
#include <netlink/netlink.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include "main.h"
#include "sys.h"
#include "functions.h"
#include "debug.h"

int parse_simple_boolean(struct state *state, int argc, char *argv[])
{
	struct settings_data *settings = state->cmd->arg;
	struct simple_boolean_data *data = settings->data;
	int ret;

	if (argc != 2) {
		fprintf(stderr, "Error - incorrect number of arguments (expected 1)\n");
		return -EINVAL;
	}

	ret = parse_bool(argv[1], &data->val);
	if (ret < 0) {
		fprintf(stderr, "Error - the supplied argument is invalid: %s\n", argv[1]);
		fprintf(stderr, "The following values are allowed:\n");
		fprintf(stderr, " * 0\n");
		fprintf(stderr, " * disable\n");
		fprintf(stderr, " * disabled\n");
		fprintf(stderr, " * 1\n");
		fprintf(stderr, " * enable\n");
		fprintf(stderr, " * enabled\n");

		return ret;
	}

	return 0;
}


static int sys_simple_nlerror(struct sockaddr_nl *nla __maybe_unused,
			      struct nlmsgerr *nlerr,	void *arg)
{
	int *result = arg;

	if (nlerr->error != -EOPNOTSUPP)
		fprintf(stderr, "Error received: %s\n",
			strerror(-nlerr->error));

	*result = nlerr->error;

	return NL_STOP;
}

int sys_simple_nlquery(struct state *state, enum batadv_nl_commands nl_cmd,
		       nl_recvmsg_msg_cb_t attribute_cb,
		       nl_recvmsg_msg_cb_t callback)
{
	int result;
	struct nl_msg *msg;
	int ret;

	if (!state->sock)
		return -EOPNOTSUPP;

	if (callback) {
		result = -EOPNOTSUPP;
		nl_cb_set(state->cb, NL_CB_VALID, NL_CB_CUSTOM, callback,
			  &result);
	} else {
		result = 0;
	}

	nl_cb_err(state->cb, NL_CB_CUSTOM, sys_simple_nlerror, &result);

	msg = nlmsg_alloc();
	if (!msg)
		return -ENOMEM;

	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, state->batadv_family, 0, 0,
		    nl_cmd, 1);
	nla_put_u32(msg, BATADV_ATTR_MESH_IFINDEX, state->mesh_ifindex);

	if (attribute_cb) {
		ret = attribute_cb(msg, state);
		if (ret < 0) {
			nlmsg_free(msg);
			return -ENOMEM;
		}
	}

	nl_send_auto_complete(state->sock, msg);
	nlmsg_free(msg);

	nl_recvmsgs(state->sock, state->cb);

	return result;
}

int sys_simple_print_boolean(struct nl_msg *msg, void *arg,
			     enum batadv_nl_attrs attr)
{
	struct nlattr *attrs[BATADV_ATTR_MAX + 1];
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	struct genlmsghdr *ghdr;
	int *result = arg;

	if (!genlmsg_valid_hdr(nlh, 0))
		return NL_OK;

	ghdr = nlmsg_data(nlh);

	if (nla_parse(attrs, BATADV_ATTR_MAX, genlmsg_attrdata(ghdr, 0),
		      genlmsg_len(ghdr), batadv_netlink_policy)) {
		return NL_OK;
	}

	if (!attrs[attr])
		return NL_OK;

	printf("%s\n", nla_get_u8(attrs[attr]) ? "enabled" : "disabled");

	*result = 0;
	return NL_STOP;
}

static void settings_usage(struct state *state)
{
	fprintf(stderr, "Usage: batctl [options] %s|%s [parameters] %s\n",
		state->cmd->name, state->cmd->abbr, state->cmd->usage);

	fprintf(stderr, "parameters:\n");
	fprintf(stderr, " \t -h print this help\n");
}

static int sys_read_setting(struct state *state, const char *path_buff,
			    const char *sysfs_name)
{
	struct settings_data *settings = state->cmd->arg;
	int res = EXIT_FAILURE;

	if (settings->netlink_get) {
		res = settings->netlink_get(state);
		if (res < 0 && res != -EOPNOTSUPP)
			return EXIT_FAILURE;
		if (res >= 0)
			return EXIT_SUCCESS;
	}

	if (sysfs_name)
		res = read_file(path_buff, sysfs_name, NO_FLAGS, 0, 0, 0);

	return res;
}

static int sys_write_setting(struct state *state, const char *path_buff,
			    const char *sysfs_name, int argc, char **argv)
{
	struct settings_data *settings = state->cmd->arg;
	int res = EXIT_FAILURE;

	if (settings->netlink_set) {
		res = settings->netlink_set(state);
		if (res < 0 && res != -EOPNOTSUPP)
			return EXIT_FAILURE;
		if (res >= 0)
			return EXIT_SUCCESS;
	}

	if (sysfs_name)
		res = write_file(path_buff, sysfs_name,
				 argv[1], argc > 2 ? argv[2] : NULL);

	return res;
}

int handle_sys_setting(struct state *state, int argc, char **argv)
{
	struct settings_data *settings = state->cmd->arg;
	int optchar, res = EXIT_FAILURE;
	char *path_buff;

	while ((optchar = getopt(argc, argv, "h")) != -1) {
		switch (optchar) {
		case 'h':
			settings_usage(state);
			return EXIT_SUCCESS;
		default:
			settings_usage(state);
			return EXIT_FAILURE;
		}
	}

	/* prepare the classic path */
	path_buff = malloc(PATH_BUFF_LEN);
	if (!path_buff) {
		fprintf(stderr, "Error - could not allocate path buffer: out of memory ?\n");
		return EXIT_FAILURE;
	}

	/* if the specified interface is a VLAN then change the path to point
	 * to the proper "vlan%{vid}" subfolder in the sysfs tree.
	 */
	if (state->vid >= 0)
		snprintf(path_buff, PATH_BUFF_LEN, SYS_VLAN_PATH,
			 state->mesh_iface, state->vid);
	else
		snprintf(path_buff, PATH_BUFF_LEN, SYS_BATIF_PATH_FMT,
			 state->mesh_iface);

	if (argc == 1) {
		res = sys_read_setting(state, path_buff, settings->sysfs_name);
		goto out;
	}

	check_root_or_die("batctl");

	if (settings->parse) {
		res = settings->parse(state, argc, argv);
		if (res < 0) {
			res = EXIT_FAILURE;
			goto out;
		}
	}

	res = sys_write_setting(state, path_buff, settings->sysfs_name, argc,
				argv);

out:
	free(path_buff);
	return res;
}
