#include "tn40.h"
#include "MV88X3120_phy.h"

#define MANUF_MODEL_NUM_MASK        (0x3F)
#define MANUF_MODEL_NUM_BIT_POS     (4)
#define MV88X3120_FILE_OFFSET  (16)
#define AN_IN_PROGRESS    			(0x0000)	/*  Speed not resolved yet, auto-negotiation in progress */
/* #define SPEED_10BT                           (0x0001)                        Not supported */
#define SPEED_RES_100M_FD 			(0x0002)	/*  100M Full Duplex */
#define SPEED_RES_1GIG_FD 			(0x0003)	/*  1Gig Full Duplex */
#define SPEED_RES_10GIG   			(0x0004)	/*  10Gig Full Duplex */
#define SPEED_RES_100M_HD 			(0x0005)	/*  100M Half Duplex */
/* #define SPEED_RES_1GIG_HD            (0x0006)                        Not supported for SGMII */

#define LINK_LOOP_MAX   			(25)

void MV88X3120_register_settings(struct bdx_priv *priv);
int MV88X3120_mdio_reset(struct bdx_priv *priv, int port, unsigned short phy);
u32 MV88X3120_link_changed(struct bdx_priv *priv);
void MV88X3120_leds(struct bdx_priv *priv, enum PHY_LEDS_OP op);
#ifdef _EEE_
void MV88X3120_enable_eee(struct bdx_priv *priv);
#endif

int MV88X3120_set_speed(struct bdx_priv *priv, signed int speed)
{
	u16 val = 0;
	u16 port = priv->phy_mdio_port;
	int rVal = 0;

	if (priv->autoneg == AUTONEG_ENABLE) {
		val = (1 << 3) | (1 << 2) | (1 << 1);
	} else {
		priv->autoneg = AUTONEG_DISABLE;
		switch (speed) {
		case 10000:	/*10G */
			pr_debug("MV88X3120 speed 10G\n");
			val = (1 << 3);
			break;

		case 1000:	/*1G */
			pr_debug("MV88X3120 speed 1G\n");
			val = (1 << 2);
			break;

		case 100:	/*100m */
			pr_debug("MV88X3120 speed 100m\n");
			val = (1 << 1);
			break;

		default:
			pr_err("does not support speed %u\n", speed);
			rVal = -1;
			break;
		}
	}
	if (rVal == 0) {
		/* set speed */
		BDX_MDIO_WRITE(priv, 0x01, 49192, val);
		/* restart autoneg */
		val = (1 << 12) | (1 << 9) | (1 << 13);
		BDX_MDIO_WRITE(priv, 0x07, 0x0, val);
	}

	return rVal;

}

__init int MV88X3120_mdio_reset(struct bdx_priv *priv, int port,
				unsigned short phy)
{
	unsigned short val, val1;
	int rVal = 0;
	u32 j, MV88X3120_phy_initdata_size =
	    sizeof(MV88X3120_phy_initdata) / sizeof(u16);

	val = bdx_mdio_read(priv, 1, port, 3);
	val = (val & (MANUF_MODEL_NUM_MASK << MANUF_MODEL_NUM_BIT_POS)) >> MANUF_MODEL_NUM_BIT_POS;	/* modelNo */
	val1 = bdx_mdio_read(priv, 3, port, 53249);	/* revNo */
	if (val == 8 && val1 < 3) {
		val = 9;
	}
	pr_debug("MV88X3120 modelNo = %d revNo %d\n", val, val1);
	do {
		val = bdx_mdio_read(priv, 1, port, 49152);
		BDX_MDIO_WRITE(priv, 1, 49152, (val | 1 << 15));
		msleep(250);
		if ((val = bdx_mdio_read(priv, 3, port, 0xD000)) != 0x000A) {
			pr_err
			    ("MV88X3120 Initialization Error. Expected 0x000A, read 0x%04X\n",
			     (unsigned)val);
			rVal = -1;
			break;
		} else {
			pr_debug("MV88X3120 Initializing data...\n");
		}
		BDX_MDIO_WRITE(priv, 3, 0xD004, 0);
		BDX_MDIO_WRITE(priv, 3, 0xD005, 0);
		for (j = MV88X3120_FILE_OFFSET; j < MV88X3120_phy_initdata_size;
		     j++) {
			val = swab16(MV88X3120_phy_initdata[j]);
			BDX_MDIO_WRITE(priv, 3, 0xD006, val);
		}
		pr_debug("MV88X3120 loaded %d 16bit words\n",
			 MV88X3120_phy_initdata_size);

		val = bdx_mdio_read(priv, 3, port, 0xD000);
		BDX_MDIO_WRITE(priv, 3, 0xD000, (val | (1 << 6)));
		msleep(500);
		if (!((val = bdx_mdio_read(priv, 3, port, 0xD000)) & (1 << 4))) {
			pr_err
			    ("MV88X3120 initdata not applied. Expected bit 4 to be 1, read 0x%04X\n",
			     (unsigned)val);
			rVal = -1;
			break;
		} else {
			pr_err("MV88X3120 initdata applied\n");
		}
		/* verify reset complete */
		for (j = 0; j < 10; j++) {
			if (!
			    ((val =
			      bdx_mdio_read(priv, 1, port, 0)) & (1 << 15))) {
				break;
			}
			pr_debug("MV88X3120 waiting for reset complete 0x%x\n",
				 val);
			msleep(10);
		}
		BDX_MDIO_WRITE(priv, 4, 49152, 0x00c2);

		BDX_MDIO_WRITE(priv, 1, 36866, 0x0009);
		val = bdx_mdio_read(priv, 1, port, 49190);
		val1 = bdx_mdio_read(priv, 1, port, 49191);
		pr_err("MV88X3120 I/D version is %d.%d.%d.%d\n",
		       ((val & 0xff00) >> 8), (val & 0x00ff),
		       ((val1 & 0xff00) >> 8), (val1 & 0x00ff));
		val = bdx_mdio_read(priv, 7, port, 1);

		val = bdx_mdio_read(priv, 1, port, 49154);
		val &= ~(1 << 6);
		BDX_MDIO_WRITE(priv, 1, 49154, val);

	}
	while (0);

	return rVal;

}

static int MV88X3120_get_link_speed(struct bdx_priv *priv)
{
	unsigned short val, leds;
	int link = 0;

#ifdef DEBUG
	val = bdx_mdio_read(priv, 3, priv->phy_mdio_port, 8);
	val = bdx_mdio_read(priv, 3, priv->phy_mdio_port, 8);
	pr_debug("MV88X3120 3.8 = 0x%x\n", val);
	val = bdx_mdio_read(priv, 3, priv->phy_mdio_port, 1);
	val = bdx_mdio_read(priv, 3, priv->phy_mdio_port, 1);
	pr_debug("MV88X3120 3.1 = 0x%x\n", val);
	val = bdx_mdio_read(priv, 1, priv->phy_mdio_port, 49192);
	val = bdx_mdio_read(priv, 1, priv->phy_mdio_port, 49192);
	pr_debug("MV88X3120 1.49192 = 0x%x\n", val);
#endif
	val = bdx_mdio_read(priv, 1, priv->phy_mdio_port, 36869);
	val = bdx_mdio_read(priv, 1, priv->phy_mdio_port, 36869);
	pr_debug("MV88X3120 1.36869 = 0x%x\n", val);
	val = bdx_mdio_read(priv, 7, priv->phy_mdio_port, 1);
	val = bdx_mdio_read(priv, 7, priv->phy_mdio_port, 1);
	pr_debug("MV88X3120 7.1 = 0x%x\n", val);
	if (val & (1 << 2)) {	/* Link up */
		leds = bdx_mdio_read(priv, 1, priv->phy_mdio_port, 49161);
		val = bdx_mdio_read(priv, 1, priv->phy_mdio_port, 49192);
		val = (val & 0x00F0) >> 4;
		switch (val) {
		case SPEED_RES_10GIG:
			link = SPEED_10000;
			bdx_mdio_write(priv, 1, priv->phy_mdio_port, 49161, (leds & 0xfff0) | 0x0009);	/* led_speed0 on */
			pr_debug("MV88X3120 10G link detected\n");
			break;

		case SPEED_RES_1GIG_FD:
			link = SPEED_1000;
			bdx_mdio_write(priv, 1, priv->phy_mdio_port, 49161, (leds & 0xfff0) | 0x0006);	/* led_speed1 on */
			pr_debug("MV88X3120 1G link detected\n");
			break;

		case SPEED_RES_100M_FD:
		case SPEED_RES_100M_HD:
			link = SPEED_100;
			bdx_mdio_write(priv, 1, priv->phy_mdio_port, 49161, (leds & 0xfff0) | 0x000a);	/* led_speeds off */
			pr_debug("MV88X3120 100M %s link detected\n",
				 (val ==
				  SPEED_RES_100M_HD) ? "Half Duplex" : "");
			break;

		case AN_IN_PROGRESS:
			bdx_mdio_write(priv, 1, priv->phy_mdio_port, 49161, (leds & 0xfff0) | 0x000a);	/* led_speeds off */
			pr_debug("MV88X3120 auto negotiation in progress...\n");
			break;

		default:
			bdx_mdio_write(priv, 1, priv->phy_mdio_port, 49161, (leds & 0xfff0) | 0x000a);	/* led_speeds off */
			pr_debug
			    ("MV88X3120 internal error - unknown link speed value (0x%x) !\n",
			     val);
			break;
		}
		priv->errmsg_count = 0;
	} else {
		if (++priv->errmsg_count < MAX_ERRMSGS) {
			pr_debug("MV88X3120 link down.\n");
		}
	}
	return link;

}

u32 MV88X3120_link_changed(struct bdx_priv *priv)
{
	u32 link, speed;

	speed = MV88X3120_get_link_speed(priv);
	if (speed != (u32) priv->link_speed) {
		switch (speed) {
		case SPEED_10000:
			pr_debug("MV88X3120 10G link detected\n");
#ifdef _EEE_
			MV88X3120_enable_eee(priv);
#endif
			break;
		case SPEED_1000:
			pr_debug("MV88X3120 1G link detected\n");
			break;
		case SPEED_100:
			pr_debug("MV88X3120 100M link detected\n");
			break;
		default:
			pr_debug("MV88X3120 link down.\n");
			break;
		}
		bdx_speed_changed(priv, speed);
	}
	link = 0;
	if ((!speed)
	    || (!(link = (READ_REG(priv, regMAC_LNK_STAT) & MAC_LINK_STAT)))) {
		u32 timeout;
		u16 leds = bdx_mdio_read(priv, 1, priv->phy_mdio_port, 49161);
		bdx_mdio_write(priv, 1, priv->phy_mdio_port, 49161, (leds & 0xfff0) | 0x000a);	/* led_speeds off */
		if (speed) {
			timeout = 1000000;	/* 1/5 sec */
			if (priv->link_loop_cnt++ > LINK_LOOP_MAX) {
				bdx_speed_changed(priv, 0);
				priv->link_loop_cnt = 0;
				pr_debug
				    ("MV88X3120 trying to recover link after %d tries\n",
				     LINK_LOOP_MAX);
			}
		} else {
			timeout = 5000000;	/* 1 sec */
		}
		pr_debug
		    ("MV88X3120 link = 0x%x speed = 0x%x setting %d timer\n",
		     link, speed, timeout);
		WRITE_REG(priv, 0x5150, timeout);
	}

	return link;

}

void MV88X3120_leds(struct bdx_priv *priv, enum PHY_LEDS_OP op)
{
	switch (op) {
	case PHY_LEDS_SAVE:
		priv->phy_ops.leds[0] =
		    bdx_mdio_read(priv, 1, priv->phy_mdio_port, 49161);
		bdx_mdio_write(priv, 1, priv->phy_mdio_port, 49161, 0xf);
		bdx_mdio_write(priv, 1, priv->phy_mdio_port, 49161, 0xff);
		break;

	case PHY_LEDS_RESTORE:
		bdx_mdio_write(priv, 1, priv->phy_mdio_port, 49161,
			       priv->phy_ops.leds[0]);
		break;

	case PHY_LEDS_ON:
		break;

	case PHY_LEDS_OFF:
		break;

	default:
		pr_debug("MV88X3120_leds() unknown op 0x%x\n", op);
		break;

	}

}

__init enum PHY_TYPE MV88X3120_register(struct bdx_priv *priv)
{
	priv->phy_ops.mdio_reset = MV88X3120_mdio_reset;
	priv->phy_ops.link_changed = MV88X3120_link_changed;
	priv->phy_ops.ledset = MV88X3120_leds;
	priv->phy_ops.mdio_speed = MDIO_SPEED_6MHZ;
	MV88X3120_register_settings(priv);

	return PHY_TYPE_MV88X3120;

}
