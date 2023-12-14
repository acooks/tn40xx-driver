/*******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "tn40.h"
#include "tn40_fw.h"

static void bdx_scan_pci(void);

uint bdx_force_no_phy_mode = 0;
module_param_named(no_phy, bdx_force_no_phy_mode, int, 0644);
MODULE_PARM_DESC(bdx_force_no_phy_mode, "no_phy=1 - force no phy mode (CX4)");

__initdata static u32 g_ndevices = 0;
__initdata static u32 g_ndevices_loaded = 0;
__initdata spinlock_t g_lock __initdata;
__initdata DEFINE_SPINLOCK(g_lock);

#define LDEV(_vid,_pid,_subdev,_msi,_ports,_phya,_phyb,_name)  \
    {_vid,_pid,_subdev,_msi,_ports,PHY_TYPE_##_phya,PHY_TYPE_##_phyb}
static struct bdx_device_descr bdx_dev_tbl[] = {
	LDEV(TEHUTI_VID, 0x4010, 0x4010, 1, 1, CX4, NA, "TN4010 Clean SROM"),
	LDEV(TEHUTI_VID, 0x4020, 0x3015, 1, 1, CX4, NA,
	     "TN9030 10GbE CX4 Ethernet Adapter"),
#ifdef PHY_MUSTANG
	LDEV(TEHUTI_VID, 0x4020, 0x2040, 1, 1, CX4, NA,
	     "Mustang-200 10GbE Ethernet Adapter"),
#endif
#ifdef PHY_QT2025
	LDEV(TEHUTI_VID, 0x4022, 0x3015, 1, 1, QT2025, NA,
	     "TN9310 10GbE SFP+ Ethernet Adapter"),
	LDEV(TEHUTI_VID, 0x4022, 0x4d00, 1, 1, QT2025, NA,
	     "D-Link DXE-810S 10GbE SFP+ Ethernet Adapter"),
	LDEV(TEHUTI_VID, 0x4022, 0x8709, 1, 1, QT2025, NA,
	     "ASUS XG-C100F 10GbE SFP+ Ethernet Adapter"),
	LDEV(TEHUTI_VID, 0x4022, 0x8103, 1, 1, QT2025, NA,
	     "Edimax 10 Gigabit Ethernet SFP+ PCI Express Adapter"),
#endif
#ifdef PHY_MV88X3120
	LDEV(TEHUTI_VID, 0x4024, 0x3015, 1, 1, MV88X3120, NA,
	     "TN9210 10GBase-T Ethernet Adapter"),
#endif
#ifdef PHY_MV88X3310
	LDEV(TEHUTI_VID, 0x4027, 0x3015, 1, 1, MV88X3310, NA,
	     "TN9710P 10GBase-T/NBASE-T Ethernet Adapter"),
	LDEV(TEHUTI_VID, 0x4027, 0x8104, 1, 1, MV88X3310, NA,
	     "Edimax 10 Gigabit Ethernet PCI Express Adapter"),
	LDEV(TEHUTI_VID, 0x4027, 0x0368, 1, 1, MV88X3310, NA,
	     "Buffalo LGY-PCIE-MG Ethernet Adapter"),
	LDEV(TEHUTI_VID, 0x4027, 0x1546, 1, 1, MV88X3310, NA,
	     "IOI GE10-PCIE4XG202P 10Gbase-T/NBASE-T Ethernet Adapter"),
	LDEV(TEHUTI_VID, 0x4027, 0x1001, 1, 1, MV88X3310, NA,
	     "LR-Link LREC6860BT 10 Gigabit Ethernet Adapter"),
	LDEV(TEHUTI_VID, 0x4027, 0x3310, 1, 1, MV88X3310, NA,
	     "QNAP PCIe Expansion Card"),
#endif
#ifdef PHY_MV88E2010
	LDEV(TEHUTI_VID, 0x4527, 0x3015, 1, 1, MV88E2010, NA,
	     "TN9710Q 5GBase-T/NBASE-T Ethernet Adapter"),
#endif
#ifdef PHY_TLK10232
	LDEV(TEHUTI_VID, 0x4026, 0x3015, 1, 1, TLK10232, NA,
	     "TN9610 10GbE SFP+ Ethernet Adapter"),
	LDEV(TEHUTI_VID, 0x4026, 0x1000, 1, 1, TLK10232, NA,
	     "LR-Link LREC6860AF 10 Gigabit Ethernet Adapter"),
#endif
#ifdef PHY_AQR105
	LDEV(TEHUTI_VID, 0x4025, 0x2900, 1, 1, AQR105, NA,
	     "D-Link DXE-810T 10GBase-T Ethernet Adapter"),
	LDEV(TEHUTI_VID, 0x4025, 0x3015, 1, 1, AQR105, NA,
	     "TN9510 10GBase-T/NBASE-T Ethernet Adapter"),
	LDEV(TEHUTI_VID, 0x4025, 0x8102, 1, 1, AQR105, NA,
	     "Edimax 10 Gigabit Ethernet PCI Express Adapter"),
	LDEV(PROMISE_VID, 0x7203, 0x7203, 1, 1, AQR105, NA,
	     "Promise SANLink3 T1 10 Gigabit Ethernet Adapter"),
#endif
	{ 0 }
};

static struct pci_device_id bdx_pci_tbl[] = {
	{ TEHUTI_VID, 0x4010, TEHUTI_VID, 0x4010, 0, 0, 0 },
	{ TEHUTI_VID, 0x4020, TEHUTI_VID, 0x3015, 0, 0, 0 },
#ifdef PHY_MUSTANG
	{ TEHUTI_VID, 0x4020, 0x180C, 0x2040, 0, 0, 0 },
#endif
#ifdef PHY_QT2025
	{ TEHUTI_VID, 0x4022, TEHUTI_VID, 0x3015, 0, 0, 0 },
	{ TEHUTI_VID, 0x4022, DLINK_VID, 0x4d00, 0, 0, 0 },
	{ TEHUTI_VID, 0x4022, ASUS_VID, 0x8709, 0, 0, 0 },
	{ TEHUTI_VID, 0x4022, EDIMAX_VID, 0x8103, 0, 0, 0 },
#endif
#ifdef PHY_MV88X3120
	{ TEHUTI_VID, 0x4024, TEHUTI_VID, 0x3015, 0, 0, 0 },
#endif
#ifdef PHY_MV88X3310
	{ TEHUTI_VID, 0x4027, TEHUTI_VID, 0x3015, 0, 0, 0 },
	{ TEHUTI_VID, 0x4027, EDIMAX_VID, 0x8104, 0, 0, 0 },
	{ TEHUTI_VID, 0x4027, BUFFALO_VID, 0x0368, 0, 0, 0 },
	{ TEHUTI_VID, 0x4027, 0x1546, 0x4027, 0, 0, 0 },
	{ TEHUTI_VID, 0x4027, 0x4C52, 0x1001, 0, 0, 0 },
	{ TEHUTI_VID, 0x4027, 0x1BAA, 0x3310, 0, 0, 0 },
#endif
#ifdef PHY_MV88E2010
	{ TEHUTI_VID, 0x4527, TEHUTI_VID, 0x3015, 0, 0, 0 },
#endif
#ifdef PHY_TLK10232
	{ TEHUTI_VID, 0x4026, TEHUTI_VID, 0x3015, 0, 0, 0 },
	{ TEHUTI_VID, 0x4026, 0x4C52, 0x1000, 0, 0, 0 },
#endif
#ifdef PHY_AQR105
	{ TEHUTI_VID, 0x4025, DLINK_VID, 0x2900, 0, 0, 0 },
	{ TEHUTI_VID, 0x4025, TEHUTI_VID, 0x3015, 0, 0, 0 },
	{ TEHUTI_VID, 0x4025, EDIMAX_VID, 0x8102, 0, 0, 0 },
	{ PROMISE_VID, 0x7203, PROMISE_VID, 0x7203, 0, 0, 0 },
#endif
	{ 0 }
};

MODULE_DEVICE_TABLE(pci, bdx_pci_tbl);

/* Definitions needed by ISR or NAPI functions */
static void bdx_rx_alloc_buffers(struct bdx_priv *priv);
static void bdx_tx_cleanup(struct bdx_priv *priv);
static int bdx_rx_receive(struct bdx_priv *priv, struct rxd_fifo *f,
			  int budget);
static int bdx_tx_init(struct bdx_priv *priv);
static int bdx_rx_init(struct bdx_priv *priv);
static void bdx_tx_free(struct bdx_priv *priv);
static void bdx_rx_free(struct bdx_priv *priv);
/* Definitions needed by FW loading */
static void bdx_tx_push_desc_safe(struct bdx_priv *priv, void *data, int size);
static inline int bdx_rxdb_available(struct rxdb *db);
/* Definitions needed by bdx_probe */
static void bdx_ethtool_ops(struct net_device *netdev);
static int bdx_rx_alloc_pages(struct bdx_priv *priv);
static void bdx_rx_free_pages(struct bdx_priv *priv);
#ifdef _DRIVER_RESUME_

static int bdx_suspend(struct device *dev);
static int bdx_resume(struct device *dev);
#endif
/*#define USE_RSS */
#if defined(USE_RSS)
/* bdx_init_rss - Initialize RSS hash HW function.
 *
 * @priv       - NIC private structure
 */
static int bdx_init_rss(struct bdx_priv *priv)
{
	int i;
	u32 seed;

	/* Disable RSS before starting the configuration */

	WRITE_REG(priv, regRSS_CNG, 0);
	/*
	 * Notes:
	 *      - We do not care about the CPU, we just need the hash value, the
	 *        Linux kernel is doing the rest of the work for us. We set the
	 *        CPU table length to 0.
	 *
	 *      - We use random32() to initialize the Toeplitz secret key. This is
	 *        probably not cryptographically secure way but who cares.
	 */

	/* UPDATE THE HASH SECRET KEY */
	seed = (uint32_t) (0xFFFFFFFF & jiffies);
	prandom_seed(seed);
	for (i = 0; i < 4 * RSS_HASH_LEN; i += 4) {
		u32 rnd = prandom_u32();
		WRITE_REG(priv, regRSS_HASH_BASE + 4 * i, rnd);
		pr_debug("bdx_init_rss() rnd 0x%x\n", rnd);
	}
	WRITE_REG(priv, regRSS_CNG,
		  RSS_ENABLED | RSS_HFT_TOEPLITZ |
		  RSS_HASH_IPV4 | RSS_HASH_TCP_IPV4 |
		  RSS_HASH_IPV6 | RSS_HASH_TCP_IPV6);

	pr_debug("regRSS_CNG =%x\n", READ_REG(priv, regRSS_CNG));
	return 0;
}
#else
#define    bdx_init_rss(priv)
#endif

#if defined(TN40_DEBUG)
int g_dbg = 0;
#endif

#if defined(TN40_REGLOG)
int g_regLog = 0;
#endif

#if defined (TN40_MEMLOG)
int g_memLog = 0;
#endif

#if defined(TN40_DEBUG)

void dbg_printFifo(struct fifo *m, char *fName)
{
	pr_debug("%s fifo:\n", fName);
	pr_debug("WPTR 0x%x = 0x%x RPTR 0x%x = 0x%x\n",
		 m->reg_WPTR, m->wptr, m->reg_RPTR, m->rptr);

}

void dbg_printRegs(struct bdx_priv *priv, char *msg)
{

	pr_debug("* %s * \n", msg);
	pr_debug("~~~~~~~~~~~~~\n");
	pr_debug("veneto:");
	pr_debug("pc = 0x%x li = 0x%x ic = %d\n", READ_REG(priv, 0x2300),
		 READ_REG(priv, 0x2310), READ_REG(priv, 0x2320));
	dbg_printFifo(&priv->txd_fifo0.m, (char *)"TXD");
	dbg_printFifo(&priv->rxf_fifo0.m, (char *)"RXF");
	dbg_printFifo(&priv->rxd_fifo0.m, (char *)"RXD");
	pr_debug("~~~~~~~~~~~~~\n");

}

void dbg_printPBL(struct pbl *pbl)
{
	pr_debug("pbl: len %u pa_lo 0x%x pa_hi 0x%x\n", pbl->len, pbl->pa_lo,
		 pbl->pa_hi);

}

void dbg_printPkt(char *pkt, u16 len)
{
	int i;

	pr_info("RX: len=%d\n", len);
	for (i = 0; i < len; i = i + 16)
		pr_err
		    ("%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x ",
		     (0xff & pkt[i]), (0xff & pkt[i + 1]), (0xff & pkt[i + 2]),
		     (0xff & pkt[i + 3]), (0xff & pkt[i + 4]),
		     (0xff & pkt[i + 5]), (0xff & pkt[i + 6]),
		     (0xff & pkt[i + 7]), (0xff & pkt[i + 8]),
		     (0xff & pkt[i + 9]), (0xff & pkt[i + 10]),
		     (0xff & pkt[i + 11]), (0xff & pkt[i + 12]),
		     (0xff & pkt[i + 13]), (0xff & pkt[i + 14]),
		     (0xff & pkt[i + 15]));
	pr_info("\n");

}

void dbg_printSkb(struct sk_buff *skb)
{
/*
DBG("SKB: len=%d data_len=%d, truesize=%d head=%p "
	"data=%p end=0x%x page=%p page_offset=%d size=%d"
	"nr_frags=%d\n",
	skb->len, skb->data_len, skb->truesize,
	skb->head, skb->data, skb->end,
	skb_shinfo(skb)->frags[0].page.p ,
	skb_shinfo(skb)->frags[0].page_offset,
	skb_shinfo(skb)->frags[0].size,
	skb_shinfo(skb)->nr_frags) ;
*/
}

void dbg_printIoctl(void)
{
	pr_info
	    ("DBG_ON %d, DBG_OFF %d, DBG_SUSPEND %d, DBG_RESUME %d DBG_PRINT_PAGE_TABLE %d\n",
	     DBG_START_DBG, DBG_STOP_DBG, DBG_SUSPEND, DBG_RESUME,
	     DBG_PRINT_PAGE_TABLE);

}

#else
#define dbg_printRegs(priv, msg)
#define dbg_printPBL(pbl)
#define dbg_printFifo(m, fName)
#define dbg_printPkt(pkt)
#define dbg_printIoctl()
#endif

#ifdef TN40_THUNDERBOLT

u32 tbReadReg(struct bdx_priv *priv, u32 reg)
{
	u32 rVal;

	if (!priv->bDeviceRemoved) {
		rVal = readl(priv->pBdxRegs + reg);
		if (rVal == 0xFFFFFFFF) {
			priv->bDeviceRemoved = 1;
		}
	} else {
		rVal = 0xFFFFFFFF;
	}

	return rVal;

}

#endif

#ifdef REGLOG
int g_regLog = 0;
u32 bdx_readl(struct bdx_priv *priv, u32 reg)
{

	u32 val;
#ifdef TN40_THUNDERBOLT
	val = tbReadReg(priv, reg);
#else
	val = readl(priv->pBdxRegs + reg);
#endif
	if (g_regLog) {
		pr_info("regR 0x%x = 0x%x\n", (u32) (((u64) reg) & 0xFFFF),
			val);
	}
	return val;
}
#endif

/*************************************************************************
 *     MDIO Interface                            *
 *************************************************************************/
/* bdx_mdio_get - read MDIO_CMD_STAT until the device is not busy
 * @regs    - NIC register space pointer
 *
 * returns the CMD_STAT value read, or -1 (0xFFFFFFFF) for failure
 * (since the busy bit should be off, -1 can never be a valid value for
 * mdio_get).
 */

u32 bdx_mdio_get(struct bdx_priv *priv)
{
	void __iomem *regs = priv->pBdxRegs;

#define BDX_MAX_MDIO_BUSY_LOOPS 1024
	int tries = 0;
	while (++tries < BDX_MAX_MDIO_BUSY_LOOPS) {
		u32 mdio_cmd_stat = readl(regs + regMDIO_CMD_STAT);

		if (GET_MDIO_BUSY(mdio_cmd_stat) == 0) {

			return mdio_cmd_stat;
		}
	}
	pr_err("MDIO busy!\n");
	return 0xFFFFFFFF;

}

/* bdx_mdio_read - read a 16bit word through the MDIO interface
 * @priv
 * @device   - 5 bit device id
 * @port     - 5 bit port id
 * @addr     - 16 bit address
 * returns a 16bit value or -1 for failure
 */
int bdx_mdio_read(struct bdx_priv *priv, int device, int port, u16 addr)
{
	void __iomem *regs = priv->pBdxRegs;
	u32 tmp_reg, i;
	/* Wait until MDIO is not busy */
	if (bdx_mdio_get(priv) == 0xFFFFFFFF) {
		return -1;
	}
	i = ((device & 0x1F) | ((port & 0x1F) << 5));
	writel(i, regs + regMDIO_CMD);
	writel((u32) addr, regs + regMDIO_ADDR);
	if ((tmp_reg = bdx_mdio_get(priv)) == 0xFFFFFFFF) {
		dev_err(&priv->pdev->dev, "MDIO busy after read command\n");
		return -1;
	}
	writel(((1 << 15) | i), regs + regMDIO_CMD);
	/* Read CMD_STAT until not busy */
	if ((tmp_reg = bdx_mdio_get(priv)) == 0xFFFFFFFF) {
		dev_err(&priv->pdev->dev, "MDIO busy after read command\n");
		return -1;
	}
	if (GET_MDIO_RD_ERR(tmp_reg)) {
		dev_dbg(&priv->pdev->dev, "MDIO error after read command\n");
		return -1;
	}
	tmp_reg = readl(regs + regMDIO_DATA);

	return (int)(tmp_reg & 0xFFFF);

}

/* bdx_mdio_write - writes a 16bit word through the MDIO interface
 * @priv
 * @device    - 5 bit device id
 * @port      - 5 bit port id
 * @addr      - 16 bit address
 * @data      - 16 bit value
 * returns 0 for success or -1 for failure
 */
int bdx_mdio_write(struct bdx_priv *priv, int device, int port, u16 addr,
		   u16 data)
{
	void __iomem *regs = priv->pBdxRegs;
	u32 tmp_reg;

	/* Wait until MDIO is not busy */
	if (bdx_mdio_get(priv) == 0xFFFFFFFF) {
		return -1;
	}
	writel(((device & 0x1F) | ((port & 0x1F) << 5)), regs + regMDIO_CMD);
	writel((u32) addr, regs + regMDIO_ADDR);
	if (bdx_mdio_get(priv) == 0xFFFFFFFF) {
		return -1;
	}
	writel((u32) data, regs + regMDIO_DATA);
	/* Read CMD_STAT until not busy */
	if ((tmp_reg = bdx_mdio_get(priv)) == 0xFFFFFFFF) {
		pr_err("MDIO busy after write command\n");
		return -1;
	}
	if (GET_MDIO_RD_ERR(tmp_reg)) {
		pr_err("MDIO error after write command\n");
		return -1;
	}
	return 0;

}

void setMDIOSpeed(struct bdx_priv *priv, u32 speed)
{
	void __iomem *regs = priv->pBdxRegs;
	int mdio_cfg;

	mdio_cfg = readl(regs + regMDIO_CMD_STAT);
	if (1 == speed) {
		mdio_cfg = (0x7d << 7) | 0x08;	/* 1MHz */
	} else {
		mdio_cfg = 0xA08;	/* 6MHz */
	}
	mdio_cfg |= (1 << 6);
	writel(mdio_cfg, regs + regMDIO_CMD_STAT);
	msleep(100);

}

int bdx_mdio_look_for_phy(struct bdx_priv *priv, int port)
{
	int phy_id, i;
	int rVal = -1;

	i = port;
	setMDIOSpeed(priv, MDIO_SPEED_1MHZ);

	phy_id = bdx_mdio_read(priv, 1, i, 0x0002);	/* PHY_ID_HIGH */
	phy_id &= 0xFFFF;
	for (i = 0; i < 32; i++) {
		msleep(10);
		dev_dbg(&priv->pdev->dev, "LOOK FOR PHY: port=0x%x\n", i);
		phy_id = bdx_mdio_read(priv, 1, i, 0x0002);	/* PHY_ID_HIGH */
		phy_id &= 0xFFFF;
		if (phy_id != 0xFFFF && phy_id != 0) {
			rVal = i;
			break;
		}
	}
	if (rVal == -1) {
		dev_err(&priv->pdev->dev, "PHY not found\n");
	}

	return rVal;

}

static int __init bdx_mdio_phy_search(struct bdx_priv *priv,
				      void __iomem *regs, int *port_t,
				      unsigned short *phy_t)
{
	int i, phy_id;
	char *s;

	if (bdx_force_no_phy_mode) {
		dev_err(&priv->pdev->dev, "Forced NO PHY mode\n");
		i = 0;
	} else {
		i = bdx_mdio_look_for_phy(priv, *port_t);
		if (i >= 0) {	/* PHY  found */
			*port_t = i;
			phy_id = bdx_mdio_read(priv, 1, *port_t, 0x0002);	/* PHY_ID_HI */
			i = phy_id << 16;
			phy_id = bdx_mdio_read(priv, 1, *port_t, 0x0003);	/* PHY_ID_LOW */
			phy_id &= 0xFFFF;
			i |= phy_id;
		}
	}
	switch (i) {

#ifdef PHY_QT2025
	case 0x0043A400:
		*phy_t = PHY_TYPE_QT2025;
		s = "QT2025 10Gbps SFP+";
		*phy_t = QT2025_register(priv);
		break;
#endif

#ifdef PHY_MV88X3120
	case 0x01405896:
		s = "MV88X3120 10Gbps 10GBase-T";
		*phy_t = MV88X3120_register(priv);
		break;

#endif

#if (defined PHY_MV88X3310) || (defined PHY_MV88E2010)
	case 0x02b09aa:
	case 0x02b09ab:
		if (priv->deviceId == 0x4027) {
			s = (i ==
			     0x02b09aa) ? "MV88X3310 (A0) 10Gbps 10GBase-T" :
			    "MV88X3310 (A1) 10Gbps 10GBase-T";
			*phy_t = MV88X3310_register(priv);
		} else if (priv->deviceId == 0x4527) {
			s = (i ==
			     0x02b09aa) ? "MV88E2010 (A0) 5Gbps 5GBase-T" :
			    "MV88E2010 (A1) 5Gbps 5GBase-T";
			*phy_t = MV88X3310_register(priv);
		} else if (priv->deviceId == 0x4010) {
			s = "Dummy CX4";
			*phy_t = CX4_register(priv);
		} else {
			s = "";
			dev_err(&priv->pdev->dev,
				"Unsupported device id/phy id 0x%x/0x%x !\n",
				priv->pdev->device, i);
		}
		break;

#endif

#ifdef PHY_TLK10232
	case 0x40005100:
		s = "TLK10232 10Gbps SFP+";
		*phy_t = TLK10232_register(priv);
		break;
#endif

#ifdef PHY_AQR105
	case 0x03A1B462:	/*AQR105 B0 */
	case 0x03A1B463:	/*AQR105 B1 */
	case 0x03A1B4A3:	/*AQR105 B1 */

		s = "AQR105 10Gbps 10GBase-T";
		*phy_t = AQR105_register(priv);
		break;
#endif

	default:
		*phy_t = PHY_TYPE_CX4;
		s = "Native 10Gbps CX4";
		*phy_t = CX4_register(priv);
		break;

	}

	priv->isr_mask |= IR_TMR1;
	setMDIOSpeed(priv, priv->phy_ops.mdio_speed);
	dev_info(&priv->pdev->dev, "PHY detected on port %u ID=%X - %s\n",
		 *port_t, i, s);

	return (PHY_TYPE_NA == *phy_t) ? -1 : 0;

}

static int __init bdx_mdio_reset(struct bdx_priv *priv, int port,
				 unsigned short phy)
{
	void __iomem *regs = priv->pBdxRegs;
	int port_t = ++port;
	unsigned short phy_t = phy;

	priv->phy_mdio_port = 0xFF;
	if (-1 == bdx_mdio_phy_search(priv, regs, &port_t, &phy_t)) {
		return -1;
	}
	if (phy != phy_t) {
		dev_err(&priv->pdev->dev, "PHY type by svid %u found %u\n", phy,
			phy_t);
		phy = phy_t;
	}
	port = port_t;
	priv->phy_mdio_port = port;
	priv->phy_type = phy;

	return priv->phy_ops.mdio_reset(priv, port, phy);

}

/*************************************************************************
 *    Print Info                             *
 *************************************************************************/

static void print_hw_id(struct pci_dev *pdev)
{
	struct pci_nic *nic = pci_get_drvdata(pdev);
	u16 pci_link_status = 0;
	u16 pci_ctrl = 0;

	pci_read_config_word(pdev, PCI_LINK_STATUS_REG, &pci_link_status);
	pci_read_config_word(pdev, PCI_DEV_CTRL_REG, &pci_ctrl);

	dev_info(&pdev->dev,
		 "srom 0x%x HWver %d build %u lane# %d max_pl 0x%x mrrs 0x%x\n",
		 readl(nic->regs + SROM_VER),
		 readl(nic->regs + FPGA_VER) & 0xFFFF,
		 readl(nic->regs + FPGA_SEED),
		 GET_LINK_STATUS_LANES(pci_link_status),
		 GET_DEV_CTRL_MAXPL(pci_ctrl), GET_DEV_CTRL_MRRS(pci_ctrl));
}

static void print_fw_id(struct bdx_priv *priv)
{
	netdev_info(priv->ndev, "fw 0x%x\n", readl(priv->nic->regs + FW_VER));
}

static void print_eth_id(struct net_device *ndev)
{
	netdev_info(ndev, "Port %c\n", (ndev->if_port == 0) ? 'A' : 'B');
}

/*************************************************************************
 *    Code                               *
 *************************************************************************/
#define bdx_enable_interrupts(priv)  do { WRITE_REG(priv, regIMR, priv->isr_mask); } while (0)
#define bdx_disable_interrupts(priv) do { WRITE_REG(priv, regIMR, 0); } while (0)

/* bdx_fifo_init
 * Create TX/RX descriptor fifo for host-NIC communication. 1K extra space is
 * allocated at the end of the fifo to simplify processing of descriptors that
 * wraps around fifo's end.
 *
 * @priv     - NIC private structure
 * @f        - Fifo to initialize
 * @fsz_type - Fifo size type: 0-4KB, 1-8KB, 2-16KB, 3-32KB
 * @reg_XXX  - Offsets of registers relative to base address
 *
 * Returns 0 on success, negative value on failure
 *
 */

static int
bdx_fifo_init(struct bdx_priv *priv, struct fifo *f, int fsz_type,
	      u16 reg_CFG0, u16 reg_CFG1, u16 reg_RPTR, u16 reg_WPTR)
{
	u16 memsz = FIFO_SIZE * (1 << fsz_type);
	memset(f, 0, sizeof(struct fifo));
	/* dma_alloc_coherent gives us 4k-aligned memory */
	if (f->va == NULL) {
		f->va = dma_alloc_coherent(&priv->pdev->dev,
					   memsz + FIFO_EXTRA_SPACE, &f->da,
					   GFP_ATOMIC);
		if (!f->va) {
			netdev_err(priv->ndev, "dma_alloc_coherent failed\n");
			return -ENOMEM;
		}
	}
	f->reg_CFG0 = reg_CFG0;
	f->reg_CFG1 = reg_CFG1;
	f->reg_RPTR = reg_RPTR;
	f->reg_WPTR = reg_WPTR;
	f->rptr = 0;
	f->wptr = 0;
	f->memsz = memsz;
	f->size_mask = memsz - 1;
	WRITE_REG(priv, reg_CFG0, (u32) ((f->da & TX_RX_CFG0_BASE) | fsz_type));
	WRITE_REG(priv, reg_CFG1, H32_64(f->da));

	return 0;
}

/* bdx_fifo_free - Free all resources used by fifo
 * @priv     - Nic private structure
 * @f        - Fifo to release
 */
static void bdx_fifo_free(struct bdx_priv *priv, struct fifo *f)
{

	if (f->va) {
		dma_free_coherent(&priv->pdev->dev,
				  f->memsz + FIFO_EXTRA_SPACE, f->va,
				  (enum dma_data_direction)f->da);
		f->va = NULL;
	}

}

int bdx_speed_set(struct bdx_priv *priv, u32 speed)
{
	int i;
	u32 val;

	dev_dbg(&priv->pdev->dev, "speed %d\n", speed);

	switch (speed) {
	case SPEED_10000:
	case SPEED_5000:
	case SPEED_2500:
	case SPEED_1000X:
	case SPEED_100X:
		dev_dbg(&priv->pdev->dev, "link_speed %d\n", speed);

		WRITE_REG(priv, 0x1010, 0x217);	/*ETHSD.REFCLK_CONF  */
		WRITE_REG(priv, 0x104c, 0x4c);	/*ETHSD.L0_RX_PCNT  */
		WRITE_REG(priv, 0x1050, 0x4c);	/*ETHSD.L1_RX_PCNT  */
		WRITE_REG(priv, 0x1054, 0x4c);	/*ETHSD.L2_RX_PCNT  */
		WRITE_REG(priv, 0x1058, 0x4c);	/*ETHSD.L3_RX_PCNT  */
		WRITE_REG(priv, 0x102c, 0x434);	/*ETHSD.L0_TX_PCNT  */
		WRITE_REG(priv, 0x1030, 0x434);	/*ETHSD.L1_TX_PCNT  */
		WRITE_REG(priv, 0x1034, 0x434);	/*ETHSD.L2_TX_PCNT  */
		WRITE_REG(priv, 0x1038, 0x434);	/*ETHSD.L3_TX_PCNT  */
		WRITE_REG(priv, 0x6300, 0x0400);	/*MAC.PCS_CTRL */

		WRITE_REG(priv, 0x1018, 0x00);	/*Mike2 */
		udelay(5);
		WRITE_REG(priv, 0x1018, 0x04);	/*Mike2 */
		udelay(5);
		WRITE_REG(priv, 0x1018, 0x06);	/*Mike2 */
		udelay(5);
		/*MikeFix1 */
		/*L0: 0x103c , L1: 0x1040 , L2: 0x1044 , L3: 0x1048 =0x81644 */
		WRITE_REG(priv, 0x103c, 0x81644);	/*ETHSD.L0_TX_DCNT  */
		WRITE_REG(priv, 0x1040, 0x81644);	/*ETHSD.L1_TX_DCNT  */
		WRITE_REG(priv, 0x1044, 0x81644);	/*ETHSD.L2_TX_DCNT  */
		WRITE_REG(priv, 0x1048, 0x81644);	/*ETHSD.L3_TX_DCNT  */
		WRITE_REG(priv, 0x1014, 0x043);	/*ETHSD.INIT_STAT */
		for (i = 1000; i; i--) {
			udelay(50);
			val = READ_REG(priv, 0x1014);	/*ETHSD.INIT_STAT */
			if (val & (1 << 9)) {
				WRITE_REG(priv, 0x1014, 0x3);	/*ETHSD.INIT_STAT */
				val = READ_REG(priv, 0x1014);	/*ETHSD.INIT_STAT */

				break;
			}
		}
		if (0 == i) {
			dev_err(&priv->pdev->dev, "MAC init timeout!\n");
		}

		WRITE_REG(priv, 0x6350, 0x0);	/*MAC.PCS_IF_MODE */
		WRITE_REG(priv, regCTRLST, 0xC13);	/*0x93//0x13 */
		WRITE_REG(priv, 0x111c, 0x7ff);	/*MAC.MAC_RST_CNT */
		for (i = 40; i--;) {
			udelay(50);
		}
		WRITE_REG(priv, 0x111c, 0x0);	/*MAC.MAC_RST_CNT */

		break;

	case SPEED_1000:
	case SPEED_100:

		WRITE_REG(priv, 0x1010, 0x613);	/*ETHSD.REFCLK_CONF  */
		WRITE_REG(priv, 0x104c, 0x4d);	/*ETHSD.L0_RX_PCNT  */
		WRITE_REG(priv, 0x1050, 0x0);	/*ETHSD.L1_RX_PCNT  */
		WRITE_REG(priv, 0x1054, 0x0);	/*ETHSD.L2_RX_PCNT  */
		WRITE_REG(priv, 0x1058, 0x0);	/*ETHSD.L3_RX_PCNT  */
		WRITE_REG(priv, 0x102c, 0x35);	/*ETHSD.L0_TX_PCNT  */
		WRITE_REG(priv, 0x1030, 0x0);	/*ETHSD.L1_TX_PCNT  */
		WRITE_REG(priv, 0x1034, 0x0);	/*ETHSD.L2_TX_PCNT  */
		WRITE_REG(priv, 0x1038, 0x0);	/*ETHSD.L3_TX_PCNT  */
		WRITE_REG(priv, 0x6300, 0x01140);	/*MAC.PCS_CTRL */

		WRITE_REG(priv, 0x1014, 0x043);	/*ETHSD.INIT_STAT */
		for (i = 1000; i; i--) {
			udelay(50);
			val = READ_REG(priv, 0x1014);	/*ETHSD.INIT_STAT */
			if (val & (1 << 9)) {
				WRITE_REG(priv, 0x1014, 0x3);	/*ETHSD.INIT_STAT */
				val = READ_REG(priv, 0x1014);	/*ETHSD.INIT_STAT */

				break;
			}
		}
		if (0 == i) {
			dev_err(&priv->pdev->dev, "MAC init timeout!\n");
		}
		WRITE_REG(priv, 0x6350, 0x2b);	/*MAC.PCS_IF_MODE 1g */
		WRITE_REG(priv, 0x6310, 0x9801);	/*MAC.PCS_DEV_AB */

		WRITE_REG(priv, 0x6314, 0x1);	/*MAC.PCS_PART_AB */
		WRITE_REG(priv, 0x6348, 0xc8);	/*MAC.PCS_LINK_LO */
		WRITE_REG(priv, 0x634c, 0xc8);	/*MAC.PCS_LINK_HI */
		udelay(50);
		WRITE_REG(priv, regCTRLST, 0xC13);	/*0x93//0x13 */
		WRITE_REG(priv, 0x111c, 0x7ff);	/*MAC.MAC_RST_CNT */
		for (i = 40; i--;) {
			udelay(50);
		}
		WRITE_REG(priv, 0x111c, 0x0);	/*MAC.MAC_RST_CNT */
		WRITE_REG(priv, 0x6300, 0x1140);	/*MAC.PCS_CTRL */

		break;

	case 0:		/* Link down */

		WRITE_REG(priv, 0x104c, 0x0);	/*ETHSD.L0_RX_PCNT  */
		WRITE_REG(priv, 0x1050, 0x0);	/*ETHSD.L1_RX_PCNT  */
		WRITE_REG(priv, 0x1054, 0x0);	/*ETHSD.L2_RX_PCNT  */
		WRITE_REG(priv, 0x1058, 0x0);	/*ETHSD.L3_RX_PCNT  */
		WRITE_REG(priv, 0x102c, 0x0);	/*ETHSD.L0_TX_PCNT  */
		WRITE_REG(priv, 0x1030, 0x0);	/*ETHSD.L1_TX_PCNT  */
		WRITE_REG(priv, 0x1034, 0x0);	/*ETHSD.L2_TX_PCNT  */
		WRITE_REG(priv, 0x1038, 0x0);	/*ETHSD.L3_TX_PCNT  */

		WRITE_REG(priv, regCTRLST, 0x800);
		WRITE_REG(priv, 0x111c, 0x7ff);	/*MAC.MAC_RST_CNT */
		for (i = 40; i--;) {
			udelay(50);
		}
		WRITE_REG(priv, 0x111c, 0x0);	/*MAC.MAC_RST_CNT */
		break;
	default:
		dev_err(&priv->pdev->dev,
			"Link speed was not identified yet (%d)\n", speed);
		speed = 0;
		break;
	}

	return speed;

}

void bdx_speed_changed(struct bdx_priv *priv, u32 speed)
{

	dev_dbg(&priv->pdev->dev, "speed %d\n", speed);
	speed = bdx_speed_set(priv, speed);
	dev_dbg(&priv->pdev->dev, "link_speed %d speed %d\n", priv->link_speed,
		speed);
	if (priv->link_speed != speed) {
		priv->link_speed = speed;
		dev_dbg(&priv->pdev->dev, "Speed changed %d\n",
			priv->link_speed);
	}

}

/*
 * bdx_link_changed - Notify the OS about hw link state.
 *
 * @bdx_priv - HW adapter structure
 */

static void bdx_link_changed(struct bdx_priv *priv)
{
	u32 link = priv->phy_ops.link_changed(priv);;

	if (!link) {
		if (netif_carrier_ok(priv->ndev)) {
			netif_stop_queue(priv->ndev);
			netif_carrier_off(priv->ndev);
			netdev_err(priv->ndev, "Link Down\n");
#ifdef _EEE_
			if (priv->phy_ops.reset_eee != NULL) {
				priv->phy_ops.reset_eee(priv);
			}
#endif
		}
	} else {
		if (!netif_carrier_ok(priv->ndev)) {
			netif_wake_queue(priv->ndev);
			netif_carrier_on(priv->ndev);
			netdev_info(priv->ndev, "Link Up %s\n",
				    (priv->link_speed ==
				     SPEED_10000) ? "10G" : (priv->link_speed ==
							     SPEED_5000) ? "5G"
				    : (priv->link_speed ==
				       SPEED_2500) ? "2.5G" : (priv->link_speed
							       == SPEED_1000)
				    || (priv->link_speed ==
					SPEED_1000X) ? "1G" : (priv->link_speed
							       == SPEED_100)
				    || (priv->link_speed ==
					SPEED_100X) ? "100M" : " ");
		}
	}

}

static inline void bdx_isr_extra(struct bdx_priv *priv, u32 isr)
{
	if (isr & (IR_LNKCHG0 | IR_LNKCHG1 | IR_TMR0)) {
		netdev_dbg(priv->ndev, "isr = 0x%x\n", isr);
		bdx_link_changed(priv);
	}
#if 0
	if (isr & IR_RX_FREE_0) {
		pr_debug("RX_FREE_0\n");
	}
	if (isr & IR_PCIE_LINK)
		pr_err("%s PCI-E Link Fault\n", priv->ndev->name);

	if (isr & IR_PCIE_TOUT)
		pr_err("%s PCI-E Time Out\n", priv->ndev->name);
#endif
}

/* bdx_isr - Interrupt Service Routine for Bordeaux NIC
 * @irq    - Interrupt number
 * @ndev   - Network device
 * @regs   - CPU registers
 *
 * Return IRQ_NONE if it was not our interrupt, IRQ_HANDLED - otherwise
 *
 * Read the ISR register to know interrupt triggers and process them one by
 * one.
 *
 * Interrupt triggers are:
 *    RX_DESC - A new packet has arrived and RXD fifo holds its descriptor
 *    RX_FREE - The number of free Rx buffers in RXF fifo gets low
 *    TX_FREE - A packet was transmitted and RXF fifo holds its descriptor
 */
static irqreturn_t bdx_isr_napi(int irq, void *dev)
{
	struct net_device *ndev = dev;
	struct bdx_priv *priv = netdev_priv(ndev);
	u32 isr;

	isr = READ_REG(priv, regISR_MSK0);

	if (unlikely(!isr)) {
		bdx_enable_interrupts(priv);
		return IRQ_NONE;	/* Not our interrupt */
	}

	if (isr & IR_EXTRA)
		bdx_isr_extra(priv, isr);

	if (isr & (IR_RX_DESC_0 | IR_TX_FREE_0 | IR_TMR1)) {
		if (likely(LUXOR__SCHEDULE_PREP(&priv->napi, ndev))) {
			LUXOR__SCHEDULE(&priv->napi, ndev);
			return IRQ_HANDLED;
		} else {
			/*
			 * NOTE: We get here if an interrupt has slept into
			 *       the small time window between these lines in
			 *       bdx_poll:
			 *   bdx_enable_interrupts(priv);
			 *   return 0;
			 *
			 *   Currently interrupts are disabled (since we
			 *   read the ISR register) and we have failed to
			 *   register the next poll. So we read the regs to
			 *   trigger the chip and allow further interrupts.
			 */
			READ_REG(priv, regTXF_WPTR_0);
			READ_REG(priv, regRXD_WPTR_0);
		}
	}

	bdx_enable_interrupts(priv);
	return IRQ_HANDLED;

}

static int bdx_poll(struct napi_struct *napi, int budget)
{
	struct bdx_priv *priv = container_of(napi, struct bdx_priv, napi);
	int work_done;

	if (!priv->bDeviceRemoved) {
		bdx_tx_cleanup(priv);

		work_done = bdx_rx_receive(priv, &priv->rxd_fifo0, budget);
		if (work_done < budget) {
			napi_complete(napi);
			bdx_enable_interrupts(priv);
		}
	} else {
		work_done = budget;
	}

	return work_done;
}

/* bdx_fw_load - Load the firmware to the NIC
 * @priv       - NIC private structure
 *
 * The firmware is loaded via TXD fifo, which needs be initialized first.
 * The firmware needs to be loaded once per NIC and not per PCI device
 * provided by NIC (a NIC can have multiple devices). So all the drivers use
 * semaphore register to load the FW only once.
 */

static int __init bdx_fw_load(struct bdx_priv *priv)
{
	int master, i;
	int rVal = 0;

	master = READ_REG(priv, regINIT_SEMAPHORE);
	if (!READ_REG(priv, regINIT_STATUS) && master) {
		netdev_dbg(priv->ndev, "Loading FW...\n");
		bdx_tx_push_desc_safe(priv, s_firmLoad, sizeof(s_firmLoad));
		mdelay(100);
	}
	for (i = 0; i < 200; i++) {
		if (READ_REG(priv, regINIT_STATUS))
			break;
		mdelay(2);
	}
	if (master)
		WRITE_REG(priv, regINIT_SEMAPHORE, 1);

	if (i == 200) {
		netdev_err(priv->ndev, "firmware loading failed\n");
		netdev_dbg(priv->ndev,
			   "VPC = 0x%x VIC = 0x%x INIT_STATUS = 0x%x i =%d\n",
			   READ_REG(priv, regVPC), READ_REG(priv, regVIC),
			   READ_REG(priv, regINIT_STATUS), i);
		rVal = -EIO;
	} else {
		netdev_dbg(priv->ndev, "firmware loading success\n");
	}
	print_fw_id(priv);
	return rVal;

}

static void bdx_restore_mac(struct net_device *ndev, struct bdx_priv *priv)
{
	u32 val;

	netdev_dbg(priv->ndev, "mac0 =%x mac1 =%x mac2 =%x\n",
		   READ_REG(priv, regUNC_MAC0_A),
		   READ_REG(priv, regUNC_MAC1_A), READ_REG(priv,
							   regUNC_MAC2_A));

	val = (ndev->dev_addr[0] << 8) | (ndev->dev_addr[1]);
	WRITE_REG(priv, regUNC_MAC2_A, val);
	val = (ndev->dev_addr[2] << 8) | (ndev->dev_addr[3]);
	WRITE_REG(priv, regUNC_MAC1_A, val);
	val = (ndev->dev_addr[4] << 8) | (ndev->dev_addr[5]);
	WRITE_REG(priv, regUNC_MAC0_A, val);

	/* More then IP MAC address */
	WRITE_REG(priv, regMAC_ADDR_0,
		  (ndev->dev_addr[3] << 24) | (ndev->dev_addr[2] << 16) |
		  (ndev->dev_addr[1]
		   << 8) | (ndev->dev_addr[0]));
	WRITE_REG(priv, regMAC_ADDR_1,
		  (ndev->dev_addr[5] << 8) | (ndev->dev_addr[4]));

	netdev_dbg(priv->ndev, "mac0 =%x mac1 =%x mac2 =%x\n",
		   READ_REG(priv, regUNC_MAC0_A),
		   READ_REG(priv, regUNC_MAC1_A), READ_REG(priv,
							   regUNC_MAC2_A));

}

static void bdx_CX4_hw_start(struct bdx_priv *priv)
{
	int i;
	u32 val;

	WRITE_REG(priv, 0x1010, 0x217);	/*ETHSD.REFCLK_CONF  */
	WRITE_REG(priv, 0x104c, 0x4c);	/*ETHSD.L0_RX_PCNT  */
	WRITE_REG(priv, 0x1050, 0x4c);	/*ETHSD.L1_RX_PCNT  */
	WRITE_REG(priv, 0x1054, 0x4c);	/*ETHSD.L2_RX_PCNT  */
	WRITE_REG(priv, 0x1058, 0x4c);	/*ETHSD.L3_RX_PCNT  */
	WRITE_REG(priv, 0x102c, 0x434);	/*ETHSD.L0_TX_PCNT  */
	WRITE_REG(priv, 0x1030, 0x434);	/*ETHSD.L1_TX_PCNT  */
	WRITE_REG(priv, 0x1034, 0x434);	/*ETHSD.L2_TX_PCNT  */
	WRITE_REG(priv, 0x1038, 0x434);	/*ETHSD.L3_TX_PCNT  */
	WRITE_REG(priv, 0x6300, 0x0400);	/*MAC.PCS_CTRL */

	WRITE_REG(priv, 0x1018, 0x00);	/*Mike2 */
	udelay(5);
	WRITE_REG(priv, 0x1018, 0x04);	/*Mike2 */
	udelay(5);
	WRITE_REG(priv, 0x1018, 0x06);	/*Mike2 */
	udelay(5);
	/*MikeFix1 */
	/*L0: 0x103c , L1: 0x1040 , L2: 0x1044 , L3: 0x1048 =0x81644 */
	WRITE_REG(priv, 0x103c, 0x81644);	/*ETHSD.L0_TX_DCNT  */
	WRITE_REG(priv, 0x1040, 0x81644);	/*ETHSD.L1_TX_DCNT  */
	WRITE_REG(priv, 0x1044, 0x81644);	/*ETHSD.L2_TX_DCNT  */
	WRITE_REG(priv, 0x1048, 0x81644);	/*ETHSD.L3_TX_DCNT  */
	WRITE_REG(priv, 0x1014, 0x043);	/*ETHSD.INIT_STAT */
	for (i = 1000; i; i--) {
		udelay(50);
		val = READ_REG(priv, 0x1014);	/*ETHSD.INIT_STAT */
		if (val & (1 << 9)) {
			WRITE_REG(priv, 0x1014, 0x3);	/*ETHSD.INIT_STAT */
			val = READ_REG(priv, 0x1014);	/*ETHSD.INIT_STAT */

			break;
		}
	}
	if (0 == i) {
		netdev_err(priv->ndev, "MAC init timeout!\n");
	}
	WRITE_REG(priv, 0x6350, 0x0);	/*MAC.PCS_IF_MODE */
	WRITE_REG(priv, regCTRLST, 0xC13);	/*0x93//0x13 */
	WRITE_REG(priv, 0x111c, 0x7ff);	/*MAC.MAC_RST_CNT */
	/* CX4 */
	WRITE_REG(priv, regFRM_LENGTH, 0X3FE0);
	WRITE_REG(priv, regRX_FIFO_SECTION, 0x10);
	WRITE_REG(priv, regTX_FIFO_SECTION, 0xE00010);

	for (i = 40; i--;) {
		udelay(50);
	}
	WRITE_REG(priv, 0x111c, 0x0);	/*MAC.MAC_RST_CNT */

}

/* bdx_hw_start - Initialize registers and starts HW's Rx and Tx engines
 * @priv    - NIC private structure
 */
static int bdx_hw_start(struct bdx_priv *priv)
{

	netdev_dbg(priv->ndev, "********** bdx_hw_start() ************\n");
	priv->link_speed = 0;	/* -1 */
	if (priv->phy_type == PHY_TYPE_CX4) {
		bdx_CX4_hw_start(priv);
	} else {
		netdev_dbg(priv->ndev,
			   "********** bdx_hw_start() NOT CX4 ************\n");
		WRITE_REG(priv, regFRM_LENGTH, 0X3FE0);
		WRITE_REG(priv, regGMAC_RXF_A, 0X10fd);
		/*MikeFix1 */
		/*L0: 0x103c , L1: 0x1040 , L2: 0x1044 , L3: 0x1048 =0x81644 */
		WRITE_REG(priv, 0x103c, 0x81644);	/*ETHSD.L0_TX_DCNT  */
		WRITE_REG(priv, 0x1040, 0x81644);	/*ETHSD.L1_TX_DCNT  */
		WRITE_REG(priv, 0x1044, 0x81644);	/*ETHSD.L2_TX_DCNT  */
		WRITE_REG(priv, 0x1048, 0x81644);	/*ETHSD.L3_TX_DCNT  */
		WRITE_REG(priv, regRX_FIFO_SECTION, 0x10);
		WRITE_REG(priv, regTX_FIFO_SECTION, 0xE00010);
		WRITE_REG(priv, regRX_FULLNESS, 0);
		WRITE_REG(priv, regTX_FULLNESS, 0);
	}
	WRITE_REG(priv, regVGLB, 0);
	WRITE_REG(priv, regMAX_FRAME_A,
		  priv->rxf_fifo0.m.pktsz & MAX_FRAME_AB_VAL);

	netdev_dbg(priv->ndev, "RDINTCM =%08x\n", priv->rdintcm);	/*NOTE: test script uses this */
	WRITE_REG(priv, regRDINTCM0, priv->rdintcm);
	WRITE_REG(priv, regRDINTCM2, 0);

	netdev_dbg(priv->ndev, "TDINTCM =%08x\n", priv->tdintcm);	/*NOTE: test script uses this */
	WRITE_REG(priv, regTDINTCM0, priv->tdintcm);	/* old val = 0x300064 */

	/* Enable timer interrupt once in 2 secs. */

	bdx_restore_mac(priv->ndev, priv);
	/* Pause frame */
	WRITE_REG(priv, 0x12E0, 0x28);
	WRITE_REG(priv, regPAUSE_QUANT, 0xFFFF);
	WRITE_REG(priv, 0x6064, 0xF);

	WRITE_REG(priv, regGMAC_RXF_A,
		  GMAC_RX_FILTER_OSEN | GMAC_RX_FILTER_TXFC | GMAC_RX_FILTER_AM
		  | GMAC_RX_FILTER_AB);

	bdx_link_changed(priv);
	bdx_enable_interrupts(priv);
	LUXOR__POLL_ENABLE(priv->ndev);
	priv->state &= ~BDX_STATE_HW_STOPPED;

	return 0;

}

static void bdx_hw_stop(struct bdx_priv *priv)
{

	if ((priv->state & BDX_STATE_HW_STOPPED) == 0) {
		priv->state |= BDX_STATE_HW_STOPPED;
		bdx_disable_interrupts(priv);
		LUXOR__POLL_DISABLE(priv->ndev);
		netif_carrier_off(priv->ndev);
		netif_stop_queue(priv->ndev);
	}

}

static int bdx_hw_reset_direct(struct pci_dev *pdev, void __iomem *regs)
{
	u32 val, i;

	/* Reset sequences: read, write 1, read, write 0 */
	val = readl(regs + regCLKPLL);
	writel((val | CLKPLL_SFTRST) + 0x8, regs + regCLKPLL);
	udelay(50);
	val = readl(regs + regCLKPLL);
	writel(val & ~CLKPLL_SFTRST, regs + regCLKPLL);

	/* Check that the PLLs are locked and reset ended */
	for (i = 0; i < 70; i++, mdelay(10))
		if ((readl(regs + regCLKPLL) & CLKPLL_LKD) == CLKPLL_LKD) {
			udelay(50);
			/* do any PCI-E read transaction */
			readl(regs + regRXD_CFG0_0);
			return 0;
		}
	dev_err(&pdev->dev, "HW reset failed\n");

	return 1;		/* failure */

}

static int bdx_hw_reset(struct bdx_priv *priv)
{
	u32 val, i;

	if (priv->port == 0) {
		/* Reset sequences: read, write 1, read, write 0 */
		val = READ_REG(priv, regCLKPLL);
		WRITE_REG(priv, regCLKPLL, (val | CLKPLL_SFTRST) + 0x8);
		udelay(50);
		val = READ_REG(priv, regCLKPLL);
		WRITE_REG(priv, regCLKPLL, val & ~CLKPLL_SFTRST);
	}
	/* Check that the PLLs are locked and reset ended */
	for (i = 0; i < 70; i++, mdelay(10)) {
		if ((READ_REG(priv, regCLKPLL) & CLKPLL_LKD) == CLKPLL_LKD) {
			udelay(50);
			/* Do any PCI-E read transaction */
			READ_REG(priv, regRXD_CFG0_0);
			return 0;

		}
	}
	netdev_err(priv->ndev, "HW reset failed\n");

	return 1;		/* Failure */

}

static int bdx_sw_reset(struct bdx_priv *priv)
{
	int i;

	/* 1. load MAC (obsolete) */
	/* 2. disable Rx (and Tx) */
	WRITE_REG(priv, regGMAC_RXF_A, 0);
	mdelay(100);
	/* 3. Disable port */
	WRITE_REG(priv, regDIS_PORT, 1);
	/* 4. Disable queue */
	WRITE_REG(priv, regDIS_QU, 1);
	/* 5. Wait until hw is disabled */
	for (i = 0; i < 50; i++) {
		if (READ_REG(priv, regRST_PORT) & 1)
			break;
		mdelay(10);
	}
	if (i == 50) {
		netdev_err(priv->ndev, "SW reset timeout. continuing anyway\n");
	}
	/* 6. Disable interrupts */
	WRITE_REG(priv, regRDINTCM0, 0);
	WRITE_REG(priv, regTDINTCM0, 0);
	WRITE_REG(priv, regIMR, 0);
	READ_REG(priv, regISR);

	/* 7. Reset queue */
	WRITE_REG(priv, regRST_QU, 1);
	/* 8. Reset port */
	WRITE_REG(priv, regRST_PORT, 1);
	/* 9. Zero all read and write pointers */
	for (i = regTXD_WPTR_0; i <= regTXF_RPTR_3; i += 0x10)
		WRITE_REG(priv, i, 0);
	/* 10. Unset port disable */
	WRITE_REG(priv, regDIS_PORT, 0);
	/* 11. Unset queue disable */
	WRITE_REG(priv, regDIS_QU, 0);
	/* 12. Unset queue reset */
	WRITE_REG(priv, regRST_QU, 0);
	/* 13. Unset port reset */
	WRITE_REG(priv, regRST_PORT, 0);
	/* 14. Enable Rx */
	/* Skipped. will be done later */
	return 0;

}

/* bdx_reset - Perform the right type of reset depending on hw type */
static int bdx_reset(struct bdx_priv *priv)
{

	return bdx_hw_reset(priv);

}

static int bdx_start(struct bdx_priv *priv, int bLoadFw)
{
	int rc = 0;

	if ((priv->state & BDX_STATE_STARTED) == 0) {
		priv->state |= BDX_STATE_STARTED;
		do {
			rc = -1;
			if (bdx_tx_init(priv)) {
				break;
			}
			if (bdx_rx_init(priv)) {
				break;
			}
			if (bdx_rx_alloc_pages(priv)) {
				break;
			}
			bdx_rx_alloc_buffers(priv);
			if (request_irq
			    (priv->pdev->irq, &bdx_isr_napi, BDX_IRQ_TYPE,
			     priv->ndev->name, priv->ndev)) {
				break;
			}
			if (bLoadFw && bdx_fw_load(priv)) {
				break;
			}
			bdx_init_rss(priv);
			rc = 0;
		} while (0);
	}
	if (rc == 0) {
		if (priv->state & BDX_STATE_OPEN) {
			rc = bdx_hw_start(priv);
		}
	}
	return rc;

}

static void bdx_stop(struct bdx_priv *priv)
{
	if (priv->state & BDX_STATE_STARTED) {
		priv->state &= ~BDX_STATE_STARTED;
		bdx_hw_stop(priv);
		free_irq(priv->pdev->irq, priv->ndev);
		pci_free_irq_vectors(priv->pdev);
		bdx_sw_reset(priv);
		bdx_rx_free(priv);
		bdx_rx_free_pages(priv);
		bdx_tx_free(priv);
	}

}

/**
 * bdx_close - Disables a network interface
 *
 * @netdev: network interface device structure
 *
 * Returns 0, this is not allowed to fail
 *
 * This API is called when an interface is deactivated by the OS.  The
 * hardware is still under the drivers control, but needs to be disabled.  A
 * global MAC reset is issued to stop the hardware, and all transmit and
 * receive resources are freed.
 **/
static int bdx_close(struct net_device *ndev)
{
	struct bdx_priv *priv;

	priv = netdev_priv(ndev);
	bdx_stop(priv);
	LUXOR__NAPI_DISABLE(&priv->napi);
	priv->state &= ~BDX_STATE_OPEN;
	return 0;

}

/**
 * bdx_open - This API is called when a network interface is made active.
 *
 * @netdev: network interface device structure
 *
 * Returns 0 on success, negative value on failure
 *
 * This API is is called when a network interface is made active by the system
 * (IFF_UP).  At this point all resources needed for transmit and receive
 * operations are allocated, the interrupt handler is registered with the OS,
 * the watchdog timer is started, and the stack is notified that the interface
 * is ready.
 **/
static int bdx_open(struct net_device *ndev)
{
	struct bdx_priv *priv;
	int rc;

	priv = netdev_priv(ndev);
	priv->state |= BDX_STATE_OPEN;
	bdx_sw_reset(priv);
	if (netif_running(ndev)) {
		netif_stop_queue(priv->ndev);
	}
	if ((rc = bdx_start(priv, NO_FW_LOAD)) == 0) {
		LUXOR__NAPI_ENABLE(&priv->napi);
	} else {
		bdx_close(ndev);
	}

	return rc;

}

#ifdef __BIG_ENDIAN
static void __init bdx_firmware_endianess(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(s_firmLoad); i++) {
		s_firmLoad[i] = CPU_CHIP_SWAP32(s_firmLoad[i]);
	}
}
#endif

static int bdx_range_check(struct bdx_priv *priv, u32 offset)
{
	return ((offset > (u32) (BDX_REGS_SIZE / priv->nic->port_num)) ?
		-EINVAL : 0);
}

/*
 * __bdx_vlan_rx_vid - Private helper function for adding/killing VLAN vid
 *             by passing VLAN filter table to hardware
 * @ndev - Network device
 * @vid  - VLAN vid
 * @op   - Add or kill operation
 */
static void __bdx_vlan_rx_vid(struct net_device *ndev, uint16_t vid, int enable)
{
	struct bdx_priv *priv = netdev_priv(ndev);
	u32 reg, bit, val;

	netdev_dbg(ndev, "vid =%d value =%d\n", (int)vid, enable);
	if (unlikely(vid >= 4096)) {
		netdev_err(ndev, "invalid VID: %u (> 4096)\n", vid);
		return;
	}
	reg = regVLAN_0 + (vid / 32) * 4;
	bit = 1 << vid % 32;
	val = READ_REG(priv, reg);
	netdev_dbg(ndev, "reg =%x, val =%x, bit =%d\n", reg, val, bit);
	if (enable)
		val |= bit;
	else
		val &= ~bit;
	netdev_dbg(ndev, "new val %x\n", val);
	WRITE_REG(priv, reg, val);

}

/*
 * bdx_vlan_rx_add_vid - A kernel hook for adding VLAN vid to hw filtering
 *           table
 * @ndev - Network device
 * @vid  - Vlan vid to add
 */
#ifdef NETIF_F_HW_VLAN_CTAG_TX
static int bdx_vlan_rx_add_vid(struct net_device *ndev,
			       __always_unused __be16 proto, u16 vid)
#else /* !NETIF_F_HW_VLAN_CTAG_TX */
static int bdx_vlan_rx_add_vid(struct net_device *ndev, u16 vid)
#endif				/* NETIF_F_HW_VLAN_CTAG_TX */
{
	__bdx_vlan_rx_vid(ndev, vid, 1);
	return 0;
}

/*
 * bdx_vlan_rx_kill_vid - A kernel hook for killing VLAN vid in hw filtering
 *            table
 *
 * @ndev - Network device
 * @vid  - Vlan vid to kill
 */
#ifdef NETIF_F_HW_VLAN_CTAG_RX
static int bdx_vlan_rx_kill_vid(struct net_device *ndev,
				__always_unused __be16 proto, u16 vid)
#else /* !NETIF_F_HW_VLAN_CTAG_RX */
static int bdx_vlan_rx_kill_vid(struct net_device *ndev, u16 vid)
#endif				/* NETIF_F_HW_VLAN_CTAG_RX */
{
	__bdx_vlan_rx_vid(ndev, vid, 0);
	return 0;
}

/**
 * bdx_change_mtu - Change the Maximum Transfer Unit
 *
 * @netdev  - Network interface device structure
 * @new_mtu - New value for maximum frame size
 *
 * Returns 0 on success, negative error code on failure
 */
static int bdx_change_mtu(struct net_device *ndev, int new_mtu)
{

	if (new_mtu == ndev->mtu)
		return 0;

	/* enforce minimum frame size */
	if (new_mtu < ETH_ZLEN) {
		netdev_err(ndev, "mtu %d is less then minimal %d\n",
			   new_mtu, ETH_ZLEN);
		return -EINVAL;
	} else if (new_mtu > BDX_MAX_MTU) {
		netdev_err(ndev, "mtu %d is greater then max mtu %d\n",
			   new_mtu, BDX_MAX_MTU);
		return -EINVAL;
	}

	ndev->mtu = new_mtu;

	if (netif_running(ndev)) {
		bdx_close(ndev);
		bdx_open(ndev);
	}
	return 0;
}

static void bdx_setmulti(struct net_device *ndev)
{
	struct bdx_priv *priv = netdev_priv(ndev);

	u32 rxf_val =
	    GMAC_RX_FILTER_AM | GMAC_RX_FILTER_AB | GMAC_RX_FILTER_OSEN |
	    GMAC_RX_FILTER_TXFC;
	int i;

	/* IMF - imperfect (hash) rx multicast filter */
	/* PMF - perfect rx multicast filter */

	/* FIXME: RXE(OFF) */
	if (ndev->flags & IFF_PROMISC) {
		rxf_val |= GMAC_RX_FILTER_PRM;
	} else if (ndev->flags & IFF_ALLMULTI) {
		/* set IMF to accept all multicast frames */
		for (i = 0; i < MAC_MCST_HASH_NUM; i++) {
			WRITE_REG(priv, regRX_MCST_HASH0 + i * 4, ~0);
		}
	} else if (netdev_mc_count(ndev)) {
		u8 hash;
		struct dev_mc_list *mclist;
		u32 reg, val;

		/* Set IMF to deny all multicast frames */
		for (i = 0; i < MAC_MCST_HASH_NUM; i++) {
			WRITE_REG(priv, regRX_MCST_HASH0 + i * 4, 0);
		}
		/* Set PMF to deny all multicast frames */
		for (i = 0; i < MAC_MCST_NUM; i++) {
			WRITE_REG(priv, regRX_MAC_MCST0 + i * 8, 0);
			WRITE_REG(priv, regRX_MAC_MCST1 + i * 8, 0);
		}

		/* Use PMF to accept first MAC_MCST_NUM (15) addresses */
		/* TBD: Sort the addresses and write them in ascending order into
		 *  RX_MAC_MCST regs. we skip this phase now and accept ALL
		 *  multicast frames through IMF. Accept the rest of
		 *      addresses throw IMF.
		 */
		netdev_for_each_mc_addr(mclist, ndev) {
			hash = 0;
			for (i = 0; i < ETH_ALEN; i++) {
				hash ^= mclist->dmi_addr[i];

			}
			reg = regRX_MCST_HASH0 + ((hash >> 5) << 2);
			val = READ_REG(priv, reg);
			val |= (1 << (hash % 32));
			WRITE_REG(priv, reg, val);
		}

	} else {

		rxf_val |= GMAC_RX_FILTER_AB;
	}
	WRITE_REG(priv, regGMAC_RXF_A, rxf_val);
	/* Enable RX */
	/* FIXME: RXE(ON) */

}

static int bdx_set_mac(struct net_device *ndev, void *p)
{
	struct bdx_priv *priv = netdev_priv(ndev);
	struct sockaddr *addr = p;

	/*
	   if (netif_running(dev))
	   return -EBUSY
	 */
	eth_hw_addr_set(ndev, addr->sa_data);
	bdx_restore_mac(ndev, priv);
	return 0;
}

static int bdx_read_mac(struct bdx_priv *priv)
{
	u16 macAddress[3], i;
	u8 addr[ETH_ALEN];

	macAddress[2] = READ_REG(priv, regUNC_MAC0_A);
	macAddress[2] = READ_REG(priv, regUNC_MAC0_A);
	macAddress[1] = READ_REG(priv, regUNC_MAC1_A);
	macAddress[1] = READ_REG(priv, regUNC_MAC1_A);
	macAddress[0] = READ_REG(priv, regUNC_MAC2_A);
	macAddress[0] = READ_REG(priv, regUNC_MAC2_A);
	for (i = 0; i < 3; i++) {
		addr[i * 2 + 1] = macAddress[i];
		addr[i * 2] = macAddress[i] >> 8;
	}
	eth_hw_addr_set(priv->ndev, addr);
	return 0;
}

static u64 bdx_read_l2stat(struct bdx_priv *priv, int reg)
{
	u64 val;

	val = READ_REG(priv, reg);
	val |= ((u64) READ_REG(priv, reg + 8)) << 32;
	return val;
}

/*Do the statistics-update work*/

static void bdx_update_stats(struct bdx_priv *priv)
{
	struct bdx_stats *stats = &priv->hw_stats;
	u64 *stats_vector = (u64 *) stats;
	int i;
	int addr;

	/*Fill HW structure */
	addr = 0x7200;

	/*First 12 statistics - 0x7200 - 0x72B0 */
	for (i = 0; i < 12; i++) {
		stats_vector[i] = bdx_read_l2stat(priv, addr);
		addr += 0x10;
	}
	BDX_ASSERT(addr != 0x72C0);

	/* 0x72C0-0x72E0 RSRV */
	addr = 0x72F0;
	for (; i < 16; i++) {
		stats_vector[i] = bdx_read_l2stat(priv, addr);
		addr += 0x10;
	}
	BDX_ASSERT(addr != 0x7330);

	/* 0x7330-0x7360 RSRV */
	addr = 0x7370;
	for (; i < 19; i++) {
		stats_vector[i] = bdx_read_l2stat(priv, addr);
		addr += 0x10;
	}
	BDX_ASSERT(addr != 0x73A0);

	/* 0x73A0-0x73B0 RSRV */
	addr = 0x73C0;
	for (; i < 23; i++) {
		stats_vector[i] = bdx_read_l2stat(priv, addr);
		addr += 0x10;
	}

	BDX_ASSERT(addr != 0x7400);
	BDX_ASSERT((sizeof(struct bdx_stats) / sizeof(u64)) != i);
}

static struct net_device_stats *bdx_get_stats(struct net_device *ndev)
{
	struct bdx_priv *priv = netdev_priv(ndev);
	struct net_device_stats *net_stat = &priv->net_stats;
	return net_stat;
}

static void print_rxdd(struct rxd_desc *rxdd, u32 rxd_val1, u16 len,
		       u16 rxd_vlan);
static void print_rxfd(struct rxf_desc *rxfd);

/*************************************************************************
 *     Rx DB                                 *
 *************************************************************************/

static void bdx_rxdb_destroy(struct rxdb *db)
{
	if (db->pkt) {
		vfree(db->pkt);
	}
	if (db) {
		vfree(db);
	}
}

static struct rxdb *bdx_rxdb_create(int nelem, u16 pktSize)
{
	struct rxdb *db;
	int i;
	size_t size = sizeof(struct rxdb) + (nelem * sizeof(int)) +
	    (nelem * sizeof(struct rx_map));

	db = vmalloc(size);
	if (likely(db != NULL)) {
		memset(db, 0, size);
		db->stack = (int *)(db + 1);
		db->elems = (void *)(db->stack + nelem);
		db->nelem = nelem;
		db->top = nelem;
		for (i = 0; i < nelem; i++) {
			db->stack[i] = nelem - i - 1;	/* To make the first
							 * alloc close to db
							 * struct */
		}
	}
	if ((db->pkt = vmalloc(pktSize)) == NULL) {
		bdx_rxdb_destroy(db);
		db = NULL;
	}

	return db;
}

static inline int bdx_rxdb_alloc_elem(struct rxdb *db)
{
	BDX_ASSERT(db->top <= 0);
	TN40_ASSERT((db->top > 0), "top %d\n", db->top);
	return db->stack[--(db->top)];
}

static inline void *bdx_rxdb_addr_elem(struct rxdb *db, unsigned n)
{
	BDX_ASSERT((n >= (unsigned)db->nelem));
	TN40_ASSERT((n < (unsigned)db->nelem), "n %d nelem %d\n", n, db->nelem);
	return db->elems + n;
}

static inline int bdx_rxdb_available(struct rxdb *db)
{
	return db->top;
}

static inline void bdx_rxdb_free_elem(struct rxdb *db, unsigned n)
{
	BDX_ASSERT((n >= (unsigned)db->nelem));
	db->stack[(db->top)++] = n;
}

/*************************************************************************
 *     Rx Engine                             *
 *************************************************************************/

static void bdx_rx_vlan(struct bdx_priv *priv, struct sk_buff *skb,
			u32 rxd_val1, u16 rxd_vlan)
{
	if (GET_RXD_VTAG(rxd_val1)) {	/* Vlan case */
		__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
				       le16_to_cpu(GET_RXD_VLAN_TCI(rxd_vlan)));
	}
}

static void dbg_printRxPage(char newLine, struct bdx_page *bdx_page)
{
	if (g_dbg) {
		if (newLine == '\n') {
			printk("\n");
		}
		if (bdx_page) {
			printk(KERN_CONT "bxP %p P %p C %d bxC %d R %d %c\n",
			       bdx_page, bdx_page->page,
			       page_count(bdx_page->page), bdx_page->ref_count,
			       bdx_page->reuse_tries, bdx_page->status);
		} else {
			printk(KERN_CONT "NULL page\n");
		}
	}
}

static void dbg_printRxPageTable(struct bdx_priv *priv)
{
	int j;
	struct bdx_page *bdx_page;
	if (g_dbg && priv->rx_page_table.bdx_pages) {
		printk("rx_page_table\n");
		printk("=============\n");
		printk("nPages %d pSize %d buf size %d bufs in page %d\n",
		       priv->rx_page_table.nPages,
		       priv->rx_page_table.page_size,
		       priv->rx_page_table.buf_size,
		       priv->rx_page_table.nBufInPage);
		printk("nFrees %d max_frees %d\n", priv->rx_page_table.nFrees,
		       priv->rx_page_table.max_frees);
		for (j = 0; j < priv->rx_page_table.nPages; j++) {
			bdx_page = &priv->rx_page_table.bdx_pages[j];
			printk("%3d. ", j);
			dbg_printRxPage(0, bdx_page);
		}
		printk("\n");
	}

}

static inline struct bdx_page *bdx_rx_page(struct rx_map *dm)
{
	return dm->bdx_page;

}

static int bdx_rx_set_page_size(struct bdx_priv *priv, int buf_size,
				int *page_used)
{
	int page_unused = buf_size;
	int min_page_size = PAGE_SIZE * (1 << get_order(buf_size));
	int page_size = 0;
	int psz;

	for (psz = LUXOR__MAX_PAGE_SIZE; psz >= min_page_size; psz >>= 1) {
		int unused = psz - ((psz / buf_size) * buf_size);
		if (unused <= page_unused) {
			page_unused = unused;
			page_size = psz;
		}
		netdev_dbg(priv->ndev, "psz %d page_size %d unused %d \n",
			   psz, page_size, unused);
	}
	*page_used = page_size - page_unused;

	return page_size;

}

static int bdx_rx_get_page_size(struct bdx_priv *priv)
{
	return priv->rx_page_table.page_size;

}

static int bdx_rx_alloc_page(struct bdx_priv *priv, struct bdx_page *bdx_page)
{
	int rVal = -1;

	bdx_page->page =
	    alloc_pages(priv->rx_page_table.gfp_mask,
			priv->rx_page_table.page_order);
	if (likely(bdx_page->page != NULL)) {
		get_page(bdx_page->page);
		bdx_page->ref_count = 1;
		bdx_page->dma =
		    dma_map_page(&priv->pdev->dev, bdx_page->page, 0L,
				 priv->rx_page_table.page_size,
				 DMA_FROM_DEVICE);
		if (!dma_mapping_error(&priv->pdev->dev, bdx_page->dma)) {
			rVal = 0;
		} else {
			__free_pages(bdx_page->page,
				     priv->rx_page_table.page_order);
			netdev_err(priv->ndev, "Failed to map page %p !\n",
				   bdx_page->page);
		}
	}

	return rVal;

}

static int bdx_rx_alloc_pages(struct bdx_priv *priv)
{
	int page_used, nPages, j;
	struct rxf_fifo *f = &priv->rxf_fifo0;
	struct rxdb *db = priv->rxdb0;
	int buf_size = ROUND_UP(f->m.pktsz, SMP_CACHE_BYTES);
	int rVal = -1;

	do {
		priv->rx_page_table.page_size =
		    bdx_rx_set_page_size(priv, buf_size, &page_used);
		nPages = (db->nelem * buf_size + (page_used - 1)) / page_used;
		priv->rx_page_table.gfp_mask = GFP_ATOMIC | __GFP_NOWARN;
		if (priv->rx_page_table.page_size > PAGE_SIZE) {
			priv->rx_page_table.gfp_mask |= __GFP_COMP;
		}
		priv->rx_page_table.page_order =
		    get_order(priv->rx_page_table.page_size);
		netdev_dbg
		    (priv->ndev,
		     "buf_size %d nelem %d page_size %d page_used %d nPages %d\n",
		     buf_size, db->nelem, priv->rx_page_table.page_size,
		     page_used, nPages);
		priv->rx_page_table.bdx_pages =
		    vmalloc(sizeof(struct bdx_page) * nPages);
		if (priv->rx_page_table.bdx_pages == NULL) {
			netdev_err(priv->ndev,
				   "Cannot allocate page table !\n");
			break;
		} else {
			memset(priv->rx_page_table.bdx_pages, 0,
			       sizeof(struct bdx_page) * nPages);
		}
		INIT_LIST_HEAD(&priv->rx_page_table.free_list);
		for (j = 0; j < nPages; j++) {
			struct bdx_page *bdx_page =
			    &priv->rx_page_table.bdx_pages[j];
			bdx_page->page_index = j;
			if (bdx_rx_alloc_page(priv, bdx_page) == 0) {
				list_add_tail(&bdx_page->free,
					      &priv->rx_page_table.free_list);
				bdx_page->status = 'F';
				priv->rx_page_table.nFrees += 1;
			} else {
				netdev_err(priv->ndev,
					   "Allocated only %d pages out of %d\n",
					   j, nPages);
				nPages = j;
				break;
			}
		}
		priv->rx_page_table.nPages = nPages;
		priv->rx_page_table.buf_size = buf_size;
		priv->rx_page_table.nBufInPage =
		    priv->rx_page_table.page_size / buf_size;
		priv->rx_page_table.max_frees = priv->rx_page_table.nPages / 2;
		if (priv->rx_page_table.nPages) {
			rVal = 0;
		}

	} while (0);

	return rVal;

}

static void bdx_rx_free_bdx_page(struct bdx_priv *priv,
				 struct bdx_page *bdx_page)
{

	dma_unmap_page(&priv->pdev->dev, bdx_page->dma,
		       priv->rx_page_table.page_size, DMA_FROM_DEVICE);
	put_page(bdx_page->page);

}

static void bdx_rx_free_page(struct bdx_priv *priv, struct bdx_page *bdx_page)
{
	int j;

	bdx_rx_free_bdx_page(priv, bdx_page);
	for (j = 0; j < bdx_page->ref_count; j++) {
		put_page(bdx_page->page);
	}

}

static void bdx_rx_free_pages(struct bdx_priv *priv)
{
	int j;

	dbg_printRxPageTable(priv);
	if (priv->rx_page_table.bdx_pages != NULL) {
		for (j = 0; j < priv->rx_page_table.nPages; j++) {
			bdx_rx_free_page(priv,
					 &priv->rx_page_table.bdx_pages[j]);
		}
		vfree(priv->rx_page_table.bdx_pages);
		priv->rx_page_table.bdx_pages = NULL;
	}

}

static void bdx_rx_ref_page(struct bdx_page *bdx_page)
{
	get_page(bdx_page->page);
	bdx_page->ref_count += 1;

}

static struct bdx_page *bdx_rx_get_page(struct bdx_priv *priv)
{
	int nLoops = 0;
	struct bdx_page *rPage = NULL, *firstPage;
	struct list_head *pos, *temp;

	if (!list_empty(&priv->rx_page_table.free_list)) {
		list_for_each_safe(pos, temp, &priv->rx_page_table.free_list) {
			if (likely
			    (page_count(((struct bdx_page *)pos)->page) == 2)) {
				rPage = ((struct bdx_page *)pos);
				netdev_dbg(priv->ndev, "nLoops %d ", nLoops);
				dbg_printRxPage(0, rPage);
				list_del(pos);
				rPage->status = ' ';
				priv->rx_page_table.nFrees -= 1;
				bdx_rx_ref_page(rPage);
				break;
			} else {
				((struct bdx_page *)pos)->reuse_tries += 1;
			}
			nLoops += 1;
		}
		if ((rPage == NULL)
		    && (priv->rx_page_table.nFrees >
			priv->rx_page_table.max_frees)) {
			firstPage =
			    list_first_entry(&priv->rx_page_table.free_list,
					     struct bdx_page, free);
			netdev_dbg(priv->ndev,
				   "Replacing - loops %d nFrees %d\n", nLoops,
				   priv->rx_page_table.nFrees);
			dbg_printRxPage('\n', firstPage);
			bdx_rx_free_bdx_page(priv, firstPage);
			if (bdx_rx_alloc_page(priv, firstPage) == 0) {
				rPage = firstPage;
				list_del((struct list_head *)rPage);
				rPage->status = ' ';
				priv->rx_page_table.nFrees -= 1;
				bdx_rx_ref_page(rPage);
			} else {
				netdev_info
				    (priv->ndev,
				     "Page alloc failed nFrees %d max_frees %d order %d\n",
				     priv->rx_page_table.nFrees,
				     priv->rx_page_table.max_frees,
				     priv->rx_page_table.page_order);
			}
		}
	}

	return rPage;

}

static void bdx_rx_reuse_page(struct bdx_priv *priv, struct rx_map *dm)
{
	if (--dm->bdx_page->ref_count == 1) {
		dm->bdx_page->reuse_tries = 0;
		list_add_tail(&dm->bdx_page->free,
			      &priv->rx_page_table.free_list);
		dm->bdx_page->status = 'F';
		priv->rx_page_table.nFrees += 1;
	}

}

static void bdx_rx_put_page(struct bdx_priv *priv, struct rx_map *dm)
{
	/* DO NOTHING */

}

static void bdx_rx_set_dm_page(register struct rx_map *dm,
			       struct bdx_page *bdx_page)
{
	dm->bdx_page = bdx_page;

}

/* bdx_rx_init - Initialize RX all related HW and SW resources
 * @priv       - NIC private structure
 *
 * Returns 0 on success, a negative value on failure
 *
 * bdx_rx_init creates rxf and rxd fifos, updates the relevant HW registers,
 * preallocates skbs for rx. It assumes that Rx is disabled in HW funcs are
 * grouped for better cache usage
 *
 * RxD fifo is smaller then RxF fifo by design. Upon high load, RxD will be
 * filled and packets will be dropped by the NIC without getting into the host
 * or generating interrupts. In this situation the host has no chance of
 * processing all the packets. Dropping packets by the NIC is cheaper, since it
 * takes 0 CPU cycles.
 */
static int bdx_rx_init(struct bdx_priv *priv)
{

	if (bdx_fifo_init(priv, &priv->rxd_fifo0.m, priv->rxd_size,
			  regRXD_CFG0_0, regRXD_CFG1_0,
			  regRXD_RPTR_0, regRXD_WPTR_0))
		goto err_mem;
	if (bdx_fifo_init(priv, &priv->rxf_fifo0.m, priv->rxf_size,
			  regRXF_CFG0_0, regRXF_CFG1_0,
			  regRXF_RPTR_0, regRXF_WPTR_0))
		goto err_mem;
	priv->rxf_fifo0.m.pktsz = priv->ndev->mtu + VLAN_ETH_HLEN;
	if (!
	    (priv->rxdb0 =
	     bdx_rxdb_create(priv->rxf_fifo0.m.memsz / sizeof(struct rxf_desc),
			     priv->rxf_fifo0.m.pktsz)))
		goto err_mem;

	return 0;

err_mem:
	netdev_err(priv->ndev, "Rx init failed\n");
	return -ENOMEM;
}

/* bdx_rx_free_buffers - Free and unmap all the fifo allocated buffers.
 *
 * @priv - NIC private structure
 * @f    - RXF fifo
 */
static void bdx_rx_free_buffers(struct bdx_priv *priv, struct rxdb *db,
				struct rxf_fifo *f)
{
	struct rx_map *dm;
	u16 i;

	netdev_dbg(priv->ndev, "total =%d free =%d busy =%d\n", db->nelem,
		   bdx_rxdb_available(db), db->nelem - bdx_rxdb_available(db));
	while (bdx_rxdb_available(db) > 0) {
		i = bdx_rxdb_alloc_elem(db);
		dm = bdx_rxdb_addr_elem(db, i);
		dm->dma = 0;
	}
	for (i = 0; i < db->nelem; i++) {
		dm = bdx_rxdb_addr_elem(db, i);
		if (dm->dma) {
			if (dm->skb) {
				dma_unmap_single(&priv->pdev->dev, dm->dma,
						 f->m.pktsz, DMA_FROM_DEVICE);
				dev_kfree_skb(dm->skb);
			} else {
				struct bdx_page *bdx_page = bdx_rx_page(dm);
				if (bdx_page) {
					bdx_rx_put_page(priv, dm);
				}
			}
		}
	}

}

/* bdx_rx_free - Release all Rx resources.
 *
 * @priv       - NIC private structure
 *
 * Note: This functuion assumes that Rx is disabled in HW.
 */
static void bdx_rx_free(struct bdx_priv *priv)
{

	if (priv->rxdb0) {
		bdx_rx_free_buffers(priv, priv->rxdb0, &priv->rxf_fifo0);
		bdx_rxdb_destroy(priv->rxdb0);
		priv->rxdb0 = NULL;
	}
	bdx_fifo_free(priv, &priv->rxf_fifo0.m);
	bdx_fifo_free(priv, &priv->rxd_fifo0.m);

}

/* bdx_rx_alloc_buffers - Fill rxf fifo with new skbs.
 *
 * @priv - NIC's private structure
 * @f    - RXF fifo that needs skbs
 *
 * bdx_rx_alloc_buffers allocates skbs, builds rxf descs and pushes them (rxf
 * descr) into the rxf fifo.  Skb's virtual and physical addresses are stored
 * in skb db.
 * To calculate the free space, we uses the cached values of RPTR and WPTR
 * when needed. This function also updates RPTR and WPTR.
 */

/* TBD: Do not update WPTR if no desc were written */

static void _bdx_rx_alloc_buffers(struct bdx_priv *priv)
{
	int dno, delta, idx;
	register struct rxf_desc *rxfd;
	register struct rx_map *dm;
	int page_size;
	struct rxdb *db = priv->rxdb0;
	struct rxf_fifo *f = &priv->rxf_fifo0;
	int nPages = 0;
	struct bdx_page *bdx_page = NULL;
	int buf_size = priv->rx_page_table.buf_size;
	int page_off = -1;
	u64 dma = 0ULL;

	netdev_dbg(priv->ndev, "_bdx_rx_alloc_buffers is at %p\n",
		   _bdx_rx_alloc_buffers);
	dno = bdx_rxdb_available(db) - 1;
	page_size = bdx_rx_get_page_size(priv);
	netdev_dbg(priv->ndev, "dno %d page_size %d buf_size %d\n", dno,
		   page_size, priv->rx_page_table.buf_size);
	while (dno > 0) {

		/*
		 * Note: We allocate large pages (i.e. 64KB) and store
		 *       multiple packet buffers in each page. The
		 *       packet buffers are stored backwards in each
		 *       page (starting from the highest address). We
		 *       utilize the fact that the last buffer in each
		 *       page has a 0 offset to detect that all the
		 *       buffers were processed in order to unmap the
		 *       page.
		 */
		if (unlikely(page_off < 0)) {
			bdx_page = bdx_rx_get_page(priv);
			if (bdx_page == NULL) {
				u32 timeout;
				timeout = 1000000;	/* 1/5 sec */
				WRITE_REG(priv, 0x5154, timeout);
				netdev_dbg
				    (priv->ndev,
				     "warning - system memory is temporary low\n");
				break;
			}
			page_off = ((page_size / buf_size) - 1) * buf_size;
			dma = bdx_page->dma;
			nPages += 1;
		} else {
			bdx_rx_ref_page(bdx_page);

			/*
			 * Page is already allocated and mapped, just
			 * increment the page usage count.
			 */
		}
		rxfd = (struct rxf_desc *)(f->m.va + f->m.wptr);
		idx = bdx_rxdb_alloc_elem(db);
		dm = bdx_rxdb_addr_elem(db, idx);
		dm->size = page_size;
		bdx_rx_set_dm_page(dm, bdx_page);
		dm->off = page_off;
		dm->dma = dma + page_off;
		netdev_dbg(priv->ndev, "dm size %d off %d dma %p\n",
			   dm->size, dm->off, (void *)dm->dma);
		page_off -= buf_size;

		rxfd->info = CPU_CHIP_SWAP32(0x10003);	/* INFO =1 BC =3 */
		rxfd->va_lo = idx;
		rxfd->pa_lo = CPU_CHIP_SWAP32(L32_64(dm->dma));
		rxfd->pa_hi = CPU_CHIP_SWAP32(H32_64(dm->dma));
		rxfd->len = CPU_CHIP_SWAP32(f->m.pktsz);
		print_rxfd(rxfd);
		f->m.wptr += sizeof(struct rxf_desc);
		delta = f->m.wptr - f->m.memsz;
		if (unlikely(delta >= 0)) {
			f->m.wptr = delta;
			if (delta > 0) {
				memcpy(f->m.va, f->m.va + f->m.memsz, delta);
				netdev_dbg(priv->ndev, "Wrapped descriptor\n");
			}
		}
		dno--;
	}
	netdev_dbg(priv->ndev, "nPages %d\n", nPages);
	WRITE_REG(priv, f->m.reg_WPTR, f->m.wptr & TXF_WPTR_WR_PTR);
	netdev_dbg(priv->ndev, "WRITE_REG 0x%04x f->m.reg_WPTR 0x%x\n",
		   f->m.reg_WPTR, f->m.wptr & TXF_WPTR_WR_PTR);
	netdev_dbg(priv->ndev, "READ_REG  0x%04x f->m.reg_RPTR=0x%x\n",
		   f->m.reg_RPTR, READ_REG(priv, f->m.reg_RPTR));
	netdev_dbg(priv->ndev, "READ_REG  0x%04x f->m.reg_WPTR=0x%x\n",
		   f->m.reg_WPTR, READ_REG(priv, f->m.reg_WPTR));
	dbg_printFifo(&priv->rxf_fifo0.m, (char *)"RXF");

}

static void bdx_rx_alloc_buffers(struct bdx_priv *priv)
{
	_bdx_rx_alloc_buffers(priv);

}

static void bdx_recycle_skb(struct bdx_priv *priv, struct rxd_desc *rxdd)
{
	struct rxdb *db = priv->rxdb0;
	struct rx_map *dm = bdx_rxdb_addr_elem(db, rxdd->va_lo);
	struct rxf_fifo *f = &priv->rxf_fifo0;
	struct rxf_desc *rxfd = (struct rxf_desc *)(f->m.va + f->m.wptr);
	int delta;

	rxfd->info = CPU_CHIP_SWAP32(0x10003);	/* INFO=1 BC=3 */
	rxfd->va_lo = rxdd->va_lo;
	rxfd->pa_lo = CPU_CHIP_SWAP32(L32_64(dm->dma));
	rxfd->pa_hi = CPU_CHIP_SWAP32(H32_64(dm->dma));
	rxfd->len = CPU_CHIP_SWAP32(f->m.pktsz);
	print_rxfd(rxfd);
	f->m.wptr += sizeof(struct rxf_desc);
	delta = f->m.wptr - f->m.memsz;
	if (unlikely(delta >= 0)) {
		f->m.wptr = delta;
		if (delta > 0) {
			memcpy(f->m.va, f->m.va + f->m.memsz, delta);
			netdev_dbg(priv->ndev, "wrapped descriptor\n");
		}
	}

}

static inline u16 tcpCheckSum(u16 *buf, u16 len, u16 *saddr, u16 *daddr,
			      u16 proto)
{
	u32 sum;
	u16 j = len;

	sum = 0;
	while (j > 1) {
		sum += *buf++;
		if (sum & 0x80000000) {
			sum = (sum & 0xFFFF) + (sum >> 16);
		}
		j -= 2;
	}
	if (j & 1) {
		sum += *((u8 *) buf);
	}
	/* Add the tcp pseudo-header */
	sum += *(saddr++);
	sum += *saddr;
	sum += *(daddr++);
	sum += *daddr;
	sum += __constant_htons(proto);
	sum += __constant_htons(len);
	/* Fold 32-bit sum to 16 bits */
	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}
	/* One's complement of sum */

	return ((u16) (sum));

}

static void bdx_skb_add_rx_frag(struct sk_buff *skb, int i, struct page *page,
				int off, int len)
{
	skb_add_rx_frag(skb, 0, page, off, len, SKB_TRUESIZE(len));
}

#define PKT_ERR_LEN		(70)

static int bdx_rx_error(struct bdx_priv *priv, char *pkt, u32 rxd_err, u16 len)
{
	struct ethhdr *eth = (struct ethhdr *)pkt;
	struct iphdr *iph =
	    (struct iphdr *)(pkt + sizeof(struct ethhdr) +
			     ((eth->h_proto ==
			       __constant_htons(ETH_P_8021Q)) ? VLAN_HLEN : 0));
	int rVal = 1;

	if (rxd_err == 0x8) {	/* UDP checksum error */
		struct udphdr *udp =
		    (struct udphdr *)((u8 *) iph + sizeof(struct iphdr));
		if (udp->check == 0) {
			netdev_dbg(priv->ndev, "false rxd_err = 0x%x\n",
				   rxd_err);
			rVal = 0;	/* Work around H/W false error indication */
		} else if (len < PKT_ERR_LEN) {
			u16 udpSum;
			udpSum =
			    tcpCheckSum((u16 *) udp,
					htons(iph->tot_len) -
					(iph->ihl * sizeof(u32)),
					(u16 *) & iph->saddr,
					(u16 *) & iph->daddr, IPPROTO_UDP);
			if (udpSum == 0xFFFF) {
				netdev_dbg(priv->ndev,
					   "false rxd_err = 0x%x\n", rxd_err);
				rVal = 0;	/* Work around H/W false error indication */
			}
		}
	} else if ((rxd_err == 0x10) && (len < PKT_ERR_LEN)) {	/* TCP checksum error */
		u16 tcpSum;
		struct tcphdr *tcp =
		    (struct tcphdr *)((u8 *) iph + sizeof(struct iphdr));
		tcpSum =
		    tcpCheckSum((u16 *) tcp,
				htons(iph->tot_len) - (iph->ihl * sizeof(u32)),
				(u16 *) & iph->saddr, (u16 *) & iph->daddr,
				IPPROTO_TCP);
		if (tcpSum == 0xFFFF) {
			netdev_dbg(priv->ndev, "false rxd_err = 0x%x\n",
				   rxd_err);
			rVal = 0;	/* Work around H/W false error indication */
		}
	}

	return rVal;

}

/* bdx_rx_receive - Receives full packet from RXD fifo and pass them to the OS.
 *
 * NOTE: A special treatment is given to non-contiguous descriptors that start
 *   near the end, wraps around and continue at the beginning. The second
 *   part is copied right after the first, and then descriptor is
 *   interpreted as normal. The fifo has an extra space to allow such
 *   operations
 *
 * @priv - NIC's private structure
 * @f    - RXF fifo that needs skbs
 */

/* TBD: replace memcpy func call by explicit inline asm */

static int bdx_rx_receive(struct bdx_priv *priv, struct rxd_fifo *f, int budget)
{
	struct sk_buff *skb;
	struct rxd_desc *rxdd;
	struct rx_map *dm;
	struct bdx_page *bdx_page;
	struct rxf_fifo *rxf_fifo;
	u32 rxd_val1, rxd_err;
	u16 len;
	u16 rxd_vlan;
	u32 pkt_id;
	int tmp_len, size;
	char *pkt;
	int done = 0;
	struct rxdb *db = NULL;

	f->m.wptr = READ_REG(priv, f->m.reg_WPTR) & TXF_WPTR_WR_PTR;
	size = f->m.wptr - f->m.rptr;
	if (size < 0) {
		size += f->m.memsz;	/* Size is negative :-) */
	}
	while (size > 0) {
		rxdd = (struct rxd_desc *)(f->m.va + f->m.rptr);
		db = priv->rxdb0;

		/*
		 * Note: We have a chicken and egg problem here. If the
		 *       descriptor is wrapped we first need to copy the tail
		 *       of the descriptor to the end of the buffer before
		 *       extracting values from the descriptor. However in
		 *       order to know if the descriptor is wrapped we need to
		 *       obtain the length of the descriptor from (the
		 *       wrapped) descriptor. Luckily the length is the first
		 *       word of the descriptor. Descriptor lengths are
		 *       multiples of 8 bytes so in case of a wrapped
		 *       descriptor the first 8 bytes guaranteed to appear
		 *       before the end of the buffer. We first obtain the
		 *       length, we then copy the rest of the descriptor if
		 *       needed and then extract the rest of the values from
		 *       the descriptor.
		 *
		 *   Do not change the order of operations as it will
		 *       break the code!!!
		 */
		rxd_val1 = CPU_CHIP_SWAP32(rxdd->rxd_val1);
		tmp_len = GET_RXD_BC(rxd_val1) << 3;
		pkt_id = GET_RXD_PKT_ID(rxd_val1);
		BDX_ASSERT(tmp_len <= 0);
		size -= tmp_len;
		/* CHECK FOR A PARTIALLY ARRIVED DESCRIPTOR */
		if (size < 0) {
			netdev_dbg
			    (priv->ndev,
			     "bdx_rx_receive() PARTIALLY ARRIVED DESCRIPTOR tmp_len %d\n",
			     tmp_len);
			break;
		}
		/* HAVE WE REACHED THE END OF THE QUEUE? */
		f->m.rptr += tmp_len;
		tmp_len = f->m.rptr - f->m.memsz;
		if (unlikely(tmp_len >= 0)) {
			f->m.rptr = tmp_len;
			if (tmp_len > 0) {
				/* COPY PARTIAL DESCRIPTOR TO THE END OF THE QUEUE */
				netdev_dbg(priv->ndev,
					   "wrapped desc rptr=%d tmp_len=%d\n",
					   f->m.rptr, tmp_len);
				memcpy(f->m.va + f->m.memsz, f->m.va, tmp_len);
			}
		}
		dm = bdx_rxdb_addr_elem(db, rxdd->va_lo);
		prefetch(dm);
		bdx_page = bdx_rx_page(dm);

		len = CPU_CHIP_SWAP16(rxdd->len);
		rxd_vlan = CPU_CHIP_SWAP16(rxdd->rxd_vlan);
		print_rxdd(rxdd, rxd_val1, len, rxd_vlan);
		/* CHECK FOR ERRORS */
		if (unlikely(rxd_err = GET_RXD_ERR(rxd_val1))) {
			int bErr = 1;

			if ((!(rxd_err & 0x4)) &&	/* NOT CRC error */
			    (((rxd_err == 0x8) && (pkt_id == 2)) ||	/* UDP checksum error */
			     ((rxd_err == 0x10) && (len < PKT_ERR_LEN) && (pkt_id == 1))	/* TCP checksum error */
			    )
			    ) {
				pkt =
				    ((char *)page_address(bdx_page->page) +
				     dm->off);
				bErr = bdx_rx_error(priv, pkt, rxd_err, len);
			}
			if (bErr) {

				netdev_err(priv->ndev, "rxd_err = 0x%x\n",
					   rxd_err);
				priv->net_stats.rx_errors++;
				bdx_recycle_skb(priv, rxdd);
				continue;
			}
		}
		netdev_dbg(priv->ndev, "tn40xx: * RX %d *\n", len);
		rxf_fifo = &priv->rxf_fifo0;

		/*
		 * Note: In this case we obtain a pre-allocated skb
		 *       from napi. We add a frag with the
		 *       page/off/len tuple of the buffer that we have
		 *       just read and then call
		 *       vlan_gro_frags()/napi_gro_frags() to process
		 *       the packet. The same skb is used again and
		 *       again to handle all packets, which eliminates
		 *       the need to allocate an skb for each packet.
		 */
		if ((skb = napi_get_frags(&priv->napi)) == NULL) {
			netdev_err(priv->ndev, "napi_get_frags failed\n");
			break;
		}
		skb->ip_summed =
		    (pkt_id == 0) ? CHECKSUM_NONE : CHECKSUM_UNNECESSARY;
		bdx_skb_add_rx_frag(skb, 0, bdx_page->page, dm->off, len);
		bdx_rxdb_free_elem(db, rxdd->va_lo);
		/* PROCESS PACKET */
		bdx_rx_vlan(priv, skb, rxd_val1, rxd_vlan);
		napi_gro_frags(&priv->napi);

		bdx_rx_reuse_page(priv, dm);

#if defined(USE_RSS)
		skb->hash = CPU_CHIP_SWAP32(rxdd->rss_hash);
		skb->l4_hash = 1;
		netdev_dbg(priv->ndev, "rxhash    = 0x%x\n", skb->hash);
#endif /* USE_RSS */
		priv->net_stats.rx_bytes += len;

		if (unlikely(++done >= budget)) {
			break;
		}
	}

	/* CLEANUP */
	LUXOR__GRO_FLUSH(&priv->napi);
	priv->net_stats.rx_packets += done;
	/* FIXME: Do something to minimize pci accesses    */
	WRITE_REG(priv, f->m.reg_RPTR, f->m.rptr & TXF_WPTR_WR_PTR);
	bdx_rx_alloc_buffers(priv);

	return done;

}

/*************************************************************************
 * Debug / Temporary Code                       *
 *************************************************************************/
static void print_rxdd(struct rxd_desc *rxdd, u32 rxd_val1, u16 len,
		       u16 rxd_vlan)
{
	pr_debug("rxdd bc %d rxfq %d to %d type %d err %d rxp %d "
		 "pkt_id %d vtag %d len %d vlan_id %d cfi %d prio %d "
		 "va_lo %d va_hi %d\n",
		 GET_RXD_BC(rxd_val1), GET_RXD_RXFQ(rxd_val1),
		 GET_RXD_TO(rxd_val1), GET_RXD_TYPE(rxd_val1),
		 GET_RXD_ERR(rxd_val1), GET_RXD_RXP(rxd_val1),
		 GET_RXD_PKT_ID(rxd_val1), GET_RXD_VTAG(rxd_val1), len,
		 GET_RXD_VLAN_ID(rxd_vlan), GET_RXD_CFI(rxd_vlan),
		 GET_RXD_PRIO(rxd_vlan), rxdd->va_lo, rxdd->va_hi);
}

static void print_rxfd(struct rxf_desc *rxfd)
{
	/*  pr_debug("=== RxF desc CHIP ORDER/ENDIANESS =============\n" */
	/*      "info 0x%x va_lo %u pa_lo 0x%x pa_hi 0x%x len 0x%x\n", */
	/*      rxfd->info, rxfd->va_lo, rxfd->pa_lo, rxfd->pa_hi, rxfd->len); */
}

/*
 * TX HW/SW interaction overview
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * There are 2 types of TX communication channels between driver and NIC.
 * 1) TX Free Fifo - TXF - Holds ack descriptors for sent packets.
 * 2) TX Data Fifo - TXD - Holds descriptors of full buffers.
 *
 * Currently the NIC supports TSO, checksumming and gather DMA
 * UFO and IP fragmentation is on the way.
 *
 * RX SW Data Structures
 * ~~~~~~~~~~~~~~~~~~~~~
 * TXDB is used to keep track of all skbs owned by SW and their DMA addresses.
 * For TX case, ownership lasts from getting the packet via hard_xmit and
 * until the HW acknowledges sending the packet by TXF descriptors.
 * TXDB is implemented as a cyclic buffer.
 *
 * FIFO objects keep info about the fifo's size and location, relevant HW
 * registers, usage and skb db. Each RXD and RXF fifo has their own fifo
 * structure.  Implemented as simple struct.
 *
 * TX SW Execution Flow
 * ~~~~~~~~~~~~~~~~~~~~
 * OS calls the driver's hard_xmit method with a packet to send. The driver
 * creates DMA mappings, builds TXD descriptors and kicks the HW by updating
 * TXD WPTR.
 *
 * When a packet is sent, The HW write a TXF descriptor and the SW frees the
 * original skb.  To prevent TXD fifo overflow without reading HW registers
 * every time, the SW deploys "tx level" technique. Upon startup, the tx level
 * is initialized to TXD fifo length.  For every sent packet, the SW gets its
 * TXD descriptor size (from a pre-calculated array) and subtracts it from tx
 * level.  The size is also stored in txdb. When a TXF ack arrives, the SW
 * fetched the size of the original TXD descriptor from the txdb and adds it
 * to the tx level.  When the Tx level drops below some predefined threshold,
 * the driver stops the TX queue. When the TX level rises above that level,
 * the tx queue is enabled again.
 *
 * This technique avoids excessive reading of RPTR and WPTR registers.
 * As our benchmarks shows, it adds 1.5 Gbit/sec to NIS's throughput.
 */

/*************************************************************************
 *     Tx DB                                 *
 *************************************************************************/
static inline int bdx_tx_db_size(struct txdb *db)
{
	int taken = db->wptr - db->rptr;

	if (taken < 0)
		taken = db->size + 1 + taken;	/* (size + 1) equals memsz */

	return db->size - taken;
}

/* __bdx_tx_ptr_next - A helper function, increment read/write pointer + wrap.
 *
 * @d   - Tx data base
 * @ptr - Read or write pointer
 */
static inline void __bdx_tx_db_ptr_next(struct txdb *db, struct tx_map **pptr)
{
	BDX_ASSERT(db == NULL || pptr == NULL);	/* sanity */
	BDX_ASSERT(*pptr != db->rptr &&	/* expect either read */
		   *pptr != db->wptr);	/* or write pointer */
	BDX_ASSERT(*pptr < db->start ||	/* pointer has to be */
		   *pptr >= db->end);	/* in range */

	++*pptr;
	if (unlikely(*pptr == db->end))
		*pptr = db->start;
}

/* bdx_tx_db_inc_rptr - Increment the read pointer.
 *
 * @d - tx data base
 */
static inline void bdx_tx_db_inc_rptr(struct txdb *db)
{
	BDX_ASSERT(db->rptr == db->wptr);	/* can't read from empty db */
	__bdx_tx_db_ptr_next(db, &db->rptr);
}

/* bdx_tx_db_inc_rptr - Increment the   write pointer.
 *
 * @d - tx data base
 */
static inline void bdx_tx_db_inc_wptr(struct txdb *db)
{
	__bdx_tx_db_ptr_next(db, &db->wptr);
	BDX_ASSERT(db->rptr == db->wptr);	/* we can not get empty db as
						   a result of write */
}

/* bdx_tx_db_init - Create and initialize txdb.
 *
 * @d       - tx data base
 * @sz_type - size of tx fifo
 * Returns 0 on success, error code otherwise
 */
static int bdx_tx_db_init(struct txdb *d, int sz_type)
{
	int memsz = FIFO_SIZE * (1 << (sz_type + 1));

	d->start = vmalloc(memsz);
	if (!d->start)
		return -ENOMEM;

	/*
	 * In order to differentiate between an empty db state and a full db
	 * state at least one element should always be empty in order to
	 * avoid rptr == wptr, which means that the db is empty.
	 */
	d->size = memsz / sizeof(struct tx_map) - 1;
	d->end = d->start + d->size + 1;	/* just after last element */

	/* All dbs are created empty */
	d->rptr = d->start;
	d->wptr = d->start;

	return 0;
}

/* bdx_tx_db_close - Close tx db and free all memory.
 *
 * @d - tx data base
 */
static void bdx_tx_db_close(struct txdb *d)
{
	BDX_ASSERT(d == NULL);

	if (d->start) {
		vfree(d->start);
		d->start = NULL;
	}
}

/*************************************************************************
 *     Tx Engine                             *
 *************************************************************************/

/*
 * Sizes of tx desc (including padding if needed) as function of the SKB's
 * frag number
 */
static struct {
	u16 bytes;
	u16 qwords;		/* qword = 64 bit */
} txd_sizes[MAX_PBL];

/* txdb_map_skb - Create and store DMA mappings for skb's data blocks.
 *
 * @priv - NIC private structure
 * @skb  - socket buffer to map
 *
 * This function creates DMA mappings for skb's data blocks and writes them to
 * PBL of a new tx descriptor. It also stores them in the tx db, so they could
 * be unmapped after the data has been sent. It is the responsibility of the
 * caller to make sure that there is enough space in the txdb. The last
 * element holds a pointer to skb itself and is marked with a zero length.
 */
inline void bdx_setPbl(struct pbl *pbl, dma_addr_t dmaAddr, int len)
{
	pbl->len = CPU_CHIP_SWAP32(len);
	pbl->pa_lo = CPU_CHIP_SWAP32(L32_64(dmaAddr));
	pbl->pa_hi = CPU_CHIP_SWAP32(H32_64(dmaAddr));
	dbg_printPBL(pbl);

}

static inline void bdx_setTxdb(struct txdb *db, dma_addr_t dmaAddr, int len)
{
	db->wptr->len = len;
	db->wptr->addr.dma = dmaAddr;

}

static inline int bdx_tx_map_skb(struct bdx_priv *priv, struct sk_buff *skb,
				 struct txd_desc *txdd, int *nr_frags,
				 unsigned int *pkt_len)
{
	skb_frag_t *frag;
	dma_addr_t dmaAddr;
	int i, len;
	struct txdb *db = &priv->txdb;
	struct pbl *pbl = &txdd->pbl[0];
	int nrFrags = skb_shinfo(skb)->nr_frags;
	int copyFrags = 0;
	unsigned int size;

	netdev_dbg(priv->ndev, "TX skb %p skbLen %d dataLen %d frags %d\n",
		   skb, skb->len, skb->data_len, nrFrags);

	if (nrFrags > MAX_PBL - 1) {
		netdev_err(priv->ndev, "MAX PBL exceeded %d !!!\n", nrFrags);
		return -1;
	}
	*nr_frags = nrFrags;
	/* initial skb */
	len = skb->len - skb->data_len;
	dmaAddr =
	    dma_map_single(&priv->pdev->dev, skb->data, len, DMA_TO_DEVICE);
	bdx_setTxdb(db, dmaAddr, len);
	bdx_setPbl(pbl++, db->wptr->addr.dma, db->wptr->len);
	*pkt_len = db->wptr->len;

	/* remaining frags */
	for (i = copyFrags; i < nrFrags; i++) {

		frag = &skb_shinfo(skb)->frags[i];
		size = skb_frag_size(frag);
		dmaAddr =
		    skb_frag_dma_map(&priv->pdev->dev, frag, 0,
				     size, DMA_TO_DEVICE);
		bdx_tx_db_inc_wptr(db);
		bdx_setTxdb(db, dmaAddr, size);
		bdx_setPbl(pbl++, db->wptr->addr.dma, db->wptr->len);
		*pkt_len += db->wptr->len;
	}
	if (skb->len < 60) {
		++nrFrags;	/* SHORT_PKT_FIX */
	}
	/* Add skb clean up info. */
	bdx_tx_db_inc_wptr(db);
	db->wptr->len = -txd_sizes[nrFrags].bytes;
	db->wptr->addr.skb = skb;
	bdx_tx_db_inc_wptr(db);

	return 0;
}

/*
 * init_txd_sizes - Pre-calculate the sizes of descriptors for skbs up to 16
 * frags The number of frags is used as an index to fetch the correct
 * descriptors size, instead of calculating it each time
 */
static void __init init_txd_sizes(void)
{
	int i, lwords;

	/* 7 - is number of lwords in txd with one phys buffer
	 * 3 - is number of lwords used for every additional phys buffer
	 */
	for (i = 0; i < MAX_PBL; i++) {
		lwords = 7 + (i * 3);
		if (lwords & 1)
			lwords++;	/* pad it with 1 lword */
		txd_sizes[i].qwords = lwords >> 1;
		txd_sizes[i].bytes = lwords << 2;
		pr_debug("%2d. %d\n", i, txd_sizes[i].bytes);
	}
}

/*
 * bdx_tx_init - Initialize all Tx related stuff.  Namely, TXD and TXF fifos,
 *       database etc
 */
static int bdx_tx_init(struct bdx_priv *priv)
{
	if (bdx_fifo_init(priv, &priv->txd_fifo0.m, priv->txd_size,
			  regTXD_CFG0_0, regTXD_CFG1_0,
			  regTXD_RPTR_0, regTXD_WPTR_0))
		goto err_mem;
	if (bdx_fifo_init(priv, &priv->txf_fifo0.m, priv->txf_size,
			  regTXF_CFG0_0, regTXF_CFG1_0,
			  regTXF_RPTR_0, regTXF_WPTR_0))
		goto err_mem;

	/*
	 * The TX db has to keep mappings for all packets sent (on TxD)
	 * and not yet reclaimed (on TxF)
	 */
	if (bdx_tx_db_init(&priv->txdb, max(priv->txd_size, priv->txf_size)))
		goto err_mem;

	/* SHORT_PKT_FIX */
	priv->b0_len = 64;
	priv->b0_va =
	    dma_alloc_coherent(&priv->pdev->dev, priv->b0_len, &priv->b0_dma,
			       GFP_ATOMIC);
	memset(priv->b0_va, 0, priv->b0_len);
	/* SHORT_PKT_FIX end */

	priv->tx_level = BDX_MAX_TX_LEVEL;
	priv->tx_update_mark = priv->tx_level - 1024;

	return 0;

err_mem:
	netdev_err(priv->ndev, "Tx init failed\n");
	return -ENOMEM;
}

/*
 * bdx_tx_space - Calculate the available space in the TX fifo.
 *
 * @priv - NIC private structure
 * Returns available space in TX fifo in bytes
 */
static inline int bdx_tx_space(struct bdx_priv *priv)
{
	struct txd_fifo *f = &priv->txd_fifo0;
	int fsize;

	f->m.rptr = READ_REG(priv, f->m.reg_RPTR) & TXF_WPTR_WR_PTR;
	fsize = f->m.rptr - f->m.wptr;
	if (fsize <= 0)
		fsize = f->m.memsz + fsize;
	return (fsize);
}

void bdx_tx_timeout(struct net_device *ndev)
{
	netdev_dbg(ndev, "TX timeout\n");
}

/* bdx_tx_transmit - Send a packet to the NIC.
 *
 * @skb - Packet to send
 * ndev - The network device assigned to NIC
 *
 * Return codes:
 * o NETDEV_TX_OK   Everything is  OK.
 *
 * o NETDEV_TX_BUSY Cannot transmit packet, try later. Usually a bug, means
 *          queue start/stop flow control is broken in the driver.
 *          Note: The driver must NOT put the skb in its DMA ring.
 *
 * o NETDEV_TX_LOCKED Locking failed, please retry quickly.
 */
static int bdx_tx_transmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct bdx_priv *priv = netdev_priv(ndev);
	struct txd_fifo *f = &priv->txd_fifo0;
	int txd_checksum = 7;	/* full checksum */
	int txd_lgsnd = 0;
	int txd_vlan_id = 0;
	int txd_vtag = 0;
	int txd_mss = 0;
	unsigned int pkt_len;
	struct txd_desc *txdd;
	int nr_frags, len;
	DBG_OFF;

	if (!(priv->state & BDX_STATE_STARTED)) {
		return -1;
	}

	/* Build tx descriptor */
	BDX_ASSERT(f->m.wptr >= f->m.memsz);	/* started with valid wptr */
	txdd = (struct txd_desc *)(f->m.va + f->m.wptr);
	if (bdx_tx_map_skb(priv, skb, txdd, &nr_frags, &pkt_len) != 0) {
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;	// probably not entirely OK.
	}

	if (unlikely(skb->ip_summed != CHECKSUM_PARTIAL)) {
		txd_checksum = 0;
	}

	if (skb_shinfo(skb)->gso_size) {
		txd_mss = skb_shinfo(skb)->gso_size;
		txd_lgsnd = 1;
		netdev_dbg(ndev, "skb %p pkt len %d gso size = %d\n", skb,
			   pkt_len, txd_mss);
	}

	if (vlan_tx_tag_present(skb)) {
		/* Don't cut VLAN ID to 12 bits */
		txd_vlan_id = vlan_tx_tag_get(skb);
		txd_vtag = 1;
	}
	txdd->va_hi = 0;
	txdd->va_lo = (u32) ((u64) skb);
	txdd->length = CPU_CHIP_SWAP16(pkt_len);
	txdd->mss = CPU_CHIP_SWAP16(txd_mss);
	txdd->txd_val1 =
	    CPU_CHIP_SWAP32(TXD_W1_VAL
			    (txd_sizes[nr_frags].qwords, txd_checksum,
			     txd_vtag, txd_lgsnd, txd_vlan_id));

	netdev_dbg(ndev, "w1 qwords[%d] %d\n", nr_frags,
		   txd_sizes[nr_frags].qwords);
	netdev_dbg(ndev, "TxD desc w1: 0x%x w2: mss 0x%x len 0x%x\n",
		   txdd->txd_val1, txdd->mss, txdd->length);

	/*
	 * Increment TXD write pointer. In case of fifo wrapping copy reminder of
	 *  the descriptor to the beginning
	 */
	f->m.wptr += txd_sizes[nr_frags].bytes;
	len = f->m.wptr - f->m.memsz;
	if (unlikely(len >= 0)) {
		f->m.wptr = len;
		if (len > 0) {
			BDX_ASSERT(len > f->m.memsz);
			memcpy(f->m.va, f->m.va + f->m.memsz, len);
		}
	}
	BDX_ASSERT(f->m.wptr >= f->m.memsz);	/* finished with valid wptr */
	priv->tx_level -= txd_sizes[nr_frags].bytes;
	BDX_ASSERT(priv->tx_level <= 0 || priv->tx_level > BDX_MAX_TX_LEVEL);
#if (defined(TN40_PTP) && defined(ETHTOOL_GET_TS_INFO))
	skb_tx_timestamp(skb);
#endif
	if (priv->tx_level > priv->tx_update_mark) {
		/*
		 * Force memory writes to complete before letting the HW know
		 * there are new descriptors to fetch (might be needed on
		 * platforms like IA64).
		 *  wmb();
		 */
		WRITE_REG(priv, f->m.reg_WPTR, f->m.wptr & TXF_WPTR_WR_PTR);
	} else {
		if (priv->tx_noupd++ > BDX_NO_UPD_PACKETS) {
			priv->tx_noupd = 0;
			WRITE_REG(priv, f->m.reg_WPTR,
				  f->m.wptr & TXF_WPTR_WR_PTR);
		}
	}
	netif_trans_update(ndev);
	priv->net_stats.tx_packets++;
	priv->net_stats.tx_bytes += pkt_len;
	if (priv->tx_level < BDX_MIN_TX_LEVEL) {
		netdev_dbg(ndev, "TX Q STOP level %d\n", priv->tx_level);
		netif_stop_queue(ndev);
	}

	return NETDEV_TX_OK;
}

/* bdx_tx_cleanup - Clean the TXF fifo, run in the context of IRQ.
 *
 * @priv - bdx adapter
 *
 * This function scans the TXF fifo for descriptors, frees DMA mappings and
 * reports to the OS that those packets were sent.
 */
static void bdx_tx_cleanup(struct bdx_priv *priv)
{
	struct txf_fifo *f = &priv->txf_fifo0;
	struct txdb *db = &priv->txdb;
	int tx_level = 0;

	f->m.wptr = READ_REG(priv, f->m.reg_WPTR) & TXF_WPTR_MASK;
	BDX_ASSERT(f->m.rptr >= f->m.memsz);	/* Started with valid rptr */
	netif_tx_lock(priv->ndev);

	while (f->m.wptr != f->m.rptr) {
		f->m.rptr += BDX_TXF_DESC_SZ;
		f->m.rptr &= f->m.size_mask;
		/* Unmap all fragments */
		/* First has to come tx_maps containing DMA */
		BDX_ASSERT(db->rptr->len == 0);
		do {
			BDX_ASSERT(db->rptr->addr.dma == 0);
			netdev_dbg(priv->ndev,
				   "dma_unmap_page 0x%llx len %d\n",
				   db->rptr->addr.dma, db->rptr->len);
			dma_unmap_page(&priv->pdev->dev, db->rptr->addr.dma,
				       db->rptr->len, DMA_TO_DEVICE);
			bdx_tx_db_inc_rptr(db);
		} while (db->rptr->len > 0);
		tx_level -= db->rptr->len;	/* '-' Because the len is negative */

		/* Now should come skb pointer - free it */
		dev_kfree_skb_any(db->rptr->addr.skb);
		netdev_dbg(priv->ndev, "dev_kfree_skb_any %p %d\n",
			   db->rptr->addr.skb, -db->rptr->len);
		bdx_tx_db_inc_rptr(db);
	}

	/* Let the HW know which TXF descriptors were cleaned */
	BDX_ASSERT((f->m.wptr & TXF_WPTR_WR_PTR) >= f->m.memsz);
	WRITE_REG(priv, f->m.reg_RPTR, f->m.rptr & TXF_WPTR_WR_PTR);

	/*
	 * We reclaimed resources, so in case the Q is stopped by xmit callback,
	 * we resume the transmission and use tx_lock to synchronize with xmit.
	 */
	priv->tx_level += tx_level;
	BDX_ASSERT(priv->tx_level <= 0 || priv->tx_level > BDX_MAX_TX_LEVEL);
	if (priv->tx_noupd) {
		priv->tx_noupd = 0;
		WRITE_REG(priv, priv->txd_fifo0.m.reg_WPTR,
			  priv->txd_fifo0.m.wptr & TXF_WPTR_WR_PTR);
	}
	if (unlikely(netif_queue_stopped(priv->ndev) &&
		     netif_carrier_ok(priv->ndev) &&
		     (priv->tx_level >= BDX_MAX_TX_LEVEL / 2))) {
		netdev_dbg(priv->ndev, "TX Q WAKE level %d\n", priv->tx_level);
		netif_wake_queue(priv->ndev);
	}
	netif_tx_unlock(priv->ndev);
}

/* bdx_tx_free_skbs - Free all skbs from TXD fifo.
 *
 * This function is called when the OS shuts down this device, e.g. upon
 * "ifconfig down" or rmmod.
 */
static void bdx_tx_free_skbs(struct bdx_priv *priv)
{
	struct txdb *db = &priv->txdb;

	while (db->rptr != db->wptr) {
		if (likely(db->rptr->len))
			dma_unmap_page(&priv->pdev->dev, db->rptr->addr.dma,
				       db->rptr->len, DMA_TO_DEVICE);
		else
			dev_kfree_skb(db->rptr->addr.skb);
		bdx_tx_db_inc_rptr(db);
	}

}

/* bdx_tx_free - Free all Tx resources */

static void bdx_tx_free(struct bdx_priv *priv)
{

	bdx_tx_free_skbs(priv);
	bdx_fifo_free(priv, &priv->txd_fifo0.m);
	bdx_fifo_free(priv, &priv->txf_fifo0.m);
	bdx_tx_db_close(&priv->txdb);
	/* SHORT_PKT_FIX */
	if (priv->b0_len) {
		dma_free_coherent(&priv->pdev->dev, priv->b0_len, priv->b0_va,
				  (enum dma_data_direction)priv->b0_dma);
		priv->b0_len = 0;
	}
	/* SHORT_PKT_FIX end */

}

/* bdx_tx_push_desc - Push a descriptor to TxD fifo.
 *
 * @priv - NIC private structure
 * @data - desc's data
 * @size - desc's size
 *
 * This function pushes desc to TxD fifo and overlaps it if needed.
 *
 * NOTE: This function does not check for available space, nor does it check
 *   that the data size is smaller than the fifo size. Checking for space
 *   is the responsibility of the caller.
 */
static void bdx_tx_push_desc(struct bdx_priv *priv, void *data, int size)
{
	struct txd_fifo *f = &priv->txd_fifo0;
	int i = f->m.memsz - f->m.wptr;

	if (size == 0)
		return;

	if (i > size) {
		memcpy(f->m.va + f->m.wptr, data, size);
		f->m.wptr += size;
	} else {
		memcpy(f->m.va + f->m.wptr, data, i);
		f->m.wptr = size - i;
		memcpy(f->m.va, data + i, f->m.wptr);
	}
	WRITE_REG(priv, f->m.reg_WPTR, f->m.wptr & TXF_WPTR_WR_PTR);
}

/* bdx_tx_push_desc_safe - Push descriptor to TxD fifo in a safe way.
 *
 * @priv - NIC private structure
 * @data - descriptor data
 * @size - descriptor size
 *
 * NOTE: This function does check for available space and, if necessary, waits
 *   for the NIC to read existing data before writing new data.
 */

static void bdx_tx_push_desc_safe(struct bdx_priv *priv, void *data, int size)
{
	int timer = 0;

	while (size > 0) {
		/*
		 * We subtract 8 because when the fifo is full rptr == wptr, which
		 * also means that fifo is empty, we can understand the difference,
		 * but could the HW do the same ??? :)
		 */
		int avail = bdx_tx_space(priv) - 8;
		if (avail <= 0) {
			if (timer++ > 300) {	/* Prevent endless loop */
				netdev_dbg
				    (priv->ndev,
				     "timeout while writing desc to TxD fifo\n");
				break;
			}
			udelay(50);	/* Give the HW a chance to clean the fifo */
			continue;
		}
		avail = MIN(avail, size);
		netdev_dbg(priv->ndev,
			   "about to push  %d bytes starting %p size %d\n",
			   avail, data, size);
		bdx_tx_push_desc(priv, data, avail);
		size -= avail;
		data += avail;
	}

}

static int bdx_ioctl_priv(struct net_device *ndev, struct ifreq *ifr, int cmd)
{
	struct bdx_priv *priv = netdev_priv(ndev);
	tn40_ioctl_t tn40_ioctl;
	int error;
	u16 dev, addr;

	netdev_dbg(ndev, "jiffies =%ld cmd =%d\n", jiffies, cmd);
	if (cmd != SIOCDEVPRIVATE) {
		error =
		    copy_from_user(&tn40_ioctl, ifr->ifr_data,
				   sizeof(tn40_ioctl));
		if (error) {
			netdev_err(ndev, "cant copy from user\n");
			return error;
		}
		netdev_dbg(ndev, "%d 0x%x 0x%x 0x%p\n", tn40_ioctl.data[0],
			   tn40_ioctl.data[1], tn40_ioctl.data[2],
			   tn40_ioctl.buf);
	}
	if (!capable(CAP_SYS_RAWIO))
		return -EPERM;

	switch (tn40_ioctl.data[0]) {
	case OP_INFO:
		switch (tn40_ioctl.data[1]) {
		case 1:
			tn40_ioctl.data[2] = 1;
			break;
		case 2:
			tn40_ioctl.data[2] = priv->phy_mdio_port;
			break;
		default:
			tn40_ioctl.data[2] = 0xFFFFFFFF;
			break;
		}
		error =
		    copy_to_user(ifr->ifr_data, &tn40_ioctl,
				 sizeof(tn40_ioctl));
		if (error)
			return error;
		break;

	case OP_READ_REG:
		error = bdx_range_check(priv, tn40_ioctl.data[1]);
		if (error < 0)
			return error;
		tn40_ioctl.data[2] = READ_REG(priv, tn40_ioctl.data[1]);
		netdev_dbg(ndev, "read_reg(0x%x)=0x%x (dec %d)\n",
			   tn40_ioctl.data[1], tn40_ioctl.data[2],
			   tn40_ioctl.data[2]);
		error =
		    copy_to_user(ifr->ifr_data, &tn40_ioctl,
				 sizeof(tn40_ioctl));
		if (error)
			return error;
		break;

	case OP_WRITE_REG:
		error = bdx_range_check(priv, tn40_ioctl.data[1]);
		if (error < 0)
			return error;
		WRITE_REG(priv, tn40_ioctl.data[1], tn40_ioctl.data[2]);
		break;

	case OP_MDIO_READ:
		if (priv->phy_mdio_port == 0xFF)
			return -EINVAL;
		dev = (u16) (0xFFFF & (tn40_ioctl.data[1] >> 16));
		addr = (u16) (0xFFFF & tn40_ioctl.data[1]);
		tn40_ioctl.data[2] = 0xFFFF & PHY_MDIO_READ(priv, dev, addr);
		error =
		    copy_to_user(ifr->ifr_data, &tn40_ioctl,
				 sizeof(tn40_ioctl));
		if (error)
			return error;
		break;

	case OP_MDIO_WRITE:
		if (priv->phy_mdio_port == 0xFF)
			return -EINVAL;
		dev = (u16) (0xFFFF & (tn40_ioctl.data[1] >> 16));
		addr = (u16) (0xFFFF & tn40_ioctl.data[1]);
		PHY_MDIO_WRITE(priv, dev, addr, (u16) (tn40_ioctl.data[2]));
		break;

#ifdef _TRACE_LOG_
	case op_TRACE_ON:
		traceOn();
		break;

	case op_TRACE_OFF:
		traceOff();
		break;

	case op_TRACE_ONCE:
		traceOnce();
		break;

	case op_TRACE_PRINT:
		tracePrint();
		break;
#endif
#ifdef TN40_MEMLOG
	case OP_MEMLOG_DMESG:
		memLogDmesg();
		break;

	case OP_MEMLOG_PRINT:
		{
			char *buf;
			uint buf_size;
			unsigned long bytes;

			error = 0;
			buf = memLogGetLine(&buf_size);
			if (buf != NULL) {

				tn40_ioctl.data[2] =
				    min(tn40_ioctl.data[1], buf_size);
				bytes =
				    copy_to_user(ifr->ifr_data, &tn40_ioctl,
						 sizeof(tn40_ioctl));
				bytes =
				    copy_to_user(tn40_ioctl.buf, buf,
						 tn40_ioctl.data[2]);
				netdev_dbg(ndev,
					   "copy_to_user %p %u return %lu\n",
					   tn40_ioctl.buf, tn40_ioctl.data[2],
					   bytes);
			} else {
				netdev_dbg
				    (ndev,
				     "=================== EOF =================\n");
				error = -EIO;
			}
			return error;
			break;
		}
#endif
#ifdef TN40_DEBUG
	case OP_DBG:
		switch (tn40_ioctl.data[1]) {
#ifdef _DRIVER_RESUME_DBG

			pm_message_t pmMsg = { 0 };

		case DBG_SUSPEND:
			bdx_suspend(priv->pdev, pmMsg);
			bdx_resume(priv->pdev);
			break;

		case DBG_RESUME:
			bdx_resume(priv->pdev);
			break;
#endif
		case DBG_START_DBG:
			DBG_ON;
			break;

		case DBG_STOP_DBG:
			DBG_OFF;
			break;

		case DBG_PRINT_PAGE_TABLE:
			dbg_printRxPageTable(priv);
			break;

		default:
			dbg_printIoctl();
			break;

		}
		break;
#endif /* TN40_DEBUG */

	default:
		return -EOPNOTSUPP;
	}

	return 0;

}

static int bdx_ioctl(struct net_device *ndev, struct ifreq *ifr, int cmd)
{

	if (cmd > SIOCDEVPRIVATE && cmd <= (SIOCDEVPRIVATE + 15))
		return bdx_ioctl_priv(ndev, ifr, cmd);
	else
		return -EOPNOTSUPP;

}

static const struct net_device_ops bdx_netdev_ops = {
	.ndo_open = bdx_open,
	.ndo_stop = bdx_close,
	.ndo_start_xmit = bdx_tx_transmit,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_do_ioctl = bdx_ioctl,
	.ndo_set_rx_mode = bdx_setmulti,
	.ndo_get_stats = bdx_get_stats,
	.ndo_change_mtu = bdx_change_mtu,
	.ndo_set_mac_address = bdx_set_mac,
	.ndo_vlan_rx_add_vid = bdx_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = bdx_vlan_rx_kill_vid,
};

static int bdx_get_ports_by_id(int vendor, int device)
{
	return 1;
}

static int bdx_get_phy_by_id(int vendor, int device, int subsystem, int port)
{
	int i = 0;
	for (; bdx_dev_tbl[i].vid; i++) {
		if ((bdx_dev_tbl[i].vid == vendor)
		    && (bdx_dev_tbl[i].pid == device)
		    && (bdx_dev_tbl[i].subdev == subsystem)
		    )
			return (port ==
				0) ? bdx_dev_tbl[i].phya : bdx_dev_tbl[i].phyb;
	}
	return 0;

}

/**
 * bdx_probe - Device Initialization Routine.
 *
 * @pdev - PCI device information struct
 * @ent  - Entry in bdx_pci_tbl
 *
 * Returns 0 on success, a negative error code on failure
 *
 * bdx_probe initializes an adapter identified by a pci_dev structure.
 * The OS initialization, adapter private structure configuration,
 * and a hardware reset occur.
 *
 * Functions and their order are used as explained in
 * /usr/src/linux/Documentation/DMA-{API, mapping}.txt
 *
 */

/* TBD: netif_msg should be checked and implemented. I disable it for now */

static int __init bdx_probe(struct pci_dev *pdev,
			    const struct pci_device_id *ent)
{
	struct net_device *ndev;
	struct bdx_priv *priv;
	int err, pci_using_dac;
	resource_size_t pciaddr;
	u32 regionSize;
	struct pci_nic *nic;
	int phy;
	unsigned int nvec = 1;

	nic = vmalloc(sizeof(*nic));
	if (!nic) {
		dev_err(&pdev->dev, "bdx_probe() no memory\n");
		return -ENOMEM;

	}

       /************** pci *****************/
	if ((err = pci_enable_device(pdev)))	/* It triggers interrupt, dunno
						   why. */
		goto err_pci;	/* it's not a problem though */

	if (!(err = dma_set_mask(&pdev->dev, LUXOR__DMA_64BIT_MASK)) &&
	    !(err = dma_set_coherent_mask(&pdev->dev, LUXOR__DMA_64BIT_MASK))) {
		pci_using_dac = 1;
	} else {
		if ((err = dma_set_mask(&pdev->dev, LUXOR__DMA_32BIT_MASK)) ||
		    (err = dma_set_coherent_mask(&pdev->dev,
						 LUXOR__DMA_32BIT_MASK))) {
			pr_err("No usable DMA configuration - aborting\n");
			goto err_dma;
		}
		pci_using_dac = 0;
	}

	if ((err = pci_request_regions(pdev, BDX_DRV_NAME)))
		goto err_dma;

	pci_set_master(pdev);

	pciaddr = pci_resource_start(pdev, 0);
	if (!pciaddr) {
		err = -EIO;
		dev_err(&pdev->dev, "no MMIO resource\n");
		goto err_out_res;
	}
	if ((regionSize = pci_resource_len(pdev, 0)) < BDX_REGS_SIZE) {
		/*      err = -EIO; */
		dev_err(&pdev->dev, "MMIO resource (%x) too small\n",
			regionSize);
		/*      goto err_out_res; */
	}

	nic->regs = ioremap(pciaddr, regionSize);
	if (!nic->regs) {
		err = -EIO;
		dev_err(&pdev->dev, "ioremap failed\n");
		goto err_out_res;
	}

	if (pdev->irq < 2) {
		err = -EIO;
		dev_err(&pdev->dev, "invalid irq (%d)\n", pdev->irq);
		goto err_out_iomap;
	}
	pci_set_drvdata(pdev, nic);

	nic->port_num = bdx_get_ports_by_id(pdev->vendor, pdev->device);
	print_hw_id(pdev);
	bdx_hw_reset_direct(pdev, nic->regs);

	nvec = pci_alloc_irq_vectors(pdev, 1, nvec, PCI_IRQ_MSI);
	if (nvec < 0) {
		goto err_out_iomap;
	}
    /************** netdev **************/
	if (!(ndev = alloc_etherdev(sizeof(struct bdx_priv)))) {
		err = -ENOMEM;
		dev_err(&pdev->dev, "alloc_etherdev failed\n");
		goto err_out_iomap;
	}

	ndev->netdev_ops = &bdx_netdev_ops;
	ndev->tx_queue_len = BDX_NDEV_TXQ_LEN;

	/*
	 * These fields are used for information purposes only,
	 * so we use the same for all the ports on the board
	 */
	ndev->if_port = 0;
	ndev->base_addr = pciaddr;
	ndev->mem_start = pciaddr;
	ndev->mem_end = pciaddr + regionSize;
	ndev->irq = pdev->irq;
	ndev->features = NETIF_F_IP_CSUM |
	    NETIF_F_SG |
	    NETIF_F_FRAGLIST |
	    NETIF_F_TSO | NETIF_F_VLAN_TSO | NETIF_F_VLAN_CSUM | NETIF_F_GRO |
	    NETIF_F_RXCSUM | NETIF_F_RXHASH;

#ifdef NETIF_F_HW_VLAN_CTAG_RX
	ndev->features |= NETIF_F_HW_VLAN_CTAG_TX |
	    NETIF_F_HW_VLAN_CTAG_RX | NETIF_F_HW_VLAN_CTAG_FILTER;
#else
	ndev->features |= NETIF_F_HW_VLAN_TX |
	    NETIF_F_HW_VLAN_RX | NETIF_F_HW_VLAN_FILTER;
#endif

	if (pci_using_dac)
		ndev->features |= NETIF_F_HIGHDMA;

	ndev->vlan_features = (NETIF_F_IP_CSUM |
			       NETIF_F_SG |
			       NETIF_F_TSO | NETIF_F_GRO | NETIF_F_RXHASH);
	if (pci_using_dac)
		ndev->vlan_features |= NETIF_F_HIGHDMA;
	ndev->min_mtu = ETH_ZLEN;
	ndev->max_mtu = BDX_MAX_MTU;

	/************** PRIV ****************/
	priv = nic->priv = netdev_priv(ndev);

	memset(priv, 0, sizeof(struct bdx_priv));
	priv->drv_name = BDX_DRV_NAME;
	priv->pBdxRegs = nic->regs;
	priv->port = 0;
	priv->pdev = pdev;
	priv->ndev = ndev;
	priv->nic = nic;
	priv->msg_enable = BDX_DEF_MSG_ENABLE;
	priv->deviceId = pdev->device;
	LUXOR__NAPI_ADD(ndev, &priv->napi, bdx_poll, 64);

	if ((readl(nic->regs + FPGA_VER) & 0xFFF) == 308) {
		dev_dbg(&ndev->dev, "HW statistics not supported\n");
		priv->stats_flag = 0;
	} else {
		priv->stats_flag = 1;
	}
	/*Init PHY */
	priv->subsystem_vendor = priv->pdev->subsystem_vendor;
	priv->subsystem_device = priv->pdev->subsystem_device;
	phy =
	    bdx_get_phy_by_id(pdev->vendor, pdev->device,
			      pdev->subsystem_device, 0);
	if (bdx_mdio_reset(priv, 0, phy) == -1) {
		err = -ENODEV;
		goto err_out_iomap;
	}

	bdx_ethtool_ops(ndev);	/* Ethtool interface */

	/* Initialize fifo sizes. */
	priv->txd_size = 3;
	priv->txf_size = 3;

	priv->rxd_size = 3;
	priv->rxf_size = 3;

	/* Initialize the initial coalescing registers. */
	priv->rdintcm = INT_REG_VAL(0x20, 1, 4, 12);
	priv->tdintcm = INT_REG_VAL(0x20, 1, 0, 12);
	priv->state = BDX_STATE_HW_STOPPED;
	/*
	 * ndev->xmit_lock spinlock is not used.
	 * Private priv->tx_lock is used for synchronization
	 * between transmit and TX irq cleanup.  In addition
	 * set multicast list callback has to use priv->tx_lock.
	 */
	ndev->hw_features |= ndev->features;

	if (bdx_read_mac(priv)) {
		dev_err(&pdev->dev, "load MAC address failed\n");
		goto err_out_iomap;
	}
	SET_NETDEV_DEV(ndev, &pdev->dev);
	if ((err = register_netdev(ndev))) {
		dev_err(&pdev->dev, "register_netdev failed\n");
		goto err_out_free;
	}
	bdx_reset(priv);

	/*Set GPIO[9:0] to output 0 */

	WRITE_REG(priv, 0x51E0, 0x30010006);	/* GPIO_OE_ WR CMD */
	WRITE_REG(priv, 0x51F0, 0x0);	/* GPIO_OE_ DATA */

	WRITE_REG(priv, regMDIO_CMD_STAT, 0x3ec8);
	if (netif_running(ndev))
		netif_stop_queue(priv->ndev);

	if ((err = bdx_tx_init(priv)))
		goto err_out_free;

	if ((err = bdx_fw_load(priv)))
		goto err_out_free;

	bdx_tx_free(priv);

	netif_carrier_off(ndev);
	netif_stop_queue(ndev);

	print_eth_id(ndev);

	bdx_scan_pci();
#ifdef TN40_MEMLOG
	memLogInit();
#endif

	return 0;
err_out_free:
	unregister_netdev(ndev);
	free_netdev(ndev);
err_out_iomap:
	iounmap(nic->regs);
err_out_res:
	pci_release_regions(pdev);
err_dma:
	if (pci_dev_msi_enabled(pdev)) {
		pci_disable_msi(pdev);
	}
	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);
err_pci:
	vfree(nic);

	return err;

}

/****************** Ethtool interface *********************/
/* Get strings for tests */
static const char
 bdx_test_names[][ETH_GSTRING_LEN] = {
	"No tests defined"
};

/* Get strings for statistics counters */
static const char
 bdx_stat_names[][ETH_GSTRING_LEN] = {
	"InUCast",		/* 0x7200 */
	"InMCast",		/* 0x7210 */
	"InBCast",		/* 0x7220 */
	"InPkts",		/* 0x7230 */
	"InErrors",		/* 0x7240 */
	"InDropped",		/* 0x7250 */
	"FrameTooLong",		/* 0x7260 */
	"FrameSequenceErrors",	/* 0x7270 */
	"InVLAN",		/* 0x7280 */
	"InDroppedDFE",		/* 0x7290 */
	"InDroppedIntFull",	/* 0x72A0 */
	"InFrameAlignErrors",	/* 0x72B0 */

	/* 0x72C0-0x72E0 RSRV */

	"OutUCast",		/* 0x72F0 */
	"OutMCast",		/* 0x7300 */
	"OutBCast",		/* 0x7310 */
	"OutPkts",		/* 0x7320 */

	/* 0x7330-0x7360 RSRV */

	"OutVLAN",		/* 0x7370 */
	"InUCastOctects",	/* 0x7380 */
	"OutUCastOctects",	/* 0x7390 */

	/* 0x73A0-0x73B0 RSRV */

	"InBCastOctects",	/* 0x73C0 */
	"OutBCastOctects",	/* 0x73D0 */
	"InOctects",		/* 0x73E0 */
	"OutOctects",		/* 0x73F0 */
};

int bdx_get_link_ksettings(struct net_device *netdev,
			   struct ethtool_link_ksettings *cmd)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	bitmap_zero(cmd->link_modes.supported, __ETHTOOL_LINK_MODE_MASK_NBITS);
	bitmap_zero(cmd->link_modes.advertising,
		    __ETHTOOL_LINK_MODE_MASK_NBITS);
	bitmap_zero(cmd->link_modes.lp_advertising,
		    __ETHTOOL_LINK_MODE_MASK_NBITS);
	cmd->base.speed = priv->link_speed;
	cmd->base.duplex = DUPLEX_FULL;
	cmd->base.phy_address = priv->phy_mdio_port;
	cmd->base.mdio_support = ETH_MDIO_SUPPORTS_C22 | ETH_MDIO_SUPPORTS_C45;

	return priv->phy_ops.get_link_ksettings(netdev, cmd);

}

int bdx_set_link_ksettings(struct net_device *netdev,
			   const struct ethtool_link_ksettings *cmd)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	return priv->phy_ops.set_link_ksettings(netdev, cmd);

}

/*
 * bdx_get_drvinfo - Report driver information
 *
 * @netdev
 * @drvinfo
 */
static void
bdx_get_drvinfo(struct net_device *netdev, struct ethtool_drvinfo *drvinfo)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	strlcpy(drvinfo->driver, BDX_DRV_NAME, sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, BDX_DRV_VERSION, sizeof(drvinfo->version));
	strlcpy(drvinfo->fw_version, "N/A", sizeof(drvinfo->fw_version));
	strlcpy(drvinfo->bus_info, pci_name(priv->pdev),
		sizeof(drvinfo->bus_info));

	drvinfo->n_stats =
	    ((priv->stats_flag) ? ARRAY_SIZE(bdx_stat_names) : 0);
	drvinfo->testinfo_len = 0;
	drvinfo->regdump_len = 0;
	drvinfo->eedump_len = 0;
}

/*
 * bdx_get_coalesce - Get interrupt coalescing parameters.
 *
 * @netdev
 * @ecoal
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
static int
bdx_get_coalesce(struct net_device *netdev,
		 struct ethtool_coalesce *ecoal,
		 struct kernel_ethtool_coalesce *kernel_ecoal,
		 struct netlink_ext_ack *netext_ack)
#else
static int
bdx_get_coalesce(struct net_device *netdev, struct ethtool_coalesce *ecoal)
#endif
{
	u32 rdintcm;
	u32 tdintcm;
	struct bdx_priv *priv = netdev_priv(netdev);

	rdintcm = priv->rdintcm;
	tdintcm = priv->tdintcm;

	/*
	 * PCK_TH is measured in multiples of FIFO bytes. We translate
	 *  it to packets
	 */
	ecoal->rx_coalesce_usecs = GET_INT_COAL(rdintcm) * INT_COAL_MULT;
	ecoal->rx_max_coalesced_frames = ((GET_PCK_TH(rdintcm) * PCK_TH_MULT) /
					  sizeof(struct rxf_desc));

	ecoal->tx_coalesce_usecs = GET_INT_COAL(tdintcm) * INT_COAL_MULT;
	ecoal->tx_max_coalesced_frames = ((GET_PCK_TH(tdintcm) * PCK_TH_MULT) /
					  BDX_TXF_DESC_SZ);

	/* Adaptive parameters are ignored */
	return 0;
}

/*
 * bdx_set_coalesce - Set interrupt coalescing parameters.
 *
 * @netdev
 * @ecoal
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
static int
bdx_set_coalesce(struct net_device *netdev,
		 struct ethtool_coalesce *ecoal,
		 struct kernel_ethtool_coalesce *kernel_ecoal,
		 struct netlink_ext_ack *netext_ack)
#else
static int
bdx_set_coalesce(struct net_device *netdev, struct ethtool_coalesce *ecoal)
#endif
{
	u32 rdintcm;
	u32 tdintcm;
	struct bdx_priv *priv = netdev_priv(netdev);
	int rx_coal;
	int tx_coal;
	int rx_max_coal;
	int tx_max_coal;

	/* Check for valid input */
	rx_coal = ecoal->rx_coalesce_usecs / INT_COAL_MULT;
	tx_coal = ecoal->tx_coalesce_usecs / INT_COAL_MULT;
	rx_max_coal = ecoal->rx_max_coalesced_frames;
	tx_max_coal = ecoal->tx_max_coalesced_frames;

	/* Translate from packets to multiples of FIFO bytes */
	rx_max_coal = (((rx_max_coal * sizeof(struct rxf_desc)) +
			PCK_TH_MULT - 1) / PCK_TH_MULT);
	tx_max_coal = (((tx_max_coal * BDX_TXF_DESC_SZ) + PCK_TH_MULT - 1) /
		       PCK_TH_MULT);

	if ((rx_coal > 0x7FFF) ||
	    (tx_coal > 0x7FFF) || (rx_max_coal > 0xF) || (tx_max_coal > 0xF))
		return -EINVAL;

	rdintcm = INT_REG_VAL(rx_coal, GET_INT_COAL_RC(priv->rdintcm),
			      GET_RXF_TH(priv->rdintcm), rx_max_coal);
	tdintcm = INT_REG_VAL(tx_coal, GET_INT_COAL_RC(priv->tdintcm), 0,
			      tx_max_coal);

	priv->rdintcm = rdintcm;
	priv->tdintcm = tdintcm;

	WRITE_REG(priv, regRDINTCM0, rdintcm);
	WRITE_REG(priv, regTDINTCM0, tdintcm);

	return 0;
}

/* Convert RX fifo size to number of pending packets */

static inline int bdx_rx_fifo_size_to_packets(int rx_size)
{
	return ((FIFO_SIZE * (1 << rx_size)) / sizeof(struct rxf_desc));
}

/* Convert TX fifo size to number of pending packets */

static inline int bdx_tx_fifo_size_to_packets(int tx_size)
{
	return ((FIFO_SIZE * (1 << tx_size)) / BDX_TXF_DESC_SZ);
}

/*
 * bdx_get_ringparam - Report ring sizes.
 *
 * @netdev
 * @ring
 * @kernel_ering
 * @extack
 */
static void
bdx_get_ringparam(struct net_device *netdev,
		  struct ethtool_ringparam *ring,
		  struct kernel_ethtool_ringparam *kernel_ering,
		  struct netlink_ext_ack *extack)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	/*max_pending - The maximum-sized FIFO we allow */
	ring->rx_max_pending = bdx_rx_fifo_size_to_packets(3);
	ring->tx_max_pending = bdx_tx_fifo_size_to_packets(3);
	ring->rx_pending = bdx_rx_fifo_size_to_packets(priv->rxf_size);
	ring->tx_pending = bdx_tx_fifo_size_to_packets(priv->txd_size);
}

/*
 * bdx_set_ringparam - Set ring sizes.
 *
 * @netdev
 * @ring
 */
static int
bdx_set_ringparam(struct net_device *netdev,
		  struct ethtool_ringparam *ring,
		  struct kernel_ethtool_ringparam *kernel_ering,
		  struct netlink_ext_ack *extack)
{
	struct bdx_priv *priv = netdev_priv(netdev);
	int rx_size = 0;
	int tx_size = 0;

	for (; rx_size < 4; rx_size++) {
		if (bdx_rx_fifo_size_to_packets(rx_size) >= ring->rx_pending)
			break;
	}
	if (rx_size == 4)
		rx_size = 3;

	for (; tx_size < 4; tx_size++) {
		if (bdx_tx_fifo_size_to_packets(tx_size) >= ring->tx_pending)
			break;
	}
	if (tx_size == 4)
		tx_size = 3;

	/* Is there anything to do? */
	if ((rx_size == priv->rxf_size) && (tx_size == priv->txd_size))
		return 0;

	priv->rxf_size = rx_size;
	if (rx_size > 1)
		priv->rxd_size = rx_size - 1;
	else
		priv->rxd_size = rx_size;

	priv->txf_size = priv->txd_size = tx_size;

	if (netif_running(netdev)) {
		bdx_close(netdev);
		bdx_open(netdev);
	}
	return 0;
}

/*
 * bdx_get_strings - Return a set of strings that describe the requested
 *           objects.
 * @netdev
 * @data
 */
static void bdx_get_strings(struct net_device *netdev, u32 stringset, u8 *data)
{
	switch (stringset) {
	case ETH_SS_TEST:
		memcpy(data, *bdx_test_names, sizeof(bdx_test_names));
		break;

	case ETH_SS_STATS:
		memcpy(data, *bdx_stat_names, sizeof(bdx_stat_names));
		break;
	}
}

/*
 * bdx_get_sset_count - Return the number of statistics or tests.
 *
 * @netdev
 */
static int bdx_get_sset_count(struct net_device *netdev, int stringset)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	switch (stringset) {
	case ETH_SS_STATS:
		BDX_ASSERT(ARRAY_SIZE(bdx_stat_names) !=
			   sizeof(struct bdx_stats) / sizeof(u64));
		return ((priv->stats_flag) ? ARRAY_SIZE(bdx_stat_names) : 0);
	default:
		return -EINVAL;
	}
}

/*
 * bdx_get_ethtool_stats - Return device's hardware L2 statistics.
 *
 * @netdev
 * @stats
 * @data
 */
static void bdx_get_ethtool_stats(struct net_device *netdev,
				  struct ethtool_stats *stats, u64 *data)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	if (priv->stats_flag) {

		/* Update stats from HW */
		bdx_update_stats(priv);

		/* Copy data to user buffer */
		memcpy(data, &priv->hw_stats, sizeof(priv->hw_stats));
	}
}

/* Blink LED's for finding board */

static int bdx_set_phys_id(struct net_device *netdev,
			   enum ethtool_phys_id_state state)
{
	struct bdx_priv *priv = netdev_priv(netdev);
	int rval = 0;

	switch (state) {
	case ETHTOOL_ID_ACTIVE:
		priv->phy_ops.ledset(priv, PHY_LEDS_SAVE);
		rval = 1;
		break;

	case ETHTOOL_ID_INACTIVE:
		priv->phy_ops.ledset(priv, PHY_LEDS_RESTORE);
		rval = 1;
		break;

	case ETHTOOL_ID_ON:
		priv->phy_ops.ledset(priv, PHY_LEDS_ON);
		break;

	case ETHTOOL_ID_OFF:
		priv->phy_ops.ledset(priv, PHY_LEDS_OFF);
		break;
	}
	return rval;
}

#ifdef _EEE_
#ifdef ETHTOOL_GEEE

/*
 * bdx_get_eee - Get device-specific EEE settings
 *
 * @netdev
 * @ecmd
 */
static int bdx_get_eee(struct net_device *netdev, struct ethtool_eee *edata)
{
	int err;
	struct bdx_priv *priv = netdev_priv(netdev);

	if (priv->phy_ops.get_eee == NULL) {
		netdev_err(netdev, "EEE Functionality is not supported\n");
		err = -EOPNOTSUPP;
	} else {
		err = priv->phy_ops.get_eee(netdev, edata);
	}

	return err;

}
#endif
#ifdef ETHTOOL_SEEE

/*
 * bdx_set_eee - set device-specific EEE settings
 *
 * @netdev
 * @ecmd
 */
static int bdx_set_eee(struct net_device *netdev, struct ethtool_eee *edata)
{
	int err;
	struct bdx_priv *priv = netdev_priv(netdev);

	if (priv->phy_ops.set_eee == NULL) {
		netdev_err(netdev, "EEE Functionality is not supported\n");
		err = -EOPNOTSUPP;
	} else {
		netdev_info(netdev, "Setting EEE\n");
		err = priv->phy_ops.set_eee(priv);
	}
	return err;

}
#endif
#endif

#if (defined(TN40_PTP) && defined(ETHTOOL_GET_TS_INFO))

static int bdx_get_ts_info(struct net_device *netdev,
			   struct ethtool_ts_info *info)
{
	info->so_timestamping |= SOF_TIMESTAMPING_SOFTWARE |
	    SOF_TIMESTAMPING_TX_SOFTWARE | SOF_TIMESTAMPING_RX_SOFTWARE;

	return 0;

}

#endif

/*
 * bdx_ethtool_ops - Ethtool interface implementation.
 *
 * @netdev
 */
static void bdx_ethtool_ops(struct net_device *netdev)
{

	static struct ethtool_ops bdx_ethtool_ops = {
		.get_link_ksettings = bdx_get_link_ksettings,
		.set_link_ksettings = bdx_set_link_ksettings,
		.get_drvinfo = bdx_get_drvinfo,
		.get_link = ethtool_op_get_link,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
		.supported_coalesce_params = ETHTOOL_COALESCE_USECS,
#endif
		.get_coalesce = bdx_get_coalesce,
		.set_coalesce = bdx_set_coalesce,
		.get_ringparam = bdx_get_ringparam,
		.set_ringparam = bdx_set_ringparam,
#if defined(_EEE_)
#ifdef ETHTOOL_GEEE
		.get_eee = bdx_get_eee,
#endif
#ifdef ETHTOOL_SEEE
		.set_eee = bdx_set_eee,
#endif
#endif
		.get_strings = bdx_get_strings,
		.get_sset_count = bdx_get_sset_count,
		.get_ethtool_stats = bdx_get_ethtool_stats,
		.set_phys_id = bdx_set_phys_id,
#if (defined(TN40_PTP) && defined(ETHTOOL_GET_TS_INFO))
		.get_ts_info = bdx_get_ts_info,
#endif
	};

#define SET_ETHTOOL_OPS(netdev, ops) ((netdev)->ethtool_ops = (ops))
	SET_ETHTOOL_OPS(netdev, &bdx_ethtool_ops);
}

/**
 * bdx_remove - Device removal routine.
 *
 * @pdev - PCI device information struct
 *
 * bdx_remove is called by the PCI subsystem to notify the driver
 * that it should release a PCI device.  This could be caused by a
 * Hot-Plug event, or because the driver is going to be removed from
 * memory.
 */
static void __exit bdx_remove(struct pci_dev *pdev)
{
	struct pci_nic *nic = pci_get_drvdata(pdev);
	struct net_device *ndev = nic->priv->ndev;
	struct bdx_priv *priv = nic->priv;

	bdx_hw_stop(priv);
	bdx_sw_reset(priv);
	bdx_rx_free(priv);
	bdx_tx_free(priv);
	unregister_netdev(ndev);
	free_netdev(ndev);
	iounmap(nic->regs);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);
	vfree(nic);
#ifdef _DRIVER_RESUME_
	spin_lock(&g_lock);
	g_ndevices_loaded -= 1;
	spin_unlock(&g_lock);
#endif
	pr_info("Device removed\n");

}

#ifdef _DRIVER_RESUME_

#define PCI_PMCR 0x7C

static int bdx_suspend(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct pci_nic *nic = pci_get_drvdata(pdev);
	struct bdx_priv *priv = nic->priv;
	int err;

	netdev_info(priv->ndev, "System suspend\n");
	bdx_stop(priv);
	err = pci_save_state(pdev);
	netdev_dbg(priv->ndev, "pci_save_state = %d\n", err);
	err = pci_prepare_to_sleep(pdev);
	netdev_dbg(priv->ndev, "pci_prepare_to_sleep = %d\n", err);
	return 0;
}

static int bdx_resume(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct pci_nic *nic = pci_get_drvdata(pdev);
	struct bdx_priv *priv = nic->priv;
	int rc = -1;

	netdev_info(priv->ndev, "System resume\n");
	do {
		pci_restore_state(pdev);
		rc = pci_save_state(pdev);
		netdev_dbg(priv->ndev, "pci_save_state = %d\n", rc);
		setMDIOSpeed(priv, priv->phy_ops.mdio_speed);
		if (priv->phy_ops.mdio_reset(priv, priv->phy_mdio_port,
					     priv->phy_type) != 0) {
			netdev_err(priv->ndev,
				   "bdx_resume() failed to load PHY");
			break;
		}
		if (bdx_reset(priv) != 0) {
			netdev_err(priv->ndev,
				   "bdx_resume() bdx_reset failed\n");
			break;
		}
		WRITE_REG(priv, regMDIO_CMD_STAT, 0x3ec8);
		if (bdx_start(priv, FW_LOAD)) {
			break;
		}
		rc = 0;
	} while (0);

	return rc;

}
#endif

#ifdef _DRIVER_RESUME_
__refdata static struct dev_pm_ops bdx_pm_ops = {
	.suspend = bdx_suspend,
	.resume_noirq = bdx_resume,
	.freeze = bdx_suspend,
	.restore_noirq = bdx_resume,
};
#endif
__refdata static struct pci_driver bdx_pci_driver = {
	.name = BDX_DRV_NAME,
	.id_table = bdx_pci_tbl,
	.probe = bdx_probe,
	.remove = __exit_p(bdx_remove),
	.shutdown = __exit_p(bdx_remove),
#ifdef _DRIVER_RESUME_
	.driver.pm = &bdx_pm_ops,
#endif

};

#ifndef _DRIVER_RESUME_

int bdx_no_hotplug(struct pci_dev *pdev, const struct pci_device_id *ent)
{

	dev_err
	    (&pdev->dev,
	     "rescan/hotplug is *NOT* supported!, please use rmmod/insmod instead\n");
	return -1;

}

#endif

static void __init bdx_scan_pci(void)
{
	int j, nDevices = 0, nLoaded;
	struct pci_dev *dev = NULL;

	/* count our devices */
	for (j = 0; j < sizeof(bdx_pci_tbl) / sizeof(struct pci_device_id); j++) {
		while ((dev =
			pci_get_subsys(bdx_pci_tbl[j].vendor,
				       bdx_pci_tbl[j].device,
				       bdx_pci_tbl[j].subvendor,
				       bdx_pci_tbl[j].subdevice, dev))) {
			nDevices += 1;
			dev_info(&dev->dev, "%d %04x:%04x:%04x:%04x\n",
				 nDevices, bdx_pci_tbl[j].vendor,
				 bdx_pci_tbl[j].device,
				 bdx_pci_tbl[j].subvendor,
				 bdx_pci_tbl[j].subdevice);
			if (nDevices > 20) {
				pr_err("to many devices detected ?!\n");
				break;
			}
		}

	}
	spin_lock(&g_lock);
	g_ndevices = nDevices;
	g_ndevices_loaded += 1;
	nLoaded = g_ndevices_loaded;
#ifndef _DRIVER_RESUME_
	if (g_ndevices_loaded >= g_ndevices) {	/* all loaded */
		bdx_pci_driver.probe = bdx_no_hotplug;
	}
#endif
	spin_unlock(&g_lock);
	pr_info("detected %d cards, %d loaded\n", nDevices, nLoaded);

}

static void bdx_print_phys(void)
{
	pr_info("Supported phys : %s %s %s %s %s %s %s %s\n",
#ifdef PHY_MV88X3120
		"MV88X3120",
#else
		"",
#endif
#ifdef PHY_MV88X3310
		"MV88X3310",
#else
		"",
#endif
#ifdef PHY_MV88E2010
		"MV88E2010",
#else
		"",
#endif
#ifdef PHY_QT2025
		"QT2025",
#else
		"",
#endif
#ifdef PHY_TLK10232
		"TLK10232",
#else
		"",
#endif
#ifdef PHY_AQR105
		"AQR105",
#else
		"",
#endif
#ifdef PHY_MUSTANG
		"MUSTANG",
#else
		"",
#endif
		bdx_pci_driver.suspend != NULL ? "in driver suspend" : "");

}

/*
 * print_driver_id - Print the driver build parameters the .
 */
static void __init print_driver_id(void)
{
	pr_info("%s, %s\n", BDX_DRV_DESC, BDX_DRV_VERSION);
	bdx_print_phys();
}

static int __init bdx_module_init(void)
{

#ifdef __BIG_ENDIAN
	bdx_firmware_endianess();
#endif
	traceInit();
	init_txd_sizes();
	print_driver_id();
	return pci_register_driver(&bdx_pci_driver);
}

module_init(bdx_module_init);

static void __exit bdx_module_exit(void)
{

	pci_unregister_driver(&bdx_pci_driver);
	pr_info("Driver unloaded\n");

}

module_exit(bdx_module_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION(BDX_DRV_VERSION);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(BDX_DRV_DESC);
