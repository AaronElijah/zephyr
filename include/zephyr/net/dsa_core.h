/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Distributed Switch Architecture (DSA)
 */

#ifndef ZEPHYR_INCLUDE_NET_DSA_CORE_H_
#define ZEPHYR_INCLUDE_NET_DSA_CORE_H_

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/phy.h>

/**
 * @brief Distributed Switch Architecture (DSA)
 * @defgroup dsa_core Distributed Switch Architecture (DSA)
 * @since 4.2
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_DSA_PORT_MAX_COUNT)
#define DSA_PORT_MAX_COUNT CONFIG_DSA_PORT_MAX_COUNT
#else
#define DSA_PORT_MAX_COUNT 11
#endif

#if defined(CONFIG_DSA_TAG_SIZE)
#define DSA_TAG_SIZE CONFIG_DSA_TAG_SIZE
#else
#define DSA_TAG_SIZE 0
#endif

/** @endcond */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Macro for DSA port instance initialization.
 *
 * @param port	DSA port node identifier.
 * @param n	DSA instance number.
 * @param cfg	Pointer to dsa_port_config.
 */
#define DSA_PORT_INST_INIT(port, n, cfg)                                                           \
	NET_DEVICE_INIT_INSTANCE(CONCAT(dsa_, n, port), DEVICE_DT_NAME(port), DT_REG_ADDR(port),   \
				 dsa_port_initialize, NULL, &dsa_switch_context_##n, cfg,          \
				 CONFIG_ETH_INIT_PRIORITY, &dsa_eth_api, ETHERNET_L2,              \
				 NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

/**
 * @brief Macro for DSA switch instance initialization.
 *
 * @param n	DSA instance number.
 * @param _dapi	Pointer to dsa_api.
 * @param data	Pointer to private data.
 * @param fn	DSA port instance init function.
 */
#define DSA_SWITCH_INST_INIT(n, _dapi, data, fn)                                                   \
	struct dsa_switch_context dsa_switch_context_##n = {                                       \
		.dapi = _dapi,                                                                     \
		.prv_data = data,                                                                  \
		.init_ports = 0,                                                                   \
		.num_ports = DT_INST_CHILD_NUM_STATUS_OKAY(n),                                     \
	};                                                                                         \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(n, fn, n);

/** DSA Link Aggregation */
struct dsa_lag {
	int id;
	bool is_valid;
};

/** DSA switch context data */
struct dsa_switch_context {
	/** Pointers to all DSA user network interfaces */
	struct net_if *iface_user[DSA_PORT_MAX_COUNT];

	/** Pointer to DSA conduit network interface */
	struct net_if *iface_conduit;

	/** DSA specific API callbacks */
	struct dsa_api *dapi;

	/** Instance specific data */
	void *prv_data;

	/** Number of ports in the DSA switch */
	uint8_t num_ports;

	/** Number of initialized ports in the DSA switch */
	uint8_t init_ports;

	/** User mdio device for internal PHY devices */
	/** This is a device with an mdio_driver_api implementation for the device API */
	const struct device *user_mdio_dev;

	/** Link Aggregation*/
	/* Should set this to the maximum number of
	 * supported IDs. However we don't want to use heap memory if we can
	 * Hence we'll leave this as documentation but in future, we will statically allocate the
	 * `lags` array
	 */
	// unsigned int num_lag_ids;
	/* Maps offloaded LAG netdevs to a zero-based linear ID for
	 * drivers that need it.
	 */
	struct dsa_lag lags[DSA_PORT_MAX_COUNT]; // LAG per port
	// nominally 32 LAG IDs supported on mv88e6393 but don't have cascaded switch, maximum is 11
	// LAG IDs in total - saves having to create a list from iterating `lags` member
	unsigned int lag_ids[11];
};

/**
 * Structure to provide DSA switch api callbacks - it is an augmented
 * struct ethernet_api.
 */
struct dsa_api {
	/** DSA helper callbacks */

	/** Handle receive packet on conduit port for untagging and redirection */
	struct net_if *(*recv)(struct net_if *iface, struct net_pkt *pkt);

	/** Transmit packet on the user port with tagging */
	struct net_pkt *(*xmit)(struct net_if *iface, struct net_pkt *pkt);

	/** Port init */
	int (*port_init)(const struct device *dev);

	/** Port link change */
	void (*port_phylink_change)(const struct device *dev, struct phy_link_state *state,
				    void *user_data);

	/** Port generates random mac address */
	void (*port_generate_random_mac)(uint8_t *mac_addr);

	/** Switch setup */
	int (*switch_setup)(const struct dsa_switch_context *dsa_switch_ctx);

	/** Tagged on phylink device API from linux for convenience */
	/** linux kernel treats phylink MAC operations in a seperate struct nominally */
	/** Port MAC enable/disable */
	int (*port_enable)(const struct device *dev, int port, struct phy_link_state *phy);
	int (*port_disable)(const struct device *dev, int port);
	/** PHYLINK MAC/PCS functions */
	const struct phylink_pcs_ops *(*phylink_mac_select_pcs)(const struct device *dev, int port,
								phy_interface_t interface);
	int (*phylink_mac_prepare)(const struct device *dev, int port, phy_interface_t interface);
	int (*phylink_mac_finish)(const struct device *dev, int port, phy_interface_t interface);
	int (*phylink_mac_interface_config)(const struct device *dev, int port,
					    phy_interface_t interface);
	int (*phylink_mac_link_up)(const struct device *dev, int port, unsigned int mode, int speed,
				   int duplex, bool tx_pause, bool rx_pause);
	/** VLAN support */
	int (*port_vlan_filtering)(const struct device *dev, int port, bool vlan_filtering);
	int (*port_vlan_add)(const struct device *dev, int port, uint16_t vid, bool untagged,
			     bool pvid);
	int (*port_vlan_del)(
		const struct device *dev, int port,
		uint16_t vid); // TODO: see what flags, arguments, etc for more complex cases
	/*
	 * Forwarding database
	 */
	int (*port_fdb_del)(const struct device *dev, int port, const unsigned char *addr,
			    uint16_t vid);
	/*
	 * LAG integration
	 */
	int (*port_lag_change)(const struct device *dev, int port);
	int (*port_lag_join)(const struct device *dev, int port, struct dsa_lag lag);
	int (*port_lag_leave)(const struct device *dev, int port, struct dsa_lag lag);
	/*
	 * EEE integration
	 */
	int (*set_mac_eee)(const struct device *dev, int port, bool is_eee_enabled);
	int (*get_mac_eee)(const struct device *dev, int port, bool *is_eee_enabled);
};

/**
 * Structure of DSA port configuration.
 */
struct dsa_port_config {
	/** Port mac address */
	uint8_t mac_addr[6];
	/** Use random mac address or not */
	const bool use_random_mac_addr;
	/** Port index */
	const int port_idx;
	/** PHY device */
	const struct device *phy_dev;
	/** PHY mode */
	const char *phy_mode;
	/** Ethernet device connected to the port */
	const struct device *ethernet_connection;
	/** SFP device - need to be specified somewhere else? */
	const struct device *const sfp_dev;
	/** Instance specific config */
	void *prv_config;
};

/**
 * Structure of DSA port device data.
 */
struct dsa_port {
	const struct device *dsa_master; /**< Pointer to the master switch device */

	/** Instance specific data */
	void *prv_data;
};

/** @cond INTERNAL_HIDDEN */

enum dsa_port_type {
	NON_DSA_PORT,
	DSA_CONDUIT_PORT,
	DSA_USER_PORT,
	DSA_CPU_PORT,
	DSA_PORT,
};

/*
 * DSA port init
 *
 * Returns:
 *  - 0 if ok, < 0 if error
 */
int dsa_port_initialize(const struct device *dev);

/*
 * DSA transmit function
 *
 * param dev: Port device to transmit
 * param pkt: Network packet
 *
 * Returns:
 *  - 0 if ok, < 0 if error
 */
int dsa_xmit(const struct device *dev, struct net_pkt *pkt);

/*
 * DSA receive function
 *
 * param iface: Interface of conduit port
 * param pkt: Network packet
 *
 * Returns:
 *  - Interface to redirect
 */
struct net_if *dsa_recv(struct net_if *iface, struct net_pkt *pkt);

/*
 * DSA ethernet init function to handle flags
 *
 * param iface: Interface of port
 *
 * Returns:
 *  - 0 if ok, < 0 if error
 */
int dsa_eth_init(struct net_if *iface);

/* Ethernet APIs definition for switch ports */
extern const struct ethernet_api dsa_eth_api;

/** @endcond */

/**
 * @brief      Get network interface of a user port
 *
 * @param      iface     Conduit port
 * @param[in]  port_idx  Port index
 *
 * @return     network interface of the user if successful
 * @return     NULL if user port does not exist
 */
struct net_if *dsa_user_get_iface(struct net_if *iface, int port_idx);

/**
 * @brief Structure to provide mac address for each LAN interface
 */

/**
 * @brief      Disable switch port
 *
 * @param      iface          DSA interface
 * @param      port 		  Port to disable
 *
 * @return     0 if successful, negative if error
 */
int dsa_port_disable(struct net_if *iface, int port);

/**
 * @brief 	    Enable switch port
 *
 * @param 		iface 		   DSA interface
 * @param		port		   Port to enable
 *
 * @return 		0 if successful, negative if error
 */
int dsa_port_enable(struct net_if *iface, int port);

/**
 * @brief      Select PHYLINK PCS for a DSA port
 *
 * @param      iface     DSA interface
 * @param      port      Port to configure
 * @param      interface PHY interface type
 *
 * @return     Pointer to the PHYLINK PCS operations, or NULL if not found
 */
const struct phylink_pcs_ops *dsa_port_phylink_mac_select_pcs(struct net_if *iface, int port,
							      phy_interface_t interface);

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
				 int duplex, bool tx_pause, bool rx_pause);

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
int dsa_port_phylink_mac_interface_config(struct net_if *iface, int port,
					  phy_interface_t interface);

/**
 * @brief 	    Enable/disable VLAN filtering on switch port
 *
 * @param 		iface 		   DSA interface
 * @param		port		   Port to enable VLANs
 * @param 		vlan_filtering Enable/disable VLAN filtering
 *
 * @return 		0 if successful, negative if error
 */
int dsa_port_vlan_filtering(struct net_if *iface, int port, bool vlan_filtering);

/**
 * @brief 		Add VLAN to switch port
 *
 * @param 		iface 		   DSA interface
 * @param		port		   Port to enable
 * @param 		vlan_id 	   VLAN ID
 *
 * @return 		0 if successful, negative if error
 */
int dsa_port_vlan_add(struct net_if *iface, int port, uint16_t vid, bool untagged, bool pvid);

/**
 * @brief        Remove VLAN from switch port
 *
 * @param 		iface 		   DSA interface
 * @param		port		   Port to enable
 * @param 		vlan_id 	   VLAN ID
 *
 * @return 		0 if successful, negative if error
 */
int dsa_port_vlan_del(struct net_if *iface, int port, uint16_t vid);

/**
 * @brief       Add LAG group to switch
 *
 * @param 		iface 		   DSA interface
 * @param 		lag_id 	   	   LAG group ID
 *
 * @return 		0 if successful, negative if error
 */
int dsa_switch_lag_join(struct net_if *iface, int port, unsigned int lag_id);

/**
 * @brief       Remove LAG group from switch
 *
 * @param 		iface 		   DSA interface
 * @param 		port 	   Port to remove LAG group from
 *
 * @return 		0 if successful, negative if error
 */
int dsa_switch_lag_leave(struct net_if *iface, int port, unsigned int lag_id);

/**
 * @brief       Change LAG group to switch
 *
 * @param 		iface 		   DSA interface
 * @param 		port 	   Port to change LAG group on
 * @param 		lag 	   LAG group ID
 *
 * @return 		0 if successful, negative if error
 */
int dsa_switch_lag_change(struct net_if *iface, int port, unsigned int lag_id);

/**
 * @brief       Set EEE configuration
 *
 * @param 		iface 	   DSA port interface
 * @param 		port 	   Port to change LAG group on
 * @param 		lag 	   LAG group ID
 *
 * @return 		0 if successful, negative if error
 */
int dsa_port_eee_cfg(struct net_if *iface, int port, bool is_eee_enabled);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_NET_DSA_CORE_H_ */
