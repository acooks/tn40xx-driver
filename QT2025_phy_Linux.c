#include "tn40.h"

//-------------------------------------------------------------------------------------------------

int QT2025_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    struct bdx_priv *priv = netdev_priv(netdev);

	ecmd->supported   	= ( SUPPORTED_10000baseT_Full | SUPPORTED_FIBRE | SUPPORTED_Pause);
	ecmd->advertising 	= (ADVERTISED_10000baseT_Full | ADVERTISED_Pause );
	if (READ_REG(priv,regMAC_LNK_STAT) &  MAC_LINK_STAT)
	{
		ecmd->speed     =  priv->link_speed;
	}
	else
	{
		ecmd->speed     = 0;
	}
	ecmd->duplex      	= DUPLEX_FULL;
	ecmd->port        	= PORT_FIBRE;
	ecmd->transceiver 	= XCVR_EXTERNAL;
	ecmd->autoneg     	= AUTONEG_DISABLE;

	return 0;

} // QT2025_get_settings()

//-------------------------------------------------------------------------------------------------

int QT2025_set_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    struct bdx_priv *priv = netdev_priv(netdev);

	ERR("%s Does not support ethtool -s option\n", priv->ndev->name);

	return -EPERM;

} // QT2025_set_settings()

//-------------------------------------------------------------------------------------------------
#ifdef ETHTOOL_GLINKSETTINGS

int QT2025_get_link_ksettings(struct net_device *netdev, struct ethtool_link_ksettings *cmd)
{
    struct bdx_priv *priv = netdev_priv(netdev);

	cmd->base.speed				= (READ_REG(priv,regMAC_LNK_STAT) &  MAC_LINK_STAT) ? priv->link_speed : 0;
	cmd->base.port 				= PORT_FIBRE;
	cmd->base.autoneg 			= AUTONEG_DISABLE;
	cmd->base.duplex			= DUPLEX_FULL;

	__set_bit(ETHTOOL_LINK_MODE_10000baseT_Full_BIT, 	cmd->link_modes.supported);
	__set_bit(ETHTOOL_LINK_MODE_Pause_BIT, 				cmd->link_modes.supported);
	__set_bit(ETHTOOL_LINK_MODE_FIBRE_BIT,   			cmd->link_modes.supported);;
	memcpy(cmd->link_modes.advertising, cmd->link_modes.supported, sizeof(cmd->link_modes.advertising));

	return 0;

} // QT2025_get_link_ksettings()

#endif
//-------------------------------------------------------------------------------------------------
#ifdef ETHTOOL_SLINKSETTINGS

int QT2025_set_link_ksettings(struct net_device *netdev, const struct ethtool_link_ksettings *cmd)
{
    struct bdx_priv *priv = netdev_priv(netdev);

	ERR("%s Does not support ethtool -s option\n", priv->ndev->name);

	return -EPERM;

} // QT2025_set_link_ksettings()

#endif
//-------------------------------------------------------------------------------------------------

__init void QT2025_register_settings(struct bdx_priv *priv)
{
    priv->phy_ops.get_settings = QT2025_get_settings;
    priv->phy_ops.set_settings = QT2025_set_settings;
#ifdef ETHTOOL_GLINKSETTINGS
    priv->phy_ops.get_link_ksettings = QT2025_get_link_ksettings;
#endif
#ifdef ETHTOOL_SLINKSETTINGS
    priv->phy_ops.set_link_ksettings = QT2025_set_link_ksettings;
#endif

} // QT2025_register_settings()
