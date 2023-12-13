#include "tn40.h"

int QT2025_get_link_ksettings(struct net_device *netdev,
			      struct ethtool_link_ksettings *cmd)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	cmd->base.speed =
	    (READ_REG(priv, regMAC_LNK_STAT) & MAC_LINK_STAT) ? priv->link_speed
	    : 0;
	cmd->base.port = PORT_FIBRE;
	cmd->base.autoneg = AUTONEG_DISABLE;
	cmd->base.duplex = DUPLEX_FULL;

	__set_bit(ETHTOOL_LINK_MODE_10000baseT_Full_BIT,
		  cmd->link_modes.supported);
	__set_bit(ETHTOOL_LINK_MODE_Pause_BIT, cmd->link_modes.supported);
	__set_bit(ETHTOOL_LINK_MODE_FIBRE_BIT, cmd->link_modes.supported);;
	memcpy(cmd->link_modes.advertising, cmd->link_modes.supported,
	       sizeof(cmd->link_modes.advertising));

	return 0;
}

int QT2025_set_link_ksettings(struct net_device *netdev,
			      const struct ethtool_link_ksettings *cmd)
{
	netdev_err(netdev, "Does not support ethtool -s option\n");
	return -EPERM;
}

__init void QT2025_register_settings(struct bdx_priv *priv)
{
	priv->phy_ops.get_link_ksettings = QT2025_get_link_ksettings;
	priv->phy_ops.set_link_ksettings = QT2025_set_link_ksettings;
}
