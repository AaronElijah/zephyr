/** @file
 * @brief DSA related functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa, CONFIG_NET_DSA_LOG_LEVEL);

#include <errno.h>
#include <stdlib.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/dsa.h>

/*
 * Store, in the ethernet_context for master interface, the original
 * eth_tx() function, which will send packet with tag appended.
 */
int dsa_register_master_tx(struct net_if *iface, dsa_send_t fn)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	ctx->dsa_send = fn;
	return 0;
}

#ifdef CONFIG_NET_L2_ETHERNET
bool dsa_is_port_master(struct net_if *iface)
{
	/* First check if iface points to ETH interface */
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		/* Check its capabilities */
		if (net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_MASTER_PORT) {
			return true;
		}
	}

	return false;
}
#else
bool dsa_is_port_master(struct net_if *iface)
{
	return false;
}
#endif

/*
 * RECEIVE HANDLING CODE - ingress (ETH -> DSA slave ports)
 */

static int dsa_check_iface(struct net_if *iface)
{
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return -ENOENT;
	}

	if (!((net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_MASTER_PORT) ||
	      (net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_SLAVE_PORT))) {
		return -ESRCH;
	}

	return 0;
}

int dsa_register_recv_callback(struct net_if *iface, dsa_net_recv_cb_t cb)
{
	struct ethernet_context *ctx;
	int ret;

	ret = dsa_check_iface(iface);
	if (ret < 0) {
		return ret;
	}

	if (cb) {
		ctx = net_if_l2_data(iface);
		ctx->dsa_recv_cb = cb;
	}

	return 0;
}

struct net_if *dsa_net_recv(struct net_if *iface, struct net_pkt **pkt)
{
	const struct ethernet_context *c;
	const struct dsa_context *ctx;
	struct net_if *iface_sw;
	int ret;

	if (*pkt == NULL || iface == NULL) {
		return NULL;
	}

	c = net_if_l2_data(iface);
	ctx = c->dsa_ctx;

	if (ctx == NULL || ctx->dapi == NULL) {
		return iface;
	}

	if (ctx->dapi->dsa_get_iface == NULL) {
		NET_ERR("DSA: No callback to set LAN interfaces!");
		return iface;
	}

	iface_sw = ctx->dapi->dsa_get_iface(iface, *pkt);

	ret = dsa_check_iface(iface_sw);
	if (ret < 0) {
		return iface_sw;
	}

	/*
	 * Optional code to change the destination interface with some
	 * custom callback (to e.g. filter/switch packets based on MAC).
	 *
	 * The callback shall be only present (and used) for lan1..3, but
	 * not for the master interface, which shall support all other
	 * protocols - i.e. UDP. ICMP, TCP.
	 */
	c = net_if_l2_data(iface_sw);
	if (c->dsa_recv_cb) {
		if (c->dsa_recv_cb(iface_sw, *pkt)) {
			return iface_sw;
		}
	}

	return iface;
}

/*
 * TRANSMISSION HANDLING CODE egress (DSA slave ports -> ETH)
 */
int dsa_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct net_if *iface_master, *iface;
	struct ethernet_context *ctx;
	struct dsa_context *context;

	iface = net_if_lookup_by_dev(dev);
	if (dsa_is_port_master(iface)) {
		/*
		 * The master interface's ethernet_context structure holds
		 * pointer to its original eth_tx().
		 * The wrapper (i.e. dsa_tx()) is needed to modify packet -
		 * it appends tag to it.
		 */
		ctx = net_if_l2_data(iface);
		context = ctx->dsa_ctx;
		return ctx->dsa_send(dev, context->dapi->dsa_xmit_pkt(iface, pkt));
	}

	context = dev->data;
	iface_master = context->iface_master;

	if (iface_master == NULL) {
		NET_ERR("DSA: No master interface!");
		return -ENODEV;
	}

	/*
	 * Here packets are send via lan{123} interfaces in user program.
	 * Those structs' ethernet_api only have .send callback set to point
	 * to this wrapper function.
	 *
	 * Hence, it is necessary to get this callback from master's ethernet
	 * context structure..
	 */

	/* Adjust packet for DSA routing and send it via master interface */
	ctx = net_if_l2_data(iface_master);
	return ctx->dsa_send(net_if_get_device(iface_master),
			     context->dapi->dsa_xmit_pkt(iface, pkt));
}

struct net_if *dsa_get_slave_port(struct net_if *iface, int slave_num)
{
	struct ethernet_context *eth_ctx;
	struct dsa_context *dsa_ctx;

	eth_ctx = net_if_l2_data(iface);
	if (eth_ctx == NULL) {
		LOG_ERR("Iface %p context not available!", iface);
		return NULL;
	}

	dsa_ctx = eth_ctx->dsa_ctx;

	if (slave_num < 0 || slave_num >= dsa_ctx->num_slave_ports) {
		return NULL;
	}

	return dsa_ctx->iface_slave[slave_num];
}

int dsa_switch_read(struct net_if *iface, uint16_t reg_addr, uint8_t *value)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->switch_read(dev, reg_addr, value);
}

int dsa_switch_write(struct net_if *iface, uint16_t reg_addr, uint8_t value)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->switch_write(dev, reg_addr, value);
}

/**
 * @brief      Write static MAC table entry
 *
 * @param      iface          DSA interface
 * @param[in]  mac            MAC address
 * @param[in]  fw_port        The firmware port
 * @param[in]  tbl_entry_idx  Table entry index
 * @param[in]  flags          Flags
 *
 * @return     0 if successful, negative if error
 */
int dsa_switch_set_mac_table_entry(struct net_if *iface, const uint8_t *mac, uint8_t fw_port,
				   uint16_t tbl_entry_idx, uint16_t flags)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->switch_set_mac_table_entry(dev, mac, fw_port, tbl_entry_idx, flags);
}

/**
 * @brief      Read static MAC table entry
 *
 * @param      iface          DSA interface
 * @param      buf            Buffer to receive MAC address
 * @param[in]  tbl_entry_idx  Table entry index
 *
 * @return     0 if successful, negative if error
 */
int dsa_switch_get_mac_table_entry(struct net_if *iface, uint8_t *buf, uint16_t tbl_entry_idx)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->switch_get_mac_table_entry(dev, buf, tbl_entry_idx);
}

/**
 * @brief      Disable switch port
 *
 * @param      iface          DSA interface
 * @param      port 		  Port to disable
 *
 * @return     0 if successful, negative if error
 */
int dsa_port_disable(struct net_if *iface, int port)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->port_disable(dev, port);
}

/**
 * @brief 	    Enable switch port
 *
 * @param 		iface 		   DSA interface
 * @param		port		   Port to enable
 *
 * @return 		0 if successful, negative if error
 */
int dsa_port_enable(struct net_if *iface, int port)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	/* Common sense defaults for the PHY link */
	struct phy_link_state link_state = {};
	link_state.speed = LINK_FULL_1000BASE_T;
	link_state.is_up = true;

	return api->port_enable(dev, port, &link_state);
}

/**
 * @brief 	    Configure MAC link on switch port
 *
 * @param 		iface 		   DSA interface
 * @param		port		   Port to enable
 * @param 		mode 		   Autonegotiation mode ('phy', 'fixed', 'inband')
 * @param 		speed 		   Link speed (10, 100, 200, 1000, 2500, 10000)
 * @param		duplex 	   	   Duplex mode (1 [full], 0 [half])
 * @param 		tx_pause 	   Enable TX pause
 * @param 		rx_pause 	   Enable RX pause
 *
 * @return 		0 if successful, negative if error
 */
int dsa_port_phylink_mac_link_up(struct net_if *iface, int port, unsigned int mode, int speed,
				 int duplex, bool tx_pause, bool rx_pause)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->phylink_mac_link_up(dev, port, mode, speed, duplex, tx_pause, rx_pause);
}

/**
 * @brief 	    Enable/disable VLAN filtering on switch port
 *
 * @param 		iface 		   DSA interface
 * @param		port		   Port to enable VLANs
 * @param 		vlan_filtering Enable/disable VLAN filtering
 *
 * @return 		0 if successful, negative if error
 */
int dsa_port_vlan_filtering(struct net_if *iface, int port, bool vlan_filtering)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	// TODO: can we remove the need to have `port` as an arugment to this function? Why can't we
	// use `iface`?
	return api->port_vlan_filtering(dev, port, vlan_filtering);
}

/**
 * @brief 		Add VLAN to switch port
 *
 * @param 		iface 		   DSA interface
 * @param		port		   Port to enable
 * @param 		vlan_id 	   VLAN ID
 *
 * @return 		0 if successful, negative if error
 */
int dsa_port_vlan_add(struct net_if *iface, int port, uint16_t vid, bool untagged, bool pvid)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	// TODO: can we remove the need to have `port` as an arugment to this function? Why can't we
	// use `iface`?
	return api->port_vlan_add(dev, port, bool untagged, bool pvid);
}

/**
 * @brief       Remove VLAN from switch port
 *
 * @param 		iface 		   DSA interface
 * @param		port		   Port to enable
 * @param 		vlan_id 	   VLAN ID
 *
 * @return 		0 if successful, negative if error
 */
int dsa_port_vlan_del(struct net_if *iface, int port, uint16_t vid)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	// TODO: can we remove the need to have `port` as an arugment to this function? Why can't we
	// use `iface`?
	return api->port_vlan_del(dev, port, vid);
}

/**
 * @brief       Add LAG group to switch
 *
 * @param 		iface 		   DSA interface
 * @param 		lag_id 	   	   LAG group ID
 *
 * @return 		0 if successful, negative if error
 */
int dsa_switch_lag_join(struct net_if *iface, int port, unsigned int lag_id)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	// similar to dsa_lag_map in linux kernel
	// map LAG structure to linear LAG array
	struct dsa_lag new_lag = {0};
	for (int i = 0; i < sizeof(context->lag_ids) / sizeof(context->lag_ids[0]); i++) {
		// check if already existing valid LAG
		if (context->lag_ids[i] == lag_id && context->lag_ids[i] > 0) {
			new_lag.id = lag_id;
			new_lag.is_valid = true;
			break;
		} else if (context->lag_ids[i] == 0) {
			// add new LAG
			context->lag_ids[i] = lag_id;
			new_lag.id = lag_id;
			new_lag.is_valid = true;
			break;
		}
	}

	if (!new_lag.is_valid) {
		return -ENOMEM; // could not find or create LAG to add to context
	}

	context->lags[port] = new_lag;
	return api->port_lag_join(dev, port, new_lag);
}

/**
 * @brief       Remove LAG group from switch
 *
 * @param 		iface 		   DSA interface
 * @param 		port 	   Port to remove LAG group from
 *
 * @return 		0 if successful, negative if error
 */
int dsa_switch_lag_leave(struct net_if *iface, int port, unsigned int lag_id)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = context->dapi;

	struct dsa_lag old_lag;
	old_lag.id = lag_id;
	old_lag.is_valid = false;

	// check existing LAG group
	struct dsa_lag *port_lag = &context->lags[port];
	if (!(port_lag->is_valid || port_lag->id == 0)) {
		return -ENOTSUP; // no LAG group to remove
	} else if (port_lag->id != old_lag.id) {
		return -ENOTSUP; // wrong LAG group to remove given for the port
	}
	// edit existing LAG group
	port_lag->is_valid = false;
	port_lag->id = 0;

	// search any other ports are members of LAG
	bool is_unused_lag = true; // check if LAG is not used by any port anymore
	for (int i = 0; i < sizeof(context->lags) / sizeof(context->lags[0]); i++) {
		// check every port LAG and check if it is in the same LAG
		struct dsa_lag *existing_lag = &context->lags[i];
		if (existing_lag->is_valid && existing_lag->id == old_lag.id) {
			is_unused_lag = false;
		}
	}
	if (is_unused_lag) {
		// remove LAG from array of LAG IDs
		for (int i = 0; i < sizeof(context->lag_ids) / sizeof(context->lag_ids[0]); i++) {
			if (context->lag_ids[i] == old_lag.id) {
				context->lag_ids[i] = 0;
				break;
			}
		}
	}

	return api->port_lag_leave(dev, port, old_lag);
}

/**
 * @brief       Change LAG group to switch
 *
 * @param 		iface 		   DSA interface
 * @param 		port 	   Port to change LAG group on
 * @param 		lag 	   LAG group ID
 *
 * @return 		0 if successful, negative if error
 */
int dsa_switch_lag_change(struct net_if *iface, int port, unsigned int lag_id)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	struct dsa_lag old_lag = {0};
	old_lag.id = lag_id;
	old_lag.is_valid = true;

	// check existing LAG group
	struct dsa_lag *port_lag = &context->lags[port];
	if (!(port_lag->is_valid || port_lag->id == 0)) {
		return -ENOTSUP; // no LAG group to remove
	} else if (port_lag->id != old_lag.id) {
		return -ENOTSUP; // wrong LAG group to remove given for the port
	}
	// edit existing LAG group
	unsigned int old_lag_id = port_lag->id;
	port_lag->id = lag_id;

	// search any other ports are members of old LAG
	bool is_unused_lag = true; // check if LAG is not used by any port anymore
	for (int i = 0; i < sizeof(context->lags) / sizeof(context->lags[0]); i++) {
		// check every port LAG and check if it is in the same LAG
		struct dsa_lag *existing_lag = &context->lags[i];
		if (existing_lag->is_valid && existing_lag->id == old_lag_id) {
			is_unused_lag = false;
		}
	}
	if (is_unused_lag) {
		// remove LAG from array of LAG IDs
		for (int i = 0; i < sizeof(context->lag_ids) / sizeof(context->lag_ids[0]); i++) {
			if (context->lag_ids[i] == old_lag_id) {
				context->lag_ids[i] = 0;
				break;
			}
		}
	}

	return api->port_lag_change(dev, port);
}
