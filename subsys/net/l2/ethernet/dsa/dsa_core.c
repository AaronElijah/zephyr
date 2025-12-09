/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa_core, CONFIG_NET_DSA_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dsa_core.h>
#include <zephyr/net/phy.h>

#include "dsa_tag.h"

struct net_if *dsa_recv(struct net_if *iface, struct net_pkt *pkt)
{
	if (iface == NULL || pkt == NULL) {
		return iface;
	}

	/* Tag protocol handles to untag and re-direct interface */
	return dsa_tag_recv(iface, pkt);
}

int dsa_xmit(const struct device *dev, struct net_pkt *pkt)
{
	struct dsa_switch_context *dsa_switch_ctx = dev->data;
	struct net_if *iface = net_if_lookup_by_dev(dev);
	struct net_if *iface_conduit = dsa_switch_ctx->iface_conduit;
	const struct device *dev_conduit = net_if_get_device(iface_conduit);
	const struct ethernet_api *eth_api_conduit = dev_conduit->api;
	/* Tag protocol handles pkt first */
	struct net_pkt *dsa_pkt = dsa_tag_xmit(iface, pkt);

	/* Transmit from conduit port */
	return eth_api_conduit->send(dev_conduit, dsa_pkt);
}

int dsa_eth_init(struct net_if *iface)
{
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);

	if (eth_ctx->dsa_port == DSA_CONDUIT_PORT) {
		net_if_flag_clear(iface, NET_IF_IPV4);
		net_if_flag_clear(iface, NET_IF_IPV6);
	}

	return 0;
}

/** CUSTOM */
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
	struct dsa_switch_context *context = dev->data;
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
	struct dsa_switch_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	/* Common sense defaults for the PHY link */
	struct phy_link_state link_state = {};
	link_state.speed = LINK_FULL_1000BASE;
	link_state.is_up = true;

	return api->port_enable(dev, port, &link_state);
}

const struct phylink_pcs_ops *dsa_port_phylink_mac_select_pcs(struct net_if *iface, int port,
							      phy_interface_t interface)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_switch_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->phylink_mac_select_pcs(dev, port, interface);
}

/**
 * @brief 	    Configure MAC link on switch port MAC
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
	struct dsa_switch_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->phylink_mac_link_up(dev, port, mode, speed, duplex, tx_pause, rx_pause);
}

/**
 * @brief Configure the MAC interface for a specific port on a DSA switch
 *
 * This function configures the MAC interface for a given port on a DSA switch.
 * It sets up the MAC layer interface type for the specified port.
 *
 * @param iface Pointer to the network interface
 * @param port Port number to configure
 * @param interface MAC layer interface type to set
 *
 * @return 0 if successful, -ENOSYS if the operation is not supported by the driver, <0 for other
 * errors
 */
int dsa_port_phylink_mac_interface_config(struct net_if *iface, int port, phy_interface_t interface)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_switch_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	if (!api->phylink_mac_interface_config) {
		return -ENOSYS;
	}
	return api->phylink_mac_interface_config(dev, port, interface);
}

/**
 * @brief 	    Enable/disable VLAN filtering on switch port
 *
 * @param 		iface 		   DSA interface
 * @param 		vlan_filtering Enable/disable VLAN filtering
 *
 * @return 		0 if successful, negative if error
 */
int dsa_port_vlan_filtering(struct net_if *iface, bool vlan_filtering)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_switch_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->port_vlan_filtering(dev, vlan_filtering);
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
int dsa_port_vlan_add(struct net_if *iface, uint16_t vid, bool untagged, bool pvid)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_switch_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->port_vlan_add(dev, vid, untagged, pvid);
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
int dsa_port_vlan_del(struct net_if *iface, uint16_t vid)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_switch_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->port_vlan_del(dev, vid);
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
	struct dsa_switch_context *context = dev->data;
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
	struct dsa_switch_context *context = dev->data;
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
	struct dsa_switch_context *context = dev->data;
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

int dsa_port_eee_cfg(struct net_if *iface, int port, bool is_eee_enabled)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_switch_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->set_mac_eee ? api->set_mac_eee(dev, port, is_eee_enabled) : -ENOSYS;
}
