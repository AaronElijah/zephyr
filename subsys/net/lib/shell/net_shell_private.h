/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/net/net_ip.h>

#define PR(fmt, ...)                                                                               \
	do {                                                                                       \
		if (sh) {                                                                          \
			shell_fprintf_normal(sh, fmt, ##__VA_ARGS__);                              \
		} else {                                                                           \
			printk(fmt, ##__VA_ARGS__);                                                \
		}                                                                                  \
	} while (false)

#define PR_SHELL(sh, fmt, ...)                                                                     \
	do {                                                                                       \
		if (sh) {                                                                          \
			shell_fprintf_normal(sh, fmt, ##__VA_ARGS__);                              \
		} else {                                                                           \
			printk(fmt, ##__VA_ARGS__);                                                \
		}                                                                                  \
	} while (false)

#define PR_ERROR(fmt, ...)                                                                         \
	do {                                                                                       \
		if (sh) {                                                                          \
			shell_fprintf_error(sh, fmt, ##__VA_ARGS__);                               \
		} else {                                                                           \
			printk(fmt, ##__VA_ARGS__);                                                \
		}                                                                                  \
	} while (false)

#define PR_INFO(fmt, ...)                                                                          \
	do {                                                                                       \
		if (sh) {                                                                          \
			shell_fprintf_info(sh, fmt, ##__VA_ARGS__);                                \
		} else {                                                                           \
			printk(fmt, ##__VA_ARGS__);                                                \
		}                                                                                  \
	} while (false)

#define PR_WARNING(fmt, ...)                                                                       \
	do {                                                                                       \
		if (sh) {                                                                          \
			shell_fprintf_warn(sh, fmt, ##__VA_ARGS__);                                \
		} else {                                                                           \
			printk(fmt, ##__VA_ARGS__);                                                \
		}                                                                                  \
	} while (false)

#include "net_private.h"
#include "../ip/ipv6.h"

struct net_shell_user_data {
	const struct shell *sh;
	void *user_data;
};

/******** Calculate a conservative upper bound for net_if count */
/* IPv4/IPv6 logical iface maxima (these already exist and are app-agnostic) */
#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
#define _IF_IP_LOGICAL_MAX MAX(CONFIG_NET_IF_MAX_IPV6_COUNT, CONFIG_NET_IF_MAX_IPV4_COUNT)
#elif defined(CONFIG_NET_IPV6)
#define _IF_IP_LOGICAL_MAX CONFIG_NET_IF_MAX_IPV6_COUNT
#elif defined(CONFIG_NET_IPV4)
#define _IF_IP_LOGICAL_MAX CONFIG_NET_IF_MAX_IPV4_COUNT
#else
#define _IF_IP_LOGICAL_MAX 0
#endif

/* VLAN sub-ifaces (generic) */
#if defined(CONFIG_NET_VLAN) && defined(CONFIG_NET_VLAN_COUNT)
#define _IF_VLAN_COUNT CONFIG_NET_VLAN_COUNT
#else
#define _IF_VLAN_COUNT 0
#endif

/* Loopback usually contributes one net_if if enabled */
#ifdef CONFIG_NET_LOOPBACK
#define _IF_LOOPBACK_COUNT 1
#else
#define _IF_LOOPBACK_COUNT 0
#endif

/* Derive a conservative upper bound with NO platform-specific compatibles */
#define _NET_SHELL_IFACE_BOUND MAX(1, _IF_IP_LOGICAL_MAX + _IF_VLAN_COUNT + _IF_LOOPBACK_COUNT)

/* Allow user/app to override via Kconfig (keeps code app-agnostic) */
#if defined(CONFIG_NET_SHELL_MAX_IFACES) && (CONFIG_NET_SHELL_MAX_IFACES > 0)
#define MAX_IFACE_COUNT CONFIG_NET_SHELL_MAX_IFACES
#else
#define MAX_IFACE_COUNT _NET_SHELL_IFACE_BOUND
#endif
/*********/

#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
#define ADDR_LEN NET_IPV6_ADDR_LEN
#elif defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
#define ADDR_LEN NET_IPV4_ADDR_LEN
#else
#define ADDR_LEN NET_IPV6_ADDR_LEN
#endif

#if defined(CONFIG_NET_SHELL_DYN_CMD_COMPLETION)
#define IFACE_DYN_CMD &iface_index
#else
#define IFACE_DYN_CMD NULL
#endif /* CONFIG_NET_SHELL_DYN_CMD_COMPLETION */

const char *addrtype2str(enum net_addr_type addr_type);
const char *addrstate2str(enum net_addr_state addr_state);
void get_addresses(struct net_context *context, char addr_local[], int local_len,
		   char addr_remote[], int remote_len);
void events_enable(void);
int get_iface_idx(const struct shell *sh, char *index_str);
const char *iface2str(struct net_if *iface, const char **extra);
void ipv6_frag_cb(struct net_ipv6_reassembly *reass, void *user_data);
