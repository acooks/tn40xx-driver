#include "tn40.h"

int CX4_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    ecmd->supported   = (SUPPORTED_10000baseT_Full  | SUPPORTED_AUI | SUPPORTED_Pause);
	ecmd->advertising = (ADVERTISED_10000baseT_Full | ADVERTISED_Pause);
	ecmd->speed       = SPEED_10000;
	ecmd->duplex      = DUPLEX_FULL;
	ecmd->port        = PORT_AUI;
	ecmd->transceiver = XCVR_INTERNAL;
	ecmd->autoneg     = AUTONEG_DISABLE;

	return 0;

} // CX4_get_settings()

//-------------------------------------------------------------------------------------------------

int CX4_set_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
	ERR("CX4_set_settings() not implemented\n");

	return -EPERM;

} // CX4_set_settings()

//-------------------------------------------------------------------------------------------------
#ifdef ETHTOOL_GLINKSETTINGS

int CX4_get_link_ksettings(struct net_device *netdev, struct ethtool_link_ksettings *cmd)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	ENTER;

	cmd->base.speed				= (READ_REG(priv,regMAC_LNK_STAT) &  MAC_LINK_STAT) ? priv->link_speed : 0;
	cmd->base.port 				= PORT_AUI;
	cmd->base.autoneg 			= AUTONEG_DISABLE;
	cmd->base.duplex			= DUPLEX_FULL;

	__set_bit(ETHTOOL_LINK_MODE_10000baseT_Full_BIT, 	cmd->link_modes.supported);
	__set_bit(ETHTOOL_LINK_MODE_Pause_BIT, 				cmd->link_modes.supported);
	__set_bit(ETHTOOL_LINK_MODE_AUI_BIT,   			cmd->link_modes.supported);;

	memcpy(cmd->link_modes.advertising, cmd->link_modes.supported, sizeof(cmd->link_modes.advertising));

	RET(0);

} // CX4_get_link_ksettings()
#endif
//-------------------------------------------------------------------------------------------------
#ifdef ETHTOOL_SLINKSETTINGS

int CX4_set_link_ksettings(struct net_device *netdev, const struct ethtool_link_ksettings *cmd)
{
	ERR("CX4_set_link_ksettings() not implemented\n");

	return -EPERM;

} // CX4_set_link_ksettings()
#endif
//-------------------------------------------------------------------------------------------------

__init void CX4_register_settings(struct bdx_priv *priv)
{
    priv->phy_ops.get_settings 		 = CX4_get_settings;
    priv->phy_ops.set_settings 		 = CX4_set_settings;
#ifdef ETHTOOL_GLINKSETTINGS
    priv->phy_ops.get_link_ksettings = CX4_get_link_ksettings;
#endif
#ifdef ETHTOOL_SLINKSETTINGS
    priv->phy_ops.set_link_ksettings = CX4_set_link_ksettings;
#endif
    priv->autoneg     				 = AUTONEG_DISABLE;
} // CX4_register_settings()

//-------------------------------------------------------------------------------------------------

