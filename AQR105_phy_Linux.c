#include "tn40.h"

int AQR105_set_speed(struct bdx_priv *priv, s32 speed);

#define AQR105_ALL_SPEEDS	(__ETHTOOL_LINK_MODE_MASK_NBITS)

static void AQR105_set_link_mode(unsigned long *bits, u32 speed)
{
	bitmap_zero(bits, __ETHTOOL_LINK_MODE_MASK_NBITS);
	__set_bit(ETHTOOL_LINK_MODE_Pause_BIT, bits);
	__set_bit(ETHTOOL_LINK_MODE_TP_BIT, bits);;
	if (speed == AQR105_ALL_SPEEDS) {
		__set_bit(ETHTOOL_LINK_MODE_10000baseT_Full_BIT, bits);
		__set_bit(ETHTOOL_LINK_MODE_5000baseT_Full_BIT, bits);
		__set_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, bits);
		__set_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT, bits);
		__set_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT, bits);
		__set_bit(ETHTOOL_LINK_MODE_Autoneg_BIT, bits);
	} else {
		__set_bit(speed, bits);
	}

}

int AQR105_get_link_ksettings(struct net_device *netdev,
			      struct ethtool_link_ksettings *cmd)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	cmd->base.speed = priv->link_speed;
	cmd->base.port = PORT_TP;
	cmd->base.autoneg = AUTONEG_ENABLE;
	cmd->base.duplex = DUPLEX_FULL;

#if defined(ETH_TP_MDI_AUTO)
	cmd->base.eth_tp_mdix = ETH_TP_MDI_AUTO;
	cmd->base.eth_tp_mdix_ctrl = ETH_TP_MDI_AUTO;
#else
	cmd->base.eth_tp_mdix = ETH_TP_MDI | ETH_TP_MDI_X;
	cmd->base.eth_tp_mdix_ctrl = ETH_TP_MDI | ETH_TP_MDI_X;
#endif
	AQR105_set_link_mode(cmd->link_modes.supported, AQR105_ALL_SPEEDS);

	if (priv->autoneg == AUTONEG_ENABLE) {
		memcpy(priv->link_advertising, cmd->link_modes.supported,
		       sizeof(priv->link_advertising));
	}
	memcpy(cmd->link_modes.advertising, priv->link_advertising,
	       sizeof(cmd->link_modes.advertising));

	return 0;
}

int AQR105_set_link_ksettings(struct net_device *netdev,
			      const struct ethtool_link_ksettings *cmd)
{
	struct bdx_priv *priv = netdev_priv(netdev);
	int rVal = 0;

	priv->autoneg = cmd->base.autoneg;
	if (priv->autoneg == AUTONEG_ENABLE) {
		AQR105_set_link_mode(priv->link_advertising, AQR105_ALL_SPEEDS);
	} else {
		switch (cmd->base.speed) {
		case 10000:	/*10G */
			AQR105_set_link_mode(priv->link_advertising,
					     ETHTOOL_LINK_MODE_10000baseT_Full_BIT);
			break;

		case 5000:	/*5G */
			AQR105_set_link_mode(priv->link_advertising,
					     ETHTOOL_LINK_MODE_5000baseT_Full_BIT);
			break;

		case 2500:	/*2.5G */
			AQR105_set_link_mode(priv->link_advertising,
					     ETHTOOL_LINK_MODE_2500baseT_Full_BIT);
			break;

		case 1000:	/*1G */
			AQR105_set_link_mode(priv->link_advertising,
					     ETHTOOL_LINK_MODE_1000baseT_Full_BIT);
			break;

		case 100:	/*100m */
			AQR105_set_link_mode(priv->link_advertising,
					     ETHTOOL_LINK_MODE_100baseT_Full_BIT);
			break;

		default:
			pr_err("does not support speed %u\n", cmd->base.speed);
			rVal = -EINVAL;
			break;
		}
	}
	if (rVal == 0) {
		rVal = AQR105_set_speed(priv, cmd->base.speed);
	}

	return 0;
}

__init void AQR105_register_settings(struct bdx_priv *priv)
{
	priv->phy_ops.get_link_ksettings = AQR105_get_link_ksettings;
	priv->phy_ops.set_link_ksettings = AQR105_set_link_ksettings;
	priv->autoneg = AUTONEG_ENABLE;
}
