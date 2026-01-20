/*
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_bridge.h>
#include <zephyr/net/virtual.h>
#include <zephyr/sys/slist.h>

static int get_idx(const struct shell *sh, char *index_str)
{
	char *endptr;
	int idx;

	idx = strtol(index_str, &endptr, 10);
	if (*endptr != '\0') {
		shell_warn(sh, "Invalid index %s\n", index_str);
		return -ENOENT;
	}

	return idx;
}

static int cmd_bridge_addif(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0, br_idx, if_idx;
	struct net_if *iface;
	struct net_if *br;

	br_idx = get_idx(sh, argv[1]);
	if (br_idx < 0) {
		return br_idx;
	}

	br = eth_bridge_get_by_index(br_idx);
	if (br == NULL) {
		shell_warn(sh, "Bridge %d not found\n", br_idx);
		return -ENOENT;
	}

	for (int i = 2; i < argc; i++) {
		if_idx = get_idx(sh, argv[i]);
		if (if_idx < 0) {
			continue;
		}

		iface = net_if_get_by_index(if_idx);
		if (iface == NULL) {
			shell_warn(sh, "Interface %d not found\n", if_idx);
			continue;
		}

		if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
			shell_warn(sh, "Interface %d is not Ethernet\n", if_idx);
			continue;
		}

		if (!(net_eth_get_hw_capabilities(iface) & ETHERNET_PROMISC_MODE)) {
			shell_warn(sh, "Interface %d cannot do promiscuous mode\n", if_idx);
			continue;
		}

		ret = eth_bridge_iface_add(br, iface);
		if (ret < 0) {
			shell_error(sh, "error: bridge iface add (%d)\n", ret);
		}
	}

	return ret;
}

static int cmd_bridge_delif(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0, br_idx, if_idx;
	struct net_if *iface;
	struct net_if *br;

	br_idx = get_idx(sh, argv[1]);
	if (br_idx < 0) {
		return br_idx;
	}

	br = eth_bridge_get_by_index(br_idx);
	if (br == NULL) {
		shell_warn(sh, "Bridge %d not found\n", br_idx);
		return -ENOENT;
	}

	for (int i = 2; i < argc; i++) {
		if_idx = get_idx(sh, argv[i]);
		if (if_idx < 0) {
			continue;
		}

		iface = net_if_get_by_index(if_idx);
		if (iface == NULL) {
			shell_warn(sh, "Interface %d not found\n", if_idx);
			continue;
		}

		ret = eth_bridge_iface_remove(br, iface);
		if (ret < 0) {
			shell_error(sh, "error: bridge iface remove (%d)\n", ret);
		}
	}

	return ret;
}

static void bridge_show(struct eth_bridge_iface_context *ctx, void *data)
{
	int br_idx = eth_bridge_get_index(ctx->iface);
	const struct shell *sh = data;

	shell_fprintf(sh, SHELL_NORMAL, "%-7d", br_idx);

	if (net_if_is_up(ctx->iface)) {
		shell_fprintf(sh, SHELL_NORMAL, "%-9s", "up");
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "%-9s", "down");
	}

	if (ctx->is_setup) {
		shell_fprintf(sh, SHELL_NORMAL, "%-9s", "ok");
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "%-9s", "no");
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);

	ARRAY_FOR_EACH(ctx->eth_iface, i) {
		int if_idx;

		if (ctx->eth_iface[i] == NULL) {
			continue;
		}

		if_idx = net_if_get_by_iface(ctx->eth_iface[i]);

		shell_fprintf(sh, SHELL_NORMAL, "%-2d", if_idx);
	}

	shell_fprintf(sh, SHELL_NORMAL, "\n");

	k_mutex_unlock(&ctx->lock);
}

static int cmd_bridge_show(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *br = NULL;
	int br_idx;

	if (argc == 2) {
		br_idx = get_idx(sh, argv[1]);
		if (br_idx < 0) {
			return br_idx;
		}

		br = eth_bridge_get_by_index(br_idx);
		if (br == NULL) {
			shell_warn(sh, "Bridge %d not found\n", br_idx);
			return -ENOENT;
		}
	}

	shell_fprintf(sh, SHELL_NORMAL, "Bridge %-9s%-9sInterfaces\n", "Status", "Config");

	if (br != NULL) {
		bridge_show(net_if_get_device(br)->data, (void *)sh);
	} else {
		net_eth_bridge_foreach(bridge_show, (void *)sh);
	}

	return 0;
}

static int cmd_bridge_vlan_add(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	int if_idx;
	char iface_name[6];
	struct ethernet_vlan vlan = {0};
	struct ethernet_context *ctx = NULL;
	struct net_if *br = NULL, *p_iface = NULL;

	// get iface
	ret = snprintf(iface_name, sizeof(iface_name), "%s", argv[1]);
	if (ret < 0 || ret >= sizeof(iface_name)) {
		shell_warn(sh, "Interface name is incorrect\n");
		return -EINVAL;
	}
	if_idx = net_if_get_by_name(iface_name);
	if (if_idx < 0) {
		shell_warn(sh, "Interface %s not found\n", iface_name);
		return -ENOENT;
	}
	p_iface = net_if_get_by_index(if_idx);
	if (p_iface == NULL) {
		shell_warn(sh, "Interface %s not found\n", iface_name);
		return -ENOENT;
	}

	ctx = net_if_l2_data(p_iface);
	if (ctx->bridge == NULL) {
		shell_warn(sh, "Interface %d is not a member of a bridge\n", if_idx);
		return -EINVAL;
	}

	br = ctx->bridge;

	// check vlan is valid
	uint16_t vlan_id = atoi(argv[2]);
	if (vlan_id < 1 || vlan_id > NET_VLAN_TAG_UNSPEC - 1) {
		shell_warn(sh, "VLAN ID must be between 1 and %d inclusive\n",
			   NET_VLAN_TAG_UNSPEC - 1);
		return -EINVAL;
	}

	// check flags for untagged or pvid
	bool pvid = false;
	bool untagged = false;
	if (argc > 3) {
		for (int i = 3; i < argc; i++) {
			if (strcmp(argv[i], "untagged") == 0) {
				untagged = true;
			} else if (strcmp(argv[i], "pvid") == 0) {
				pvid = true;
			} else {
				shell_warn(sh, "Flags supported: 'untagged', 'pvid'\n");
				return -EINVAL;
			}
		}
	}

	// add vlan to bridge port
	vlan.iface = p_iface;
	vlan.tag = vlan_id;
	vlan.pvid = pvid;
	vlan.untagged = untagged;
	if (eth_bridge_vlan_add(br, p_iface, &vlan) < 0) {
		return -EINVAL;
	}

	return 0;
}

static int cmd_bridge_vlan_del(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	int if_idx;
	char iface_name[6];
	struct ethernet_vlan vlan = {0};
	struct ethernet_context *ctx = NULL;
	struct net_if *br = NULL, *p_iface = NULL;

	// get iface
	ret = snprintf(iface_name, sizeof(iface_name), "%s", argv[1]);
	if (ret < 0 || ret >= sizeof(iface_name)) {
		shell_warn(sh, "Interface name is incorrect\n");
		return -EINVAL;
	}
	if_idx = net_if_get_by_name(iface_name);
	if (if_idx < 0) {
		shell_warn(sh, "Interface %s not found\n", iface_name);
		return -ENOENT;
	}
	p_iface = net_if_get_by_index(if_idx);
	if (p_iface == NULL) {
		shell_warn(sh, "Interface %s not found\n", iface_name);
		return -ENOENT;
	}

	// check interface is added to a bridge
	ctx = net_if_l2_data(p_iface);
	if (ctx->bridge == NULL) {
		shell_warn(sh, "Interface %d is not a member of a bridge\n", if_idx);
		return -EINVAL;
	}

	br = ctx->bridge;

	// check vlan is valid
	uint16_t vlan_id = atoi(argv[2]);
	if (vlan_id < 1 || vlan_id > NET_VLAN_TAG_UNSPEC - 1) {
		shell_warn(sh, "VLAN ID must be between 1 and %d inclusive\n",
			   NET_VLAN_TAG_UNSPEC - 1);
		return -EINVAL;
	}

	// remove vlan from bridge port
	vlan.iface = p_iface;
	vlan.tag = vlan_id;
	if (eth_bridge_vlan_remove(br, p_iface, &vlan) < 0) {
		return -EINVAL;
	}

	return 0;
}

static int cmd_bridge_vlan_show(const struct shell *sh, size_t argc, char *argv[])
{
	int br_idx;
	struct net_if *br = NULL;
	// char br_name[MAX_BRIDGE_NAME_LEN] = {0};
	struct eth_bridge_iface_context *br_ctx;

	// check interface is a bridge
	br_idx = get_idx(sh, argv[1]);
	if (br_idx < 0) {
		shell_warn(sh, "Interface %d not valid\n", br_idx);
		return -ENOENT;
	}

	br = eth_bridge_get_by_index(br_idx);
	if (br == NULL) {
		shell_warn(sh, "Bridge %d not found\n", br_idx);
		return -ENOENT;
	}

	if (net_if_l2(br) != &NET_L2_GET_NAME(VIRTUAL) ||
	    !(net_virtual_get_iface_capabilities(br) & VIRTUAL_INTERFACE_BRIDGE)) {
		shell_warn(sh, "Interface %d is not a bridge\n", br_idx);
		return -EINVAL;
	}

	br_ctx = net_if_get_device(br)->data;
	// net_if_get_name(br, br_name, MAX_BRIDGE_NAME_LEN);

	// check first VLAN entry to confirm that vlans are configured
	if (br_ctx->vlan_info[0].iface == NULL) {
		shell_print(sh, "No VLANs configured on bridge %d\n", br_idx);
		return 0;
	}

	shell_print(sh, "VLANs configured on bridge %d:\n", br_idx);
	shell_print(sh, "port    vlan-id    pvid    untagged");
	// loop through every bridged port iface, then loop every vlan entry to find matches
	// obviously very inefficient but ok for small number of entries
	// in future, we can add a linked list type structure on the bridge port iface data
	// for vlan entries which apply to it
	ARRAY_FOR_EACH(br_ctx->eth_iface, i) {
		struct net_if *br_p_iface = br_ctx->eth_iface[i];
		if (br_p_iface == NULL) {
			continue;
		}
		shell_print(sh, "%d: ", net_if_get_by_iface(br_p_iface));

		ARRAY_FOR_EACH(br_ctx->vlan_info, j) {
			struct ethernet_vlan *_info = &br_ctx->vlan_info[j];
			if (_info->iface == br_p_iface) {
				shell_print(sh, "    %-9d    %-6s    %-8s", _info->tag,
					    _info->pvid ? "pvid" : "",
					    _info->untagged ? "untagged" : "");
			}
		}
	}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	bridge_vlan_commands,
	SHELL_CMD_ARG(add, NULL,
		      "Add a VLAN to a bridge.\n"
		      "'bridge vlan add <iface_name> <vlan_id> [<untagged|pvid>]'",
		      cmd_bridge_vlan_add, 3, 2),
	SHELL_CMD_ARG(del, NULL,
		      "Delete a VLAN from a bridge.\n"
		      "'bridge vlan del <iface_index> <vlan_id>'",
		      cmd_bridge_vlan_del, 3, 0),
	SHELL_CMD_ARG(show, NULL,
		      "Show VLANs added to bridge.\n"
		      "'bridge vlan show <bridge_index>'",
		      cmd_bridge_vlan_show, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	bridge_commands,
	SHELL_CMD_ARG(addif, NULL,
		      "Add a network interface to a bridge.\n"
		      "'bridge addif <bridge_index> <one or more interface index>'",
		      cmd_bridge_addif, 3, 5),
	SHELL_CMD_ARG(delif, NULL,
		      "Delete a network interface from a bridge.\n"
		      "'bridge delif <bridge_index> <one or more interface index>'",
		      cmd_bridge_delif, 3, 5),
	SHELL_CMD_ARG(show, NULL,
		      "Show bridge information.\n"
		      "'bridge show [<bridge_index>]'",
		      cmd_bridge_show, 1, 1),
	SHELL_CMD_ARG(vlan, &bridge_vlan_commands,
		      "VLAN commands for a bridge.\n"
		      "'bridge vlan <add|del> <iface_index> <vlan_id> [<other args>]'",
		      NULL, 4, 2),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((net), bridge, &bridge_commands, "Ethernet bridge commands.", cmd_bridge_show, 1,
		 1);
