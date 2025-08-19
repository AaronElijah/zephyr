#ifndef ZEPHYR_INCLUDE_DRIVERS_ETHERNET_SFP_H_
#define ZEPHYR_INCLUDE_DRIVERS_ETHERNET_SFP_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SFF-8472 defines the EEPROM size as 256 bytes for address A0h (0x50)
 */

 /*
 *	0xA0h Address data fields
 */

/** Physical Device Identifier Values (0x00) */
#define SFP_PID_REG                                                       0x00
#define SFP_PID_SIZE                                                      1
#define SFP_PID_REG_UNKNOWN                                               0x00
#define SFP_PID_REG_GBIC                                                  0x01
#define SFP_PID_REG_SOLDERED                                              0x02
#define SFP_PID_REG_SFP                                                   0x03
#define SFP_PID_REG_300PINXBI                                             0x04
#define SFP_PID_REG_XENPAK                                                0x05
#define SFP_PID_REG_XFP                                                   0x06
#define SFP_PID_REG_XFF                                                   0x07
#define SFP_PID_REG_XFP_E                                                 0x08
#define SFP_PID_REG_XPAK                                                  0x09
#define SFP_PID_REG_X2                                                    0x0A
#define SFP_PID_REG_DWDM_SFP_SFP_PLUS                                     0x0B
#define SFP_PID_REG_QSFP                                                  0x0C
#define SFP_PID_REG_QSFP_PLUS_LATER_SFF_8636                              0x0D
#define SFP_PID_REG_CXP_OR_LATER                                          0x0E
#define SFP_PID_REG_SHIELDED_MINI_MULTILANE_HD_4X                         0x0F
#define SFP_PID_REG_SHIELDED_MINI_MULTILANE_HD_8X                         0x10
#define SFP_PID_REG_QSFP28_OR_LATER_SFF_8636                              0x11
#define SFP_PID_REG_CXP2_OR_LATER                                         0x12
#define SFP_PID_REG_CDFP_STYLE_1_STYLE_2                                  0x13
#define SFP_PID_REG_SHIELDED_MINI_MULTILANE_HD_4X_FANOUT_CABLE            0x14
#define SFP_PID_REG_SHIELDED_MINI_MULTILANE_HD_8X_FANOUT_CABLE            0x15
#define SFP_PID_REG_CDFP_STYLE_3                                          0x16
#define SFP_PID_REG_MICRO_QSFP                                            0x17
#define SFP_PID_REG_QSFP_DD_DOUBLE_DENSITY_8X_PLUGGABLE_TRANSCEIVER       0x18
#define SFP_PID_REG_OSFP_8X_PLUGGABLE_TRANSCEIVER                         0x19
#define SFP_PID_REG_SFP_DD_DOUBLE_DENSITY_2X_PLUGGABLE_TRANSCEIVER_SFP_DD 0x1A
#define SFP_PID_REG_DSFP_DUAL_SMALL_FORM_FACTOR_PLUGGABLE_TRANSCEIVER     0x1B
#define SFP_PID_REG_X4_MINILINK_OCU_LINK                                  0x1C
#define SFP_PID_REG_X8_MINILINK                                           0x1D
#define SFP_PID_REG_QSFP_PLUS_OR_LATER                                    0x1E
#define SFP_PID_REG_SFP_DD_DOUBLE_DENSITY_2X_PLUGGABLE_TRANSCEIVER        0x1F
#define SFP_PID_REG_SFP_PLUS_AND_LATER                                    0x20
#define SFP_PID_REG_OSFP_XD                                               0x21
#define SFP_PID_REG_OIF_ELSFP                                             0x22
#define SFP_PID_REG_CDFP_X4_PCIE_SFF_TA_1032                              0x23
#define SFP_PID_REG_CDFP_X8_PCIE_SFF_TA_1032                              0x24
#define SFP_PID_REG_CDFP_X16_PCIE_SFF_TA_1032                             0x25
#define SFP_PID_REG_VENDOR_SPECIFIC                                       0x80 // applies up to 0xFF

/** Physical Device Extended Identifier Values (0x01) */
#define SFP_EXT_PID_REG             0x01
#define SFP_EXT_PID_SIZE            1
#define SFP_EXT_PID_GBIC_UNDEFINED  0x00
#define SFP_EXT_PID_GBIC_MOD_DEF_1  0x01
#define SFP_EXT_PID_GBIC_MOD_DEF_2  0x02
#define SFP_EXT_PID_GBIC_MOD_DEF_3  0x03
#define SFP_EXT_PID_GBIC_SFP_2_WIRE 0x04
#define SFP_EXT_PID_GBIC_MOD_DEF_5  0x05
#define SFP_EXT_PID_GBIC_MOD_DEF_6  0x06
#define SFP_EXT_PID_GBIC_MOD_DEF_7  0x07

/** Connector Values (0x02) */
#define SFP_CONNECTOR_REG                        0x02
#define SFP_CONNECTOR_SIZE                       1
#define SFP_CONNECTOR_UNKNOWN                    0x00
#define SFP_CONNECTOR_SC                         0x01
#define SFP_CONNECTOR_FIBRE_CHANNEL_STYLE_1      0x02
#define SFP_CONNECTOR_FIBRE_CHANNEL_STYLE_2      0x03
#define SFP_CONNECTOR_BNC_TNC                    0x04
#define SFP_CONNECTOR_FIBRE_CHANNEL_COAX_HEADERS 0x05
#define SFP_CONNECTOR_FIBER_JACK                 0x06
#define SFP_CONNECTOR_LC                         0x07
#define SFP_CONNECTOR_MT_RJ                      0x08
#define SFP_CONNECTOR_MU                         0x09
#define SFP_CONNECTOR_SG                         0x0A
#define SFP_CONNECTOR_OPTICAL_PIGTAIL            0x0B
#define SFP_CONNECTOR_MPO_1X12                   0x0C
#define SFP_CONNECTOR_MPO_2X16                   0x0D
#define SFP_CONNECTOR_HSSDC_II                   0x20
#define SFP_CONNECTOR_COPPER_PIGTAIL             0x21
#define SFP_CONNECTOR_RJ45                       0x22
#define SFP_CONNECTOR_NO_SEPARABLE_CONNECTOR     0x23
#define SFP_CONNECTOR_MXC_2X16                   0x24
#define SFP_CONNECTOR_CS_OPTICAL_CONNECTOR       0x25
#define SFP_CONNECTOR_SN_OPTICAL_CONNECTOR       0x26
#define SFP_CONNECTOR_MPO_2X12                   0x27
#define SFP_CONNECTOR_MPO_1X16                   0x28

/** Transceiver Compliance Codes (0x03) */
#define SFP_COMPLIANCE_REG  0x03
#define SFP_COMPLIANCE_SIZE 8

#define SFP_COMPLIANCE_CODE1_BYTENUM        3
#define SFP_COMPLIANCE_CODE1_10GBASE_ER     BIT(7)
#define SFP_COMPLIANCE_CODE1_10GBASE_LRM    BIT(6)
#define SFP_COMPLIANCE_CODE1_10GBASE_LR     BIT(5)
#define SFP_COMPLIANCE_CODE1_10GBASE_SR     BIT(4)
#define SFP_COMPLIANCE_CODE1_1X_SX          BIT(3)
#define SFP_COMPLIANCE_CODE1_1X_LX          BIT(2)
#define SFP_COMPLIANCE_CODE1_1X_COPPER_ACT  BIT(1)
#define SFP_COMPLIANCE_CODE1_1X_COPPER_PASV BIT(0)

#define SFP_COMPLIANCE_CODE2_BYTENUM     4
#define SFP_COMPLIANCE_CODE2_1310LED     BIT(7)
#define SFP_COMPLIANCE_CODE2_1310LASER   BIT(6)
#define SFP_COMPLIANCE_CODE2_OC192_SHORT BIT(5)
#define SFP_COMPLIANCE_CODE2_SONET_1     BIT(4)
#define SFP_COMPLIANCE_CODE2_SONET_2     BIT(3)
#define SFP_COMPLIANCE_CODE2_OC48_LONG   BIT(2)
#define SFP_COMPLIANCE_CODE2_OC48_MEDIUM BIT(1)
#define SFP_COMPLIANCE_CODE2_OC48_SHORT  BIT(0)

#define SFP_COMPLIANCE_CODE3_BYTENUM     5
#define SFP_COMPLIANCE_CODE3_OC12_LONG   BIT(6)
#define SFP_COMPLIANCE_CODE3_OC12_MEDIUM BIT(5)
#define SFP_COMPLIANCE_CODE3_OC12_SHORT  BIT(4)
#define SFP_COMPLIANCE_CODE3_OC3_LONG    BIT(2)
#define SFP_COMPLIANCE_CODE3_OC3_MEDIUM  BIT(1)
#define SFP_COMPLIANCE_CODE3_OC3_SHORT   BIT(0)

#define SFP_COMPLIANCE_CODE4_BYTENUM      6
#define SFP_COMPLIANCE_CODE4_BASE_PX      BIT(7)
#define SFP_COMPLIANCE_CODE4_BASE_BX10    BIT(6)
#define SFP_COMPLIANCE_CODE4_100BASE_BX10 BIT(5)
#define SFP_COMPLIANCE_CODE4_100BASE_LX   BIT(4)
#define SFP_COMPLIANCE_CODE4_1000BASE_T   BIT(3)
#define SFP_COMPLIANCE_CODE4_1000BASE_CX  BIT(2)
#define SFP_COMPLIANCE_CODE4_1000BASE_LX  BIT(1)
#define SFP_COMPLIANCE_CODE4_1000BASE_SX  BIT(0)

#define SFP_COMPLIANCE_CODE5_BYTENUM 7
#define SFP_COMPLIANCE_CODE5_V       BIT(7)
#define SFP_COMPLIANCE_CODE5_S       BIT(6)
#define SFP_COMPLIANCE_CODE5_I       BIT(5)
#define SFP_COMPLIANCE_CODE5_L       BIT(4)
#define SFP_COMPLIANCE_CODE5_M       BIT(3)
#define SFP_COMPLIANCE_CODE5_SA      BIT(2)
#define SFP_COMPLIANCE_CODE5_LC      BIT(1)
#define SFP_COMPLIANCE_CODE5_EL      BIT(0)

#define SFP_COMPLIANCE_CODE6_BYTENUM 8
#define SFP_COMPLIANCE_CODE6_EL      BIT(7)
#define SFP_COMPLIANCE_CODE6_SN      BIT(6)
#define SFP_COMPLIANCE_CODE6_SL      BIT(5)
#define SFP_COMPLIANCE_CODE6_LL      BIT(4)
#define SFP_COMPLIANCE_CODE6_ACTIVE  BIT(3)
#define SFP_COMPLIANCE_CODE6_PASSIVE BIT(2)

#define SFP_COMPLIANCE_CODE7_BYTENUM 9
#define SFP_COMPLIANCE_CODE7_TW      BIT(7)
#define SFP_COMPLIANCE_CODE7_TP      BIT(6)
#define SFP_COMPLIANCE_CODE7_MI      BIT(5)
#define SFP_COMPLIANCE_CODE7_TV      BIT(4)
#define SFP_COMPLIANCE_CODE7_M6      BIT(3)
#define SFP_COMPLIANCE_CODE7_M5      BIT(2)
#define SFP_COMPLIANCE_CODE7_SM      BIT(0)

#define SFP_COMPLIANCE_CODE8_BYTENUM 10
#define SFP_COMPLIANCE_CODE8_1200    BIT(7)
#define SFP_COMPLIANCE_CODE8_800     BIT(6)
#define SFP_COMPLIANCE_CODE8_1600    BIT(5)
#define SFP_COMPLIANCE_CODE8_400     BIT(4)
#define SFP_COMPLIANCE_CODE8_3200    BIT(3)
#define SFP_COMPLIANCE_CODE8_200     BIT(2)
#define SFP_COMPLIANCE_CODE8_100     BIT(0)

 /*
 *	0xA2h Address data fields
 */
#define SFP_AW_THRESHOLDS_REG 0x00
#define SFP_AW_THRESHOLDS_SIZE 40

#define SFP_OPT_SW_THRES_REG 0x28
#define SFP_OPT_SW_THRES_SIZE 16

#define SFP_EXT_CALCONST_REG 0x38
#define SFP_EXT_CALCONST_SIZE 36

#define SFP_CC_DMI_REG 0x5F
#define SFP_CC_DMI_SIZE 1

#define SFP_DIAG_REG 96
#define SFP_DIAG_SIZE 10
#define SFP_DIAG_TEMP_MSB 96
#define SFP_DIAG_VCC_MSB 98
#define SFP_DIAG_TX_BIAS_MSB 100
#define SFP_DIAG_TX_POWER_MSB 102
#define SFP_DIAG_RX_POWER_MSB 104
#define SFP_DIAG_LAS_WAVE_MSB 106
#define SFP_DIAG_TEC_MSB 108

#define SFP_OPT_DIAG_REG 106
#define SFP_OPT_DIAG_SIZE 4

#define SFP_STSCTL_REG 110
#define SFP_STSCTL_SIZE 1
#define SFP_STSCTL_DATANOTREADY BIT(0)
#define SFP_STSCTL_RX_LOS BIT(1)
#define SFP_STSCTL_TX_FAULT BIT(2)
#define SFP_STSCTL_RATE_SEL BIT(4)
#define SFP_STSCTL_RS_STATE BIT(5)
#define SFP_STSCTL_TX_DISABLE BIT(7)

#define SFP_ALARM_FLG_REG 112
#define SFP_ALARM_FLG_SIZE 2

#define SFP_TX_EQ_REG 114
#define SFP_TX_EQ_SIZE 1

#define SFP_RX_OUT_REG 115
#define SFP_RX_OUT_SIZE 1

#define SFP_WARNING_FLAGS_REG 116
#define SFP_WARNING_FLAGS_SIZE 2

#define SFP_EXT_STATUS_CONTROL_REG 118
#define SFP_EXT_STATUS_CONTROL_SIZE 2

struct sfp_module_eeprom {
	// 0xA0 address
	uint8_t physical_id;
	uint8_t ext_id;
	uint8_t connector;
	uint8_t compliance[SFP_COMPLIANCE_SIZE];

	// 0xA2 address
	uint8_t aw_thresholds[SFP_AW_THRESHOLDS_SIZE];
	uint8_t optional_aw[SFP_OPT_SW_THRES_SIZE];
	uint8_t extcal_consts[SFP_EXT_CALCONST_SIZE];
	uint8_t cc_dmi;
	uint8_t diagnostics[SFP_DIAG_SIZE];
	uint8_t optional_diagnostics[SFP_OPT_DIAG_SIZE];
	uint8_t stsctl;
	uint8_t alarm_flg[SFP_ALARM_FLG_SIZE];
	uint8_t tx_eq;
	uint8_t rx_out;
	uint8_t warning_flags[SFP_WARNING_FLAGS_SIZE];
	uint8_t ext_status_control[SFP_EXT_STATUS_CONTROL_SIZE];
};

typedef int (*get_module_info_t)(const struct device *dev, struct sfp_module_eeprom *info);
typedef int (*check_state_t)(const struct device *dev, uint8_t *state);

__subsystem struct sfp_driver_api {
	get_module_info_t get_module_info;
	check_state_t check_state;
};

// __syscall int sfp_get_module_info(const struct device *dev, struct sfp_module_info *info);

// static inline int z_impl_sfp_get_module_info(const struct device *dev, struct sfp_module_info
// *info)
// {
// 	const struct sfp_driver_api *api = dev->api;

// 	return api->get_module_info(dev, info);
// }

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ETHERNET_SFP_H_ */
