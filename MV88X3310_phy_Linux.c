#include "tn40.h"

int MV88X3310_set_speed(struct bdx_priv *priv, s32 speed);

//-------------------------------------------------------------------------------------------------

int MV88X3310_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	ecmd->supported = (SUPPORTED_10000baseT_Full | SUPPORTED_1000baseT_Full | SUPPORTED_100baseT_Full | SUPPORTED_Autoneg | SUPPORTED_TP | SUPPORTED_Pause);
	if (!priv->advertising)
	{
		priv->advertising = ecmd->supported;
		priv->autoneg = AUTONEG_ENABLE;
	}
	ecmd->advertising = priv->advertising;
	ecmd->speed = priv->link_speed;
	ecmd->duplex = DUPLEX_FULL;
	ecmd->port = PORT_TP;
	ecmd->transceiver = XCVR_INTERNAL;
	ecmd->autoneg = priv->autoneg;
#if defined(ETH_TP_MDI_AUTO)
	ecmd->eth_tp_mdix = ETH_TP_MDI_AUTO;
#else
#if (!defined VM_KLNX)
	ecmd->eth_tp_mdix = ETH_TP_MDI | ETH_TP_MDI_X;
#endif
#endif
	return 0;

} // MV88X3310_get_settings()


//-------------------------------------------------------------------------------------------------

int MV88X3310_set_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
	struct bdx_priv *priv 	= netdev_priv(netdev);
	s32 			speed	= ethtool_cmd_speed(ecmd);
	int				rVal    = 0;

	priv->autoneg = ecmd->autoneg;

	DBG("MV88X3310 ecmd->cmd=%x\n", ecmd->cmd);
	DBG("MV88X3310 speed=%u\n",speed);
	DBG("MV88X3310 ecmd->autoneg=%u\n",ecmd->autoneg);
	if(priv->autoneg == AUTONEG_ENABLE)
	{
        priv->advertising = (ADVERTISED_10000baseT_Full | ADVERTISED_1000baseT_Full | ADVERTISED_100baseT_Full | ADVERTISED_Autoneg | SUPPORTED_TP | ADVERTISED_Pause);

	}
	else
	{
		switch(speed)
		{
			case 10000: //10G
				priv->advertising = (ADVERTISED_10000baseT_Full | ADVERTISED_Pause);
				break;

			case 5000: //5G
				priv->advertising = (ADVERTISED_Pause);
				break;

			case 2500: //2.5G
				priv->advertising = (ADVERTISED_Pause);
				break;

			case 1000:  //1G
				priv->advertising = (ADVERTISED_1000baseT_Full | ADVERTISED_Pause);
				break;

			case 100:   //100m
				priv->advertising = (ADVERTISED_100baseT_Full | ADVERTISED_Pause);
				break;

			default :
				ERR("does not support speed %u\n", speed);
				rVal = -EINVAL;
				break;
		}
	}
	if (rVal == 0)
	{
		rVal = MV88X3310_set_speed(priv, speed);
	}

	return rVal;

} // MV88X3310_set_settings()
//-------------------------------------------------------------------------------------------------
#ifdef ETHTOOL_GLINKSETTINGS
#define MV88X3310_ALL_SPEEDS	(0xFFFFFFFF)

static void MV88X3310_set_link_mode(unsigned long *bits, u32 speed)
{
	bitmap_zero(bits, __ETHTOOL_LINK_MODE_MASK_NBITS);
	__set_bit(ETHTOOL_LINK_MODE_Pause_BIT,   	  		bits);
	__set_bit(ETHTOOL_LINK_MODE_TP_BIT,   	  			bits);;
	if (speed == MV88X3310_ALL_SPEEDS)
	{
		__set_bit(ETHTOOL_LINK_MODE_10000baseT_Full_BIT, 	bits);
#if (!defined(RHEL_RELEASE_CODE)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0))
		__set_bit(ETHTOOL_LINK_MODE_5000baseT_Full_BIT, 	bits);
		__set_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, 	bits);
#endif
		__set_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT,  	bits);
		__set_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT,   	bits);
		__set_bit(ETHTOOL_LINK_MODE_Autoneg_BIT,   	  		bits);
	}
	else
	{
		__set_bit(speed,   	bits);
	}

} // MV88X3310_set_link_mode()
//-------------------------------------------------------------------------------------------------

int MV88X3310_get_link_ksettings(struct net_device *netdev, struct ethtool_link_ksettings *cmd)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	cmd->base.speed				= priv->link_speed;
	cmd->base.port 				= PORT_TP;
	cmd->base.autoneg 			= AUTONEG_ENABLE;
	cmd->base.duplex			= DUPLEX_FULL;
#if defined(ETH_TP_MDI_AUTO)
	cmd->base.eth_tp_mdix		= ETH_TP_MDI_AUTO;
	cmd->base.eth_tp_mdix_ctrl	= ETH_TP_MDI_AUTO;
#else
#if (!defined VM_KLNX)
	cmd->base.eth_tp_mdix 		= ETH_TP_MDI | ETH_TP_MDI_X;
	cmd->base.eth_tp_mdix_ctrl	= ETH_TP_MDI | ETH_TP_MDI_X;
#endif
#endif
	MV88X3310_set_link_mode(cmd->link_modes.supported, MV88X3310_ALL_SPEEDS);

	if (priv->autoneg == AUTONEG_ENABLE)
	{
		memcpy(priv->link_advertising, cmd->link_modes.supported, sizeof(priv->link_advertising));
	}
	memcpy(cmd->link_modes.advertising, priv->link_advertising, sizeof(cmd->link_modes.advertising));

	return 0;

} // MV88X3310_get_link_ksettings()

#endif
//-------------------------------------------------------------------------------------------------
#ifdef ETHTOOL_SLINKSETTINGS

int MV88X3310_set_link_ksettings(struct net_device *netdev, const struct ethtool_link_ksettings *cmd)
{
	struct bdx_priv *priv = netdev_priv(netdev);
	int				rVal  = 0;

	priv->autoneg = cmd->base.autoneg;
	if (priv->autoneg == AUTONEG_ENABLE)
	{
		MV88X3310_set_link_mode(priv->link_advertising, MV88X3310_ALL_SPEEDS);
	}
	else
	{
		switch (cmd->base.speed)
		{
			case 10000: //10G
				MV88X3310_set_link_mode(priv->link_advertising, ETHTOOL_LINK_MODE_10000baseT_Full_BIT);
				break;

			case 5000: //5G
#if (!defined(RHEL_RELEASE_CODE)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0))
				MV88X3310_set_link_mode(priv->link_advertising, ETHTOOL_LINK_MODE_5000baseT_Full_BIT);
#endif
				break;

			case 2500: //2.5G
#if (!defined(RHEL_RELEASE_CODE)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0))
				MV88X3310_set_link_mode(priv->link_advertising, ETHTOOL_LINK_MODE_2500baseT_Full_BIT);
#endif
				break;

			case 1000:  //1G
				MV88X3310_set_link_mode(priv->link_advertising, ETHTOOL_LINK_MODE_1000baseT_Full_BIT);
				break;

			case 100:   //100m
				MV88X3310_set_link_mode(priv->link_advertising, ETHTOOL_LINK_MODE_100baseT_Full_BIT);
				break;

			default :
				ERR("does not support speed %u\n", cmd->base.speed);
				rVal = -EINVAL;
				break;
		}
	}
	if (rVal == 0)
	{
		rVal = MV88X3310_set_speed(priv, cmd->base.speed);
	}

	return 0;

} // MV88X3310_set_link_ksettings()

#endif
//----------------------------------------- EEE - IEEEaz ------------------------------------------

#ifdef _EEE_
#ifdef ETHTOOL_GEEE
int MV88X3310_get_eee(struct net_device *netdev, struct ethtool_eee *edata)
{
	ERR("EEE is not implemented yet\n");

	return -1;

} // MV88X3310_get_eee()
#endif
//-------------------------------------------------------------------------------------------------
#ifdef ETHTOOL_SEEE
int MV88X3310_set_eee(struct bdx_priv *priv)
{
	ERR("EEE is not implemented yet\n");

	return -1;

} // MV88X3310_set_eee()
#endif
//-------------------------------------------------------------------------------------------------

int MV88X3310_reset_eee(struct bdx_priv *priv)
{
	ERR("EEE is not implemented yet\n");

	return -1;

} // MV88X3310_reset_eee()

#endif
//-------------------------------------------------------------------------------------------------

__init void MV88X3310_register_settings(struct bdx_priv *priv)
{
    priv->phy_ops.get_settings 		 = MV88X3310_get_settings;
    priv->phy_ops.set_settings 		 = MV88X3310_set_settings;
#ifdef ETHTOOL_GLINKSETTINGS
    priv->phy_ops.get_link_ksettings = MV88X3310_get_link_ksettings;
#endif
#ifdef ETHTOOL_SLINKSETTINGS
    priv->phy_ops.set_link_ksettings = MV88X3310_set_link_ksettings;
#endif
    priv->autoneg     				 = AUTONEG_ENABLE;
#ifdef _EEE_
#ifdef ETHTOOL_GEEE
    priv->phy_ops.get_eee 	   = MV88X3310_get_eee;
#endif
#ifdef ETHTOOL_SEEE
    priv->phy_ops.set_eee 	   = MV88X3310_set_eee;
#endif
    priv->phy_ops.reset_eee    = MV88X3310_reset_eee;
#endif
} // MV88X3310_register_settings()

//-------------------------------------------------------------------------------------------------

