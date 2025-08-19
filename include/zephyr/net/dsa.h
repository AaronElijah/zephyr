/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *               Lukasz Majewski <lukma@denx.de>
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief DSA definitions and handlers
 */

#ifndef ZEPHYR_INCLUDE_NET_DSA_H_
#define ZEPHYR_INCLUDE_NET_DSA_H_

#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/phy.h>

/**
 * @brief DSA definitions and helpers
 * @defgroup DSA Distributed Switch Architecture definitions and helpers
 * @since 2.5
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

#define NET_DSA_PORT_MAX_COUNT 11
#define DSA_STATUS_PERIOD_MS   K_MSEC(1000)

#ifdef CONFIG_DSA_TAG_SIZE
#define DSA_TAG_SIZE CONFIG_DSA_TAG_SIZE
#else
#define DSA_TAG_SIZE 0
#endif

/** @endcond */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DSA generic transmit function
 *
 * This is a generic function for passing packets from slave DSA interface to
 * master.
 *
 * @param dev Device
 * @param pkt Network packet
 *
 * Returns:
 *  - 0 if ok (packet sent via master iface), < 0 if error
 */
int dsa_tx(const struct device *dev, struct net_pkt *pkt);

/**
 * @brief DSA (MGMT) Receive packet callback
 *
 * Callback gets called upon receiving packet. It is responsible for
 * freeing packet or indicating to the stack that it needs to free packet
 * by returning correct net_verdict.
 *
 * Returns:
 *  - NET_DROP, if packet was invalid, rejected or we want the stack to free it.
 *    In this case the core stack will free the packet.
 *  - NET_OK, if the packet was accepted, in this case the ownership of the
 *    net_pkt goes to callback and core network stack will forget it.
 */
typedef enum net_verdict (*dsa_net_recv_cb_t)(struct net_if *iface, struct net_pkt *pkt);

/**
 * @brief Register DSA Rx callback functions
 *
 * @param iface Network interface
 * @param cb Receive callback function
 *
 * @return 0 if ok, < 0 if error
 */
int dsa_register_recv_callback(struct net_if *iface, dsa_net_recv_cb_t cb);

/**
 * @brief Set DSA interface to packet
 *
 * @param iface Network interface (master)
 * @param pkt Network packet
 *
 * @return Return the slave network interface
 */
struct net_if *dsa_net_recv(struct net_if *iface, struct net_pkt **pkt);

/**
 * @brief Pointer to master interface send function
 */
typedef int (*dsa_send_t)(const struct device *dev, struct net_pkt *pkt);

/**
 * @brief DSA helper function to register transmit function for master
 *
 * @param iface Network interface (master)
 * @param fn Pointer to master interface send method
 *
 * Returns:
 *  - 0 if ok, < 0 if error
 */
int dsa_register_master_tx(struct net_if *iface, dsa_send_t fn);

/**
 * @brief DSA helper function to check if port is master
 *
 * @param iface Network interface (master)
 *
 * Returns:
 *  - true if ok, false otherwise
 */
bool dsa_is_port_master(struct net_if *iface);

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */

/** DSA context data */
struct dsa_context {
	/** Pointers to all DSA slave network interfaces */
	struct net_if *iface_slave[NET_DSA_PORT_MAX_COUNT];

	/** Pointer to DSA master network interface */
	struct net_if *iface_master;

	/** DSA specific API callbacks - filled in the switch IC driver */
	struct dsa_api *dapi;

	/** DSA related work (e.g. monitor if network interface is up) */
	struct k_work_delayable dsa_work;

	/** Number of slave ports in the DSA switch */
	uint8_t num_slave_ports;

	/** Status of each port */
	bool link_up[NET_DSA_PORT_MAX_COUNT];

	/** Instance specific data */
	void *prv_data;
};

/**
 * @endcond
 */



#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_NET_DSA_H_ */
