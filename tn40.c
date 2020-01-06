/*******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "tn40.h"
#ifdef VM_KLNX
#include "tn40_vmware.h"
#endif
#include "tn40_fw.h"

static void bdx_scan_pci(void);

uint bdx_force_no_phy_mode = 0;
module_param_named(no_phy, bdx_force_no_phy_mode, int, 0644);
MODULE_PARM_DESC(bdx_force_no_phy_mode, "no_phy=1 - force no phy mode (CX4)");

__initdata static u32 g_ndevices = 0;
__initdata static u32 g_ndevices_loaded = 0;
__initdata spinlock_t g_lock __initdata;
__initdata DEFINE_SPINLOCK(g_lock) ;

#define LDEV(_vid,_pid,_subdev,_msi,_ports,_phya,_phyb,_name)  \
    {_vid,_pid,_subdev,_msi,_ports,PHY_TYPE_##_phya,PHY_TYPE_##_phyb}
static struct bdx_device_descr  bdx_dev_tbl[] =
{
    LDEV(TEHUTI_VID,0x4010,0x4010,1,1,CX4,NA,		"TN4010 Clean SROM"),
    LDEV(TEHUTI_VID,0x4020,0x3015,1,1,CX4,NA,		"TN9030 10GbE CX4 Ethernet Adapter"),
#ifdef PHY_MUSTANG
    LDEV(TEHUTI_VID,0x4020,0x2040,1,1,CX4,NA,		"Mustang-200 10GbE Ethernet Adapter"),
#endif
#ifdef PHY_QT2025
    LDEV(TEHUTI_VID,0x4022,0x3015,1,1,QT2025,NA,	"TN9310 10GbE SFP+ Ethernet Adapter"),
    LDEV(TEHUTI_VID,0x4022,0x4d00,1,1,QT2025,NA,	"D-Link DXE-810S 10GbE SFP+ Ethernet Adapter"),
    LDEV(TEHUTI_VID,0x4022,0x8709,1,1,QT2025,NA,	"ASUS XG-C100F 10GbE SFP+ Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4022,0x8103,1,1,QT2025,NA,	"Edimax 10 Gigabit Ethernet SFP+ PCI Express Adapter"),
#endif
#ifdef PHY_MV88X3120
	LDEV(TEHUTI_VID,0x4024,0x3015,1,1,MV88X3120,NA,	"TN9210 10GBase-T Ethernet Adapter"),
#endif
#ifdef PHY_MV88X3310
	LDEV(TEHUTI_VID,0x4027,0x3015,1,1,MV88X3310,NA,	"TN9710P 10GBase-T/NBASE-T Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4027,0x8104,1,1,MV88X3310,NA,	"Edimax 10 Gigabit Ethernet PCI Express Adapter"),
	LDEV(TEHUTI_VID,0x4027,0x0368,1,1,MV88X3310,NA,	"Buffalo LGY-PCIE-MG Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4027,0x1546,1,1,MV88X3310,NA,	"IOI GE10-PCIE4XG202P 10Gbase-T/NBASE-T Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4027,0x1001,1,1,MV88X3310,NA,	"LR-Link LREC6860BT 10 Gigabit Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4027,0x3310,1,1,MV88X3310,NA,	"QNAP PCIe Expansion Card"),
#endif
#ifdef PHY_MV88E2010
	LDEV(TEHUTI_VID,0x4527,0x3015,1,1,MV88E2010,NA,	"TN9710Q 5GBase-T/NBASE-T Ethernet Adapter"),
#endif
#ifdef PHY_TLK10232
	LDEV(TEHUTI_VID,0x4026,0x3015,1,1,TLK10232,NA,	"TN9610 10GbE SFP+ Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4026,0x1000,1,1,TLK10232,NA,	"LR-Link LREC6860AF 10 Gigabit Ethernet Adapter"),
#endif
#ifdef PHY_AQR105
	LDEV(TEHUTI_VID,0x4025,0x2900,1,1,AQR105,NA,	"D-Link DXE-810T 10GBase-T Ethernet Adapter"),
    LDEV(TEHUTI_VID,0x4025,0x3015,1,1,AQR105,NA,	"TN9510 10GBase-T/NBASE-T Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4025,0x8102,1,1,AQR105,NA,	"Edimax 10 Gigabit Ethernet PCI Express Adapter"),
#endif
	{0}
};

static struct pci_device_id bdx_pci_tbl[] =
{
    {TEHUTI_VID, 0x4010, TEHUTI_VID, 0x4010, 0, 0, 0},
    {TEHUTI_VID, 0x4020, TEHUTI_VID, 0x3015, 0, 0, 0},
#ifdef PHY_MUSTANG
	{TEHUTI_VID, 0x4020, 0x180C, 	 0x2040, 0, 0, 0},
#endif
#ifdef PHY_QT2025
    {TEHUTI_VID, 0x4022, TEHUTI_VID, 0x3015, 0, 0, 0},
	{TEHUTI_VID, 0x4022, DLINK_VID,  0x4d00, 0, 0, 0},
	{TEHUTI_VID, 0x4022, ASUS_VID, 	 0x8709, 0, 0, 0},
	{TEHUTI_VID, 0x4022, EDIMAX_VID, 0x8103, 0, 0, 0},
#endif
#ifdef PHY_MV88X3120
    {TEHUTI_VID, 0x4024, TEHUTI_VID, 0x3015, 0, 0, 0},
#endif
#ifdef PHY_MV88X3310
    {TEHUTI_VID, 0x4027, TEHUTI_VID, 0x3015, 0, 0, 0},
	{TEHUTI_VID, 0x4027, EDIMAX_VID, 0x8104, 0, 0, 0},
	{TEHUTI_VID, 0x4027, BUFFALO_VID,0x0368, 0, 0, 0},
	{TEHUTI_VID, 0x4027, 0x1546, 	 0x4027, 0, 0, 0},
	{TEHUTI_VID, 0x4027, 0x4C52, 	 0x1001, 0, 0, 0},
	{TEHUTI_VID, 0x4027, 0x1BAA, 	 0x3310, 0, 0, 0},
#endif
#ifdef PHY_MV88E2010
    {TEHUTI_VID, 0x4527, TEHUTI_VID, 0x3015, 0, 0, 0},
#endif
#ifdef PHY_TLK10232
    {TEHUTI_VID, 0x4026, TEHUTI_VID, 0x3015, 0, 0, 0},
    {TEHUTI_VID, 0x4026, 0x4C52,     0x1000, 0, 0, 0},
#endif
#ifdef PHY_AQR105
    {TEHUTI_VID, 0x4025, DLINK_VID,  0x2900, 0, 0, 0},
    {TEHUTI_VID, 0x4025, TEHUTI_VID, 0x3015, 0, 0, 0},
	{TEHUTI_VID, 0x4025, EDIMAX_VID, 0x8102, 0, 0, 0},
#endif
    {0}
};

MODULE_DEVICE_TABLE(pci, bdx_pci_tbl);

/* Definitions needed by ISR or NAPI functions */
static void bdx_rx_alloc_buffers(struct bdx_priv *priv);
static void bdx_tx_cleanup(struct bdx_priv *priv);
static int bdx_rx_receive(struct bdx_priv *priv, struct rxd_fifo *f, int budget);
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
//static int bdx_suspend(struct pci_dev *pdev, pm_message_t state);
//static int bdx_resume (struct pci_dev *pdev);
static int bdx_suspend(struct device *dev);
static int bdx_resume(struct device *dev);
#endif
//#define USE_RSS
#if defined(USE_RSS)
/* bdx_init_rss - Initialize RSS hash HW function.
 *
 * @priv       - NIC private structure
 */
#if defined(RHEL_RELEASE_CODE)
#define prandom_seed(seed)
#define prandom_u32 random32
#endif
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
    seed = (uint32_t)(0xFFFFFFFF & jiffies);
    prandom_seed(seed);
    for (i =0; i < 4 * RSS_HASH_LEN; i += 4)
    {
    	u32 rnd = prandom_u32();
        WRITE_REG(priv, regRSS_HASH_BASE + 4 * i, rnd);
        DBG("bdx_init_rss() rnd 0x%x\n", rnd);
    }
    WRITE_REG(priv, regRSS_CNG,
              RSS_ENABLED   | RSS_HFT_TOEPLITZ  |
              RSS_HASH_IPV4 | RSS_HASH_TCP_IPV4 |
              RSS_HASH_IPV6 | RSS_HASH_TCP_IPV6);

    DBG("regRSS_CNG =%x\n", READ_REG(priv, regRSS_CNG));
    RET(0);
}
#else
#define    bdx_init_rss(priv)
#endif

#if defined(TN40_DEBUG)
int g_dbg=0;
#endif
#if defined(TN40_FTRACE)
int g_ftrace=0;
#endif
#if defined(TN40_REGLOG)
int g_regLog=0;
#endif
#if defined (TN40_MEMLOG)
int g_memLog=0;
#endif

#if defined(TN40_DEBUG)

//-------------------------------------------------------------------------------------------------
/*
static void dbg_irqActions(struct pci_dev *pdev)
{
    struct msi_desc *entry;
DBG_ON;
	list_for_each_entry(entry, &pdev->msi_list, list)
    {
			int i, j=0, nvec;
			if (!entry->irq)
			{
				continue;
			}
			if (entry->nvec_used)
			{
				nvec = entry->nvec_used;
			}
			else
			{
				nvec = 1 << entry->msi_attrib.multiple;
			}
			for (i = 0; i < nvec; i++)
			{
				//BUG_ON(irq_has_action(entry->irq + i));
				if (irq_has_action(entry->irq + i))
				{
					DBG("action on irq %d\n", entry->irq + i);
					j += 1;
				}
			}
			DBG("irq %d nvec %d found %d\n", entry->irq, nvec, j);
	}
DBG_OFF;

} //dbg_irqActions()

*/
//-------------------------------------------------------------------------------------------------

void dbg_printFifo(struct fifo *m, char *fName)
{
	DBG("%s fifo:\n", fName);
	DBG("WPTR 0x%x = 0x%x RPTR 0x%x = 0x%x\n",
    	m->reg_WPTR, m->wptr, m->reg_RPTR, m->rptr);

} // dbg_printFifo()

//-------------------------------------------------------------------------------------------------

void dbg_printRegs(struct bdx_priv *priv, char *msg)
{
	ENTER;
	DBG("* %s * \n", msg);
	DBG("~~~~~~~~~~~~~\n");
	DBG("veneto:");
	DBG("pc = 0x%x li = 0x%x ic = %d\n", READ_REG(priv, 0x2300), READ_REG(priv, 0x2310), READ_REG(priv, 0x2320));
	dbg_printFifo(&priv->txd_fifo0.m, (char *)"TXD");
	dbg_printFifo(&priv->rxf_fifo0.m, (char *)"RXF");
	dbg_printFifo(&priv->rxd_fifo0.m, (char *)"RXD");
	DBG("~~~~~~~~~~~~~\n");

	EXIT;

} // dbg_printRegs()

//-------------------------------------------------------------------------------------------------

void dbg_printPBL(struct pbl *pbl)
{
	DBG("pbl: len %u pa_lo 0x%x pa_hi 0x%x\n", pbl->len, pbl->pa_lo, pbl->pa_hi);

} // dbg_printPBL()

//-------------------------------------------------------------------------------------------------

void dbg_printPkt(char *pkt, u16 len)
{
	int 	i;

	MSG("RX: len=%d\n",len);
	for(i=0; i<len; i=i+16) ERR("%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x ",(0xff&pkt[i]),(0xff&pkt[i+1]),(0xff&pkt[i+2]),(0xff&pkt[i+3]),(0xff&pkt[i+4]),(0xff&pkt[i+5]),(0xff&pkt[i+6]),(0xff&pkt[i+7]),(0xff&pkt[i+8]),(0xff&pkt[i+9]),(0xff&pkt[i+10]),(0xff&pkt[i+11]),(0xff&pkt[i+12]),(0xff&pkt[i+13]),(0xff&pkt[i+14]),(0xff&pkt[i+15]));
	MSG("\n");

} // dbg_printPkt

//-------------------------------------------------------------------------------------------------

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
} // dbg_printSkb

//-------------------------------------------------------------------------------------------------

void dbg_printIoctl(void)
{
	MSG("DBG_ON %d, DBG_OFF %d, DBG_SUSPEND %d, DBG_RESUME %d DBG_PRINT_PAGE_TABLE %d\n",
		DBG_START_DBG, DBG_STOP_DBG, DBG_SUSPEND, DBG_RESUME, DBG_PRINT_PAGE_TABLE);

} // dbg_printIoctl

//-------------------------------------------------------------------------------------------------
#else
#define dbg_printRegs(priv, msg)
#define dbg_printPBL(pbl)
#define dbg_printFifo(m, fName)
#define dbg_printPkt(pkt)
#define dbg_printIoctl()
#endif

#if defined(FTRACE)
int g_ftrace = 0;
#endif

//-------------------------------------------------------------------------------------------------

#ifdef TN40_THUNDERBOLT

u32 tbReadReg(struct bdx_priv *priv, u32 reg)
{
	u32 rVal;

	if (!priv->bDeviceRemoved)
	{
		rVal = readl(priv->pBdxRegs + reg);
		if ( rVal == 0xFFFFFFFF )
		{
			priv->bDeviceRemoved = 1;
		}
	}
	else
	{
		rVal = 0xFFFFFFFF;
	}

	return rVal;

} // tbReadReg()

#endif

//-------------------------------------------------------------------------------------------------
#ifdef REGLOG
int g_regLog = 0;
u32  bdx_readl(struct bdx_priv *priv, u32 reg)
{

    u32 val;
#ifdef TN40_THUNDERBOLT
    val = tbReadReg(priv, reg);
#else
    val=readl(priv->pBdxRegs + reg);
#endif
    if (g_regLog)
    {
    	MSG("regR 0x%x = 0x%x\n",(u32)(((u64)reg)&0xFFFF) ,val);
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
	void __iomem 	*regs=priv->pBdxRegs;

#define BDX_MAX_MDIO_BUSY_LOOPS 1024
    int tries = 0;
    while (++tries < BDX_MAX_MDIO_BUSY_LOOPS)
    {
        u32 mdio_cmd_stat = readl(regs + regMDIO_CMD_STAT);

        if (GET_MDIO_BUSY(mdio_cmd_stat)== 0)
        {
            //          ERR("mdio_get success after %d tries (val=%u 0x%x)\n", tries, mdio_cmd_stat, mdio_cmd_stat);
            return mdio_cmd_stat;
        }
    }
    ERR("MDIO busy!\n");
    return 0xFFFFFFFF;

} // bdx_mdio_get()

//-------------------------------------------------------------------------------------------------

/* bdx_mdio_read - read a 16bit word through the MDIO interface
 * @priv
 * @device   - 5 bit device id
 * @port     - 5 bit port id
 * @addr     - 16 bit address
 * returns a 16bit value or -1 for failure
 */
int bdx_mdio_read(struct bdx_priv *priv, int device, int port, u16 addr)
{
    void __iomem 	*regs=priv->pBdxRegs;
    u32 tmp_reg, i;
    /* Wait until MDIO is not busy */
    if (bdx_mdio_get(priv)== 0xFFFFFFFF)
    {
        return -1;
    }
    i=( (device&0x1F) | ((port&0x1F)<<5) );
	writel(i, regs + regMDIO_CMD);
	writel((u32)addr, regs + regMDIO_ADDR);
	if ((tmp_reg = bdx_mdio_get(priv)) == 0xFFFFFFFF)
	{
		ERR("MDIO busy after read command\n");
		return -1;
	}
		writel( ((1<<15) |i) , regs + regMDIO_CMD);
    /* Read CMD_STAT until not busy */
    if ((tmp_reg = bdx_mdio_get(priv)) == 0xFFFFFFFF)
    {
        ERR("MDIO busy after read command\n");
        return -1;
    }
    if (GET_MDIO_RD_ERR(tmp_reg))
    {
        DBG("MDIO error after read command\n");
        return -1;
    }
    tmp_reg = readl(regs + regMDIO_DATA);
    //  ERR("MDIO_READ: MDIO_DATA =0x%x \n", (tmp_reg & 0xFFFF));
    return (int)(tmp_reg & 0xFFFF);

} // bdx_mdio_read()

//-------------------------------------------------------------------------------------------------

/* bdx_mdio_write - writes a 16bit word through the MDIO interface
 * @priv
 * @device    - 5 bit device id
 * @port      - 5 bit port id
 * @addr      - 16 bit address
 * @data      - 16 bit value
 * returns 0 for success or -1 for failure
 */
int bdx_mdio_write(struct bdx_priv *priv, int device, int port, u16 addr, u16 data)
{
	void __iomem 	*regs=priv->pBdxRegs;
    u32 tmp_reg;

    /* Wait until MDIO is not busy */
    if (bdx_mdio_get(priv)== 0xFFFFFFFF)
    {
        return -1;
    }
	writel(((device&0x1F)|((port&0x1F)<<5)), regs + regMDIO_CMD);
	writel((u32)addr, regs + regMDIO_ADDR);
	if (bdx_mdio_get(priv)== 0xFFFFFFFF) {		return -1;	}
	writel((u32)data, regs + regMDIO_DATA);
    /* Read CMD_STAT until not busy */
    if ((tmp_reg = bdx_mdio_get(priv)) == 0xFFFFFFFF)
    {
        ERR("MDIO busy after write command\n");
        return -1;
    }
    if (GET_MDIO_RD_ERR(tmp_reg))
    {
        ERR("MDIO error after write command\n");
        return -1;
    }
    return 0;

} // bdx_mdio_write()

//-------------------------------------------------------------------------------------------------

void setMDIOSpeed(struct bdx_priv *priv, u32 speed)
{
	void __iomem 	*regs=priv->pBdxRegs;
    int 			mdio_cfg;

	mdio_cfg  = readl(regs + regMDIO_CMD_STAT);
    if(1==speed)
    {
		mdio_cfg  = (0x7d<<7) | 0x08; // 1MHz
    }
    else
    {
		mdio_cfg  = 0xA08; // 6MHz
    }
	mdio_cfg |= (1<<6);
	writel(mdio_cfg, regs + regMDIO_CMD_STAT);
	msleep(100);

} // setMDIOSpeed()

//-------------------------------------------------------------------------------------------------

int bdx_mdio_look_for_phy(struct bdx_priv *priv, int port)
{
    int phy_id, i;
    int rVal = -1;

    i=port;
    setMDIOSpeed(priv, MDIO_SPEED_1MHZ);

    phy_id = bdx_mdio_read(priv, 1, i, 0x0002); // PHY_ID_HIGH
    phy_id &=0xFFFF;
	for (i = 0; i < 32; i++)
	{
		msleep(10);
		DBG("LOOK FOR PHY: port=0x%x\n",i);
		phy_id  = bdx_mdio_read(priv, 1, i, 0x0002); // PHY_ID_HIGH
		phy_id &=0xFFFF;
		if (phy_id!=0xFFFF && phy_id!= 0)
		{
			rVal = i;
			break;
		}
	}
    if (rVal == -1)
    {
    	ERR("PHY not found\n");
    }

    return rVal;

} // bdx_mdio_look_for_phy()

//-------------------------------------------------------------------------------------------------

static int __init bdx_mdio_phy_search(struct bdx_priv *priv, void __iomem * regs, int *port_t, unsigned  short *phy_t)
{
    int 	i, phy_id;
    char 	*s;

    if (bdx_force_no_phy_mode)
    {
    	ERR("Forced NO PHY mode\n");
    	i = 0;
    }
    else
    {
    	i = bdx_mdio_look_for_phy(priv,*port_t);
		if (i >= 0)  // PHY  found
		{
			*port_t = i;
			phy_id  = bdx_mdio_read(priv, 1, *port_t, 0x0002); // PHY_ID_HI
			i       = phy_id << 16;
			phy_id  = bdx_mdio_read(priv, 1, *port_t, 0x0003); // PHY_ID_LOW
			phy_id &=0xFFFF;
			i      |= phy_id;
		}
    }
	switch(i)
    {

#ifdef PHY_QT2025
        case 0x0043A400:
            *phy_t=PHY_TYPE_QT2025;
            s="QT2025 10Gbps SFP+";
            *phy_t = QT2025_register(priv);
            break;
#endif

#ifdef PHY_MV88X3120
        case 0x01405896:
            s="MV88X3120 10Gbps 10GBase-T";
            *phy_t = MV88X3120_register(priv);
            break;

#endif

#if (defined PHY_MV88X3310) || (defined PHY_MV88E2010)
        case 0x02b09aa:
        case 0x02b09ab:
        	if (priv->deviceId == 0x4027)
        	{
        		s = (i == 0x02b09aa) ? "MV88X3310 (A0) 10Gbps 10GBase-T" : "MV88X3310 (A1) 10Gbps 10GBase-T";
        		*phy_t = MV88X3310_register(priv);
        	}
        	else if (priv->deviceId == 0x4527)
        	{
        		s = (i == 0x02b09aa) ? "MV88E2010 (A0) 5Gbps 5GBase-T"   : "MV88E2010 (A1) 5Gbps 5GBase-T";
        		*phy_t = MV88X3310_register(priv);
        	}
        	else if (priv->deviceId == 0x4010)
        	{
        		s = "Dummy CX4";
        		*phy_t = CX4_register(priv);
        	}
        	else
        	{
        		s = "";
        		ERR("Unsupported device id/phy id 0x%x/0x%x !\n",priv->pdev->device, i);
        	}
            break;

#endif

#ifdef PHY_TLK10232
        case 0x40005100:
            s="TLK10232 10Gbps SFP+";
            *phy_t = TLK10232_register(priv);
            break;
#endif

#ifdef PHY_AQR105
        case 0x03A1B462:   //AQR105 B0
        case 0x03A1B463:   //AQR105 B1
        case 0x03A1B4A3:   //AQR105 B1

            s="AQR105 10Gbps 10GBase-T";
            *phy_t = AQR105_register(priv);
            break;
#endif

        default:
            *phy_t=PHY_TYPE_CX4;
            s="Native 10Gbps CX4";
            *phy_t = CX4_register(priv);
            break;

    } // switch(i)

	priv->isr_mask |= IR_TMR1;
	setMDIOSpeed(priv, priv->phy_ops.mdio_speed);
    MSG("PHY detected on port %u ID=%X - %s\n", *port_t,i,s);

    return (PHY_TYPE_NA == *phy_t) ? -1 : 0;

} // bdx_mdio_phy_search()

//-------------------------------------------------------------------------------------------------

static int __init bdx_mdio_reset(struct bdx_priv *priv, int port, unsigned  short phy)
{
    void __iomem 	*regs=priv->pBdxRegs;
    int 			port_t = ++port;
    unsigned  short phy_t  = phy;

    priv->phy_mdio_port=0xFF;
    if(-1 == bdx_mdio_phy_search(priv, regs, &port_t, &phy_t))
    {
    	return -1;
    }
    if(phy!=phy_t)
    {
        ERR("PHY type by svid %u found %u\n",phy,phy_t);
        phy=phy_t;
    }
    port                = port_t;
    priv->phy_mdio_port	= port;
    priv->phy_type		= phy;

    return priv->phy_ops.mdio_reset(priv, port, phy);

} // bdx_mdio_reset()

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

    MSG("srom 0x%x HWver %d build %u lane# %d max_pl 0x%x mrrs 0x%x\n",
           readl(nic->regs + SROM_VER),
           readl(nic->regs + FPGA_VER) & 0xFFFF,
           readl(nic->regs + FPGA_SEED),
           GET_LINK_STATUS_LANES(pci_link_status),
           GET_DEV_CTRL_MAXPL(pci_ctrl), GET_DEV_CTRL_MRRS(pci_ctrl));
}

//-------------------------------------------------------------------------------------------------

static void print_fw_id(struct pci_nic *nic)
{
    MSG("fw 0x%x\n", readl(nic->regs + FW_VER));
}

//-------------------------------------------------------------------------------------------------

static void print_eth_id(struct net_device *ndev)
{
    MSG("%s, Port %c\n", ndev->name, (ndev->if_port == 0) ? 'A' : 'B');
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
    /* pci_alloc_consistent gives us 4k-aligned memory */
    if (f->va == NULL)
    {
   	f->va = pci_alloc_consistent(priv->pdev,
    			memsz + FIFO_EXTRA_SPACE, &f->da);
    	if (! f->va)
    	{
    		ERR("pci_alloc_consistent failed\n");
    		RET(-ENOMEM);
    	}
    }
    f->reg_CFG0  = reg_CFG0;
    f->reg_CFG1  = reg_CFG1;
    f->reg_RPTR  = reg_RPTR;
    f->reg_WPTR  = reg_WPTR;
    f->rptr      = 0;
    f->wptr      = 0;
    f->memsz     = memsz;
    f->size_mask = memsz - 1;
    WRITE_REG(priv, reg_CFG0, (u32) ((f->da & TX_RX_CFG0_BASE) | fsz_type));
    WRITE_REG(priv, reg_CFG1, H32_64(f->da));

    RET(0);
}

//-------------------------------------------------------------------------------------------------

/* bdx_fifo_free - Free all resources used by fifo
 * @priv     - Nic private structure
 * @f        - Fifo to release
 */
static void bdx_fifo_free(struct bdx_priv *priv, struct fifo *f)
{
    ENTER;
    if (f->va)
    {
        pci_free_consistent(priv->pdev,
                            f->memsz + FIFO_EXTRA_SPACE, f->va, f->da);
        f->va = NULL;
    }
    RET();
}

//-------------------------------------------------------------------------------------------------

int bdx_speed_set(struct bdx_priv *priv, u32 speed)
{
    int i;
    u32 val;
    //void __iomem * regs=priv->pBdxRegs;
    //int port=priv->phy_mdio_port;

	DBG("speed %d\n", speed);

	switch(speed)
    {
        case SPEED_10000:
        case SPEED_5000:
        case SPEED_2500:
        case SPEED_1000X:
        case SPEED_100X:
        	DBG("link_speed %d\n", speed);
			// BDX_MDIO_WRITE(priv, 1,0,0x2040);
            WRITE_REG(priv, 0x1010, 0x217); /*ETHSD.REFCLK_CONF  */
            WRITE_REG(priv, 0x104c, 0x4c);  /*ETHSD.L0_RX_PCNT  */
            WRITE_REG(priv, 0x1050, 0x4c);  /*ETHSD.L1_RX_PCNT  */
            WRITE_REG(priv, 0x1054, 0x4c);  /*ETHSD.L2_RX_PCNT  */
            WRITE_REG(priv, 0x1058, 0x4c);  /*ETHSD.L3_RX_PCNT  */
            WRITE_REG(priv, 0x102c, 0x434); /*ETHSD.L0_TX_PCNT  */
            WRITE_REG(priv, 0x1030, 0x434); /*ETHSD.L1_TX_PCNT  */
            WRITE_REG(priv, 0x1034, 0x434); /*ETHSD.L2_TX_PCNT  */
            WRITE_REG(priv, 0x1038, 0x434); /*ETHSD.L3_TX_PCNT  */
            WRITE_REG(priv, 0x6300, 0x0400); /*MAC.PCS_CTRL*/
            //  udelay(50); val = READ_REG(priv,0x6300);ERR("MAC init:0x6300= 0x%x \n",val);
            WRITE_REG(priv, 0x1018, 0x00); /*Mike2*/
            udelay(5);
            WRITE_REG(priv, 0x1018, 0x04); /*Mike2*/
            udelay(5);
            WRITE_REG(priv, 0x1018, 0x06); /*Mike2*/
            udelay(5);
            //MikeFix1
            //L0: 0x103c , L1: 0x1040 , L2: 0x1044 , L3: 0x1048 =0x81644
            WRITE_REG(priv, 0x103c, 0x81644); /*ETHSD.L0_TX_DCNT  */
            WRITE_REG(priv, 0x1040, 0x81644); /*ETHSD.L1_TX_DCNT  */
            WRITE_REG(priv, 0x1044, 0x81644); /*ETHSD.L2_TX_DCNT  */
            WRITE_REG(priv, 0x1048, 0x81644); /*ETHSD.L3_TX_DCNT  */
            WRITE_REG(priv, 0x1014, 0x043); /*ETHSD.INIT_STAT*/
            for(i=1000; i; i--)
            {
                udelay(50);
                val = READ_REG(priv,0x1014); /*ETHSD.INIT_STAT*/
                if(val & (1<<9))
                {
                    WRITE_REG(priv, 0x1014, 0x3); /*ETHSD.INIT_STAT*/
                    val = READ_REG(priv,0x1014); /*ETHSD.INIT_STAT*/
                    //                 ERR("MAC init:0x1014=0x%x i=%d\n",val,i);
                    break;
                }
            }
            if(0==i)
            {
                ERR("MAC init timeout!\n");
            }

            WRITE_REG(priv, 0x6350, 0x0); /*MAC.PCS_IF_MODE*/
            WRITE_REG(priv, regCTRLST, 0xC13);//0x93//0x13
            WRITE_REG(priv, 0x111c, 0x7ff); /*MAC.MAC_RST_CNT*/
            for(i=40; i--;)
            {
                udelay(50);
            }
            WRITE_REG(priv, 0x111c, 0x0); /*MAC.MAC_RST_CNT*/
            //WRITE_REG(priv, 0x1104,0x24);  // EEE_CTRL.EXT_PHY_LINK=1 (bit 2)
            break;

        case SPEED_1000:
        case SPEED_100:
            //BDX_MDIO_WRITE(priv, 1,0,0x2000);       // write  1.0 0x2000  # Force 1G
            WRITE_REG(priv, 0x1010, 0x613); /*ETHSD.REFCLK_CONF  */
            WRITE_REG(priv, 0x104c, 0x4d);  /*ETHSD.L0_RX_PCNT  */
            WRITE_REG(priv, 0x1050, 0x0);  /*ETHSD.L1_RX_PCNT  */
            WRITE_REG(priv, 0x1054, 0x0);  /*ETHSD.L2_RX_PCNT  */
            WRITE_REG(priv, 0x1058, 0x0);  /*ETHSD.L3_RX_PCNT  */
            WRITE_REG(priv, 0x102c, 0x35); /*ETHSD.L0_TX_PCNT  */
            WRITE_REG(priv, 0x1030, 0x0); /*ETHSD.L1_TX_PCNT  */
            WRITE_REG(priv, 0x1034, 0x0); /*ETHSD.L2_TX_PCNT  */
            WRITE_REG(priv, 0x1038, 0x0); /*ETHSD.L3_TX_PCNT  */
            WRITE_REG(priv, 0x6300, 0x01140); /*MAC.PCS_CTRL*/
            //  udelay(50); val = READ_REG(priv,0x6300);ERR("MAC init:0x6300= 0x%x \n",val);
            WRITE_REG(priv, 0x1014, 0x043); /*ETHSD.INIT_STAT*/
            for(i=1000; i; i--)
            {
                udelay(50);
                val = READ_REG(priv,0x1014); /*ETHSD.INIT_STAT*/
                if(val & (1<<9))
                {
                    WRITE_REG(priv, 0x1014, 0x3); /*ETHSD.INIT_STAT*/
                    val = READ_REG(priv,0x1014); /*ETHSD.INIT_STAT*/
                    //                 ERR("MAC init:0x1014= 0x%x i=%d\n",val,i);
                    break;
                }
            }
            if(0==i)
            {
                ERR("MAC init timeout!\n");
            }
            WRITE_REG(priv, 0x6350, 0x2b); /*MAC.PCS_IF_MODE 1g*/
            WRITE_REG(priv, 0x6310, 0x9801); /*MAC.PCS_DEV_AB */
            //100 WRITE_REG(priv, 0x6350, 0x27); /*MAC.PCS_IF_MODE 100m*/
            //100 WRITE_REG(priv, 0x6310, 0x9501); /*MAC.PCS_DEV_AB */

            WRITE_REG(priv, 0x6314, 0x1); /*MAC.PCS_PART_AB */
            WRITE_REG(priv, 0x6348, 0xc8); /*MAC.PCS_LINK_LO */
            WRITE_REG(priv, 0x634c, 0xc8); /*MAC.PCS_LINK_HI */
            udelay(50);
            WRITE_REG(priv, regCTRLST, 0xC13);//0x93//0x13
            WRITE_REG(priv, 0x111c, 0x7ff); /*MAC.MAC_RST_CNT*/
            for(i=40; i--;)
            {
                udelay(50);
            }
            WRITE_REG(priv, 0x111c, 0x0); /*MAC.MAC_RST_CNT*/
            WRITE_REG(priv, 0x6300, 0x1140); /*MAC.PCS_CTRL*/
            //        WRITE_REG(priv, 0x1104,0x24);  // EEE_CTRL.EXT_PHY_LINK=1 (bit 2)
            break;

      case 0: // Link down
            //  WRITE_REG(priv, 0x1010, 0x613); /*ETHSD.REFCLK_CONF  */
            WRITE_REG(priv, 0x104c, 0x0);  /*ETHSD.L0_RX_PCNT  */
            WRITE_REG(priv, 0x1050, 0x0);  /*ETHSD.L1_RX_PCNT  */
            WRITE_REG(priv, 0x1054, 0x0);  /*ETHSD.L2_RX_PCNT  */
            WRITE_REG(priv, 0x1058, 0x0);  /*ETHSD.L3_RX_PCNT  */
            WRITE_REG(priv, 0x102c, 0x0); /*ETHSD.L0_TX_PCNT  */
            WRITE_REG(priv, 0x1030, 0x0); /*ETHSD.L1_TX_PCNT  */
            WRITE_REG(priv, 0x1034, 0x0); /*ETHSD.L2_TX_PCNT  */
            WRITE_REG(priv, 0x1038, 0x0); /*ETHSD.L3_TX_PCNT  */

            //                 WRITE_REG(priv, 0x1104,0x20); // EEE_CTRL.EXT_PHY_LINK=0 (bit 2)

            WRITE_REG(priv, regCTRLST, 0x800);
            WRITE_REG(priv, 0x111c, 0x7ff); /*MAC.MAC_RST_CNT*/
            for(i=40; i--;)
            {
                udelay(50);
            }
            WRITE_REG(priv, 0x111c, 0x0); /*MAC.MAC_RST_CNT*/
            break;
        default:
            ERR("%s Link speed was not identified yet (%d)\n", priv->ndev->name, speed);
            speed=0;
            break;
    }

	return speed;

} // bdx_speed_set()

//-------------------------------------------------------------------------------------------------

void bdx_speed_changed(struct bdx_priv *priv, u32 speed)
{

	DBG("speed %d\n", speed);
	speed = bdx_speed_set(priv, speed);
	DBG("link_speed %d speed %d\n", priv->link_speed, speed);
    if (priv->link_speed != speed)
    {
    	priv->link_speed = speed;
    	DBG("%s Speed changed %d\n", priv->ndev->name, priv->link_speed);
    }

} // bdx_speed_changed()

//-------------------------------------------------------------------------------------------------

/*
 * bdx_link_changed - Notify the OS about hw link state.
 *
 * @bdx_priv - HW adapter structure
 */

static void bdx_link_changed(struct bdx_priv *priv)
{
    u32 link = priv->phy_ops.link_changed(priv);;

    if (!link)
    {
        if (netif_carrier_ok(priv->ndev))
        {
            netif_stop_queue(priv->ndev);
            netif_carrier_off(priv->ndev);
            ERR("%s Link Down\n", priv->ndev->name);
#ifdef _EEE_
    	    if(priv->phy_ops.reset_eee != NULL)
    	    {
    	    	priv->phy_ops.reset_eee(priv);
    	    }
#endif
        }
    }
    else
    {
        if (!netif_carrier_ok(priv->ndev))
        {
            netif_wake_queue(priv->ndev);
            netif_carrier_on(priv->ndev);
            MSG("%s Link Up %s\n", priv->ndev->name, (priv->link_speed == SPEED_10000) ? "10G"   :
            										 (priv->link_speed == SPEED_5000)  ? "5G"    :
            										 (priv->link_speed == SPEED_2500)  ? "2.5G"  :
            										 (priv->link_speed == SPEED_1000) || (priv->link_speed == SPEED_1000X) ? "1G"   :
            										 (priv->link_speed == SPEED_100)  || (priv->link_speed == SPEED_100X)  ? "100M" :
            										  " ");
        }
    }

} // bdx_link_changed()

//-------------------------------------------------------------------------------------------------

static inline void bdx_isr_extra(struct bdx_priv *priv, u32 isr)
{
    if (isr & (IR_LNKCHG0|IR_LNKCHG1|IR_TMR0))
    {
    	DBG("isr = 0x%x\n", isr);
    	bdx_link_changed(priv);
    }
#if 0
    if (isr & IR_RX_FREE_0)
    {
        DBG("RX_FREE_0\n");
    }
    if (isr & IR_PCIE_LINK)
        ERR("%s PCI-E Link Fault\n", priv->ndev->name);

    if (isr & IR_PCIE_TOUT)
        ERR("%s PCI-E Time Out\n", priv->ndev->name);
#endif
}

//-------------------------------------------------------------------------------------------------

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24) || (defined VM_KLNX)
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

//    ENTER;
    isr = READ_REG(priv, regISR_MSK0);
    //DBG("isr = 0x%x\n", isr);
    //  isr = READ_REG(priv, 0x5100);
    if (unlikely(!isr))
    {
        bdx_enable_interrupts(priv);
        return IRQ_NONE;    /* Not our interrupt */
    }

    if (isr & IR_EXTRA)
        bdx_isr_extra(priv, isr);

    if (isr & (IR_RX_DESC_0 | IR_TX_FREE_0 | IR_TMR1))
    {
        if (likely(LUXOR__SCHEDULE_PREP(&priv->napi, ndev)))
        {
            LUXOR__SCHEDULE(&priv->napi, ndev);
            RET(IRQ_HANDLED);
        }
        else
        {
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
//    RET(IRQ_HANDLED);
    return IRQ_HANDLED;

}

//-------------------------------------------------------------------------------------------------

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
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
static irqreturn_t bdx_isr_napi(int irq, struct net_device *ndev)
{
    struct bdx_priv *priv = ndev->priv;
    u32 isr;

    ENTER;
    isr = READ_REG(priv, regISR_MSK0);
    if (unlikely(!isr))
    {
        bdx_enable_interrupts(priv);
        return IRQ_NONE;    /* Not our interrupt */
    }

    if (isr & IR_EXTRA)
        bdx_isr_extra(priv, isr);

    if (isr & (IR_RX_DESC_0 | IR_TX_FREE_0))
    {
        if (likely(LUXOR__SCHEDULE_PREP(&priv->napi, ndev)))
        {
            LUXOR__SCHEDULE(&priv->napi, ndev);
            RET(IRQ_HANDLED);
        }
        else
        {
            /*
                         * NOTE: We get here if an interrupt has slept into
                         *       the small time window between these lines in
                         *       bdx_poll:
             *   bdx_enable_interrupts(priv);
             *   return 0;
                         *
             *   Currently interrupts are disabled (since we
                         *   read the ISR register) and we have failed to
                         *       register the next poll. So we read the regs to
                         *       trigger the chip and allow further interrupts.
                         */
            READ_REG(priv, regTXF_WPTR_0);
            READ_REG(priv, regRXD_WPTR_0);
        }
    }

    bdx_enable_interrupts(priv);
    RET(IRQ_HANDLED);
}

//-------------------------------------------------------------------------------------------------

#else
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
static irqreturn_t bdx_isr_napi(int irq, void *dev, struct pt_regs *regs)
{
    struct net_device *ndev = (struct net_device *)dev;
    struct bdx_priv *priv = ndev->priv;
    u32 isr;

    ENTER;
    isr = READ_REG(priv, regISR_MSK0);
    if (unlikely(!isr))
    {
        bdx_enable_interrupts(priv);
        return IRQ_NONE;    /* Not our interrupt */
    }

    if (isr & IR_EXTRA)
        bdx_isr_extra(priv, isr);

    if (isr & (IR_RX_DESC_0 | IR_TX_FREE_0))
    {
        if (likely(LUXOR__SCHEDULE_PREP(&priv->napi, ndev)))
        {
            LUXOR__SCHEDULE(&priv->napi, ndev);
            RET(IRQ_HANDLED);
        }
        else
        {
            /*
                         * NOTE: We get here if an interrupt has slept into
                         *       the small time window between these lines in
                         *       bdx_poll:
             *   bdx_enable_interrupts(priv);
             *   return 0;
                         *
             *   Currently interrupts are disabled (since we
                         *       read the ISR register) and we have failed to
                         *       register the next poll. So we read the regs to
                         *       trigger the chip and allow further interrupts.
                         */
            READ_REG(priv, regTXF_WPTR_0);
            READ_REG(priv, regRXD_WPTR_0);
        }
    }

    bdx_enable_interrupts(priv);
    RET(IRQ_HANDLED);
}

#endif

//-------------------------------------------------------------------------------------------------

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28) || (defined VM_KLNX)
static int bdx_poll(struct napi_struct *napi, int budget)
{
    struct bdx_priv *priv = container_of(napi, struct bdx_priv, napi);
    int work_done;

    ENTER;
    if (!priv->bDeviceRemoved)
    {
		bdx_tx_cleanup(priv);

		work_done = bdx_rx_receive(priv, &priv->rxd_fifo0, budget);
		if (work_done < budget)
		{
			napi_complete(napi);
			bdx_enable_interrupts(priv);
		}
    }
    else
    {
    	work_done = budget;
    }

    return work_done;
}

//-------------------------------------------------------------------------------------------------

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
static int bdx_poll(struct napi_struct *napi, int budget)
{
    struct bdx_priv *priv = container_of(napi, struct bdx_priv, napi);
    int work_done;

    ENTER;
    if (!priv->bDeviceRemoved)
    {

		bdx_tx_cleanup(priv);
		work_done = bdx_rx_receive(priv, &priv->rxd_fifo0, budget);
		if (work_done < budget)
		{
			netif_rx_complete(priv->ndev, napi);
			bdx_enable_interrupts(priv);
		}
    }
    else
    {
    	work_done = budget;
    }

    return work_done;
}

//-------------------------------------------------------------------------------------------------

#else
static int bdx_poll(struct net_device *ndev, int *budget_p)
{
    struct 	bdx_priv *priv = ndev->priv;
    int 	work_done;
    int		rVal = 0;

    ENTER;
    if (!priv->bDeviceRemoved)
    {
    	bdx_tx_cleanup(priv);
		work_done          = bdx_rx_receive(priv, &priv->rxd_fifo0,	min(*budget_p, priv->ndev->quota));
		*budget_p         -= work_done;
		priv->ndev->quota -= work_done;
		if (work_done < *budget_p)
		{
			DBG("rx poll is done. backing to isr-driven\n");
			netif_rx_complete(ndev);
			bdx_enable_interrupts(priv);
		}
		rVal = 1;
    }

    return rVal;
}
#endif

//-------------------------------------------------------------------------------------------------

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

    ENTER;
    master = READ_REG(priv, regINIT_SEMAPHORE);
    if (!READ_REG(priv, regINIT_STATUS) && master)
    {
        DBG("Loading FW...\n");
        bdx_tx_push_desc_safe(priv, s_firmLoad, sizeof(s_firmLoad));
        mdelay(100);
    }
    for (i = 0; i < 200; i++)
    {
        if (READ_REG(priv, regINIT_STATUS))
            break;
        mdelay(2);
    }
    if (master)
        WRITE_REG(priv, regINIT_SEMAPHORE, 1);

    if (i == 200)
    {
        ERR("%s firmware loading failed\n", priv->ndev->name);
        DBG("VPC = 0x%x VIC = 0x%x INIT_STATUS = 0x%x i =%d\n",
            READ_REG(priv, regVPC),
            READ_REG(priv, regVIC), READ_REG(priv, regINIT_STATUS), i);
        rVal = -EIO;
    }
    else
    {
        DBG("%s firmware loading success\n", priv->ndev->name);
    }
    print_fw_id(priv->nic);
    RET(rVal);

}

//-------------------------------------------------------------------------------------------------

static void bdx_restore_mac(struct net_device *ndev, struct bdx_priv *priv)
{
    u32 val;

    ENTER;
    DBG("mac0 =%x mac1 =%x mac2 =%x\n",
        READ_REG(priv, regUNC_MAC0_A),
        READ_REG(priv, regUNC_MAC1_A), READ_REG(priv, regUNC_MAC2_A));

    val = (ndev->dev_addr[0] << 8) | (ndev->dev_addr[1]);
    WRITE_REG(priv, regUNC_MAC2_A, val);
    val = (ndev->dev_addr[2] << 8) | (ndev->dev_addr[3]);
    WRITE_REG(priv, regUNC_MAC1_A, val);
    val = (ndev->dev_addr[4] << 8) | (ndev->dev_addr[5]);
    WRITE_REG(priv, regUNC_MAC0_A, val);

    /* More then IP MAC address */
    WRITE_REG(priv, regMAC_ADDR_0, (ndev->dev_addr[3] << 24) | (ndev->dev_addr[2] << 16) | (ndev->dev_addr[1] << 8) | (ndev->dev_addr[0]));
    WRITE_REG(priv, regMAC_ADDR_1, (ndev->dev_addr[5] << 8)  | (ndev->dev_addr[4]));

    DBG("mac0 =%x mac1 =%x mac2 =%x\n",
        READ_REG(priv, regUNC_MAC0_A),
        READ_REG(priv, regUNC_MAC1_A), READ_REG(priv, regUNC_MAC2_A));
    RET();
}

//-------------------------------------------------------------------------------------------------

static void bdx_CX4_hw_start(struct bdx_priv *priv)
{
	int i;
	u32 val;

	WRITE_REG(priv, 0x1010, 0x217); /*ETHSD.REFCLK_CONF  */
	WRITE_REG(priv, 0x104c, 0x4c);  /*ETHSD.L0_RX_PCNT  */
	WRITE_REG(priv, 0x1050, 0x4c);  /*ETHSD.L1_RX_PCNT  */
	WRITE_REG(priv, 0x1054, 0x4c);  /*ETHSD.L2_RX_PCNT  */
	WRITE_REG(priv, 0x1058, 0x4c);  /*ETHSD.L3_RX_PCNT  */
	WRITE_REG(priv, 0x102c, 0x434); /*ETHSD.L0_TX_PCNT  */
	WRITE_REG(priv, 0x1030, 0x434); /*ETHSD.L1_TX_PCNT  */
	WRITE_REG(priv, 0x1034, 0x434); /*ETHSD.L2_TX_PCNT  */
	WRITE_REG(priv, 0x1038, 0x434); /*ETHSD.L3_TX_PCNT  */
	WRITE_REG(priv, 0x6300, 0x0400); /*MAC.PCS_CTRL*/
	//  udelay(50); val = READ_REG(priv,0x6300);ERR("MAC init:0x6300= 0x%x \n",val);
	WRITE_REG(priv, 0x1018, 0x00); /*Mike2*/
	udelay(5);
	WRITE_REG(priv, 0x1018, 0x04); /*Mike2*/
	udelay(5);
	WRITE_REG(priv, 0x1018, 0x06); /*Mike2*/
	udelay(5);
	//MikeFix1
	//L0: 0x103c , L1: 0x1040 , L2: 0x1044 , L3: 0x1048 =0x81644
	WRITE_REG(priv, 0x103c, 0x81644); /*ETHSD.L0_TX_DCNT  */
	WRITE_REG(priv, 0x1040, 0x81644); /*ETHSD.L1_TX_DCNT  */
	WRITE_REG(priv, 0x1044, 0x81644); /*ETHSD.L2_TX_DCNT  */
	WRITE_REG(priv, 0x1048, 0x81644); /*ETHSD.L3_TX_DCNT  */
	WRITE_REG(priv, 0x1014, 0x043); /*ETHSD.INIT_STAT*/
	for(i=1000; i; i--)
	{
		udelay(50);
		val = READ_REG(priv,0x1014); /*ETHSD.INIT_STAT*/
		if(val & (1<<9))
		{
			WRITE_REG(priv, 0x1014, 0x3); /*ETHSD.INIT_STAT*/
			val = READ_REG(priv,0x1014); /*ETHSD.INIT_STAT*/
			//                 ERR("MAC init:0x1014=0x%x i=%d\n",val,i);
			break;
		}
	}
	if(0==i)
	{
		ERR("MAC init timeout!\n");
	}
	WRITE_REG(priv, 0x6350, 0x0); /*MAC.PCS_IF_MODE*/
	WRITE_REG(priv, regCTRLST, 0xC13);//0x93//0x13
	WRITE_REG(priv, 0x111c, 0x7ff); /*MAC.MAC_RST_CNT*/
	// CX4 
        WRITE_REG(priv, regFRM_LENGTH, 0X3FE0);
        WRITE_REG(priv, regRX_FIFO_SECTION, 0x10);
        WRITE_REG(priv, regTX_FIFO_SECTION, 0xE00010);

	for(i=40; i--;)
	{
		udelay(50);
	}
	WRITE_REG(priv, 0x111c, 0x0); /*MAC.MAC_RST_CNT*/
	//WRITE_REG(priv, 0x1104,0x24);  // EEE_CTRL.EXT_PHY_LINK=1 (bit 2)


} // bdx_CX4_hw_start

//-------------------------------------------------------------------------------------------------
/*
static  void bdx_setAffinity(u32 irq)
{
	//u32 cpu;

	DBG("bdx_setAffinity  nr_cpu_ids 0x%x\n", nr_cpu_ids);
	DBG("bdx_setAffinity  cpu_online_mask 0x%x\n", *(u32*)cpu_online_mask);
	if (irq_set_affinity_hint(irq, cpu_online_mask))
	{
		ERR("bdx_setAffinity() irq_set_affinity_hint failed!\n");
	}

} // setAffinity()
*/

//-------------------------------------------------------------------------------------------------

/* bdx_hw_start - Initialize registers and starts HW's Rx and Tx engines
 * @priv    - NIC private structure
 */
static int bdx_hw_start(struct bdx_priv *priv)
{

    ENTER;

    DBG("********** bdx_hw_start() ************\n");
    priv->link_speed=0; /* -1 */
    if(priv->phy_type==PHY_TYPE_CX4)
    {
    	bdx_CX4_hw_start(priv);
    }
    else
    {
    	DBG("********** bdx_hw_start() NOT CX4 ************\n");
		WRITE_REG(priv, regFRM_LENGTH, 0X3FE0);
		WRITE_REG(priv, regGMAC_RXF_A, 0X10fd);
		//MikeFix1
		//L0: 0x103c , L1: 0x1040 , L2: 0x1044 , L3: 0x1048 =0x81644
		WRITE_REG(priv, 0x103c, 0x81644); /*ETHSD.L0_TX_DCNT  */
		WRITE_REG(priv, 0x1040, 0x81644); /*ETHSD.L1_TX_DCNT  */
		WRITE_REG(priv, 0x1044, 0x81644); /*ETHSD.L2_TX_DCNT  */
		WRITE_REG(priv, 0x1048, 0x81644); /*ETHSD.L3_TX_DCNT  */
		WRITE_REG(priv, regRX_FIFO_SECTION, 0x10);
		WRITE_REG(priv, regTX_FIFO_SECTION, 0xE00010);
		WRITE_REG(priv, regRX_FULLNESS, 0);
		WRITE_REG(priv, regTX_FULLNESS, 0);
	}
    WRITE_REG(priv, regVGLB, 0);
    WRITE_REG(priv, regMAX_FRAME_A, priv->rxf_fifo0.m.pktsz & MAX_FRAME_AB_VAL);

    DBG("RDINTCM =%08x\n", priv->rdintcm);  /*NOTE: test script uses this */
    WRITE_REG(priv, regRDINTCM0, priv->rdintcm);
    WRITE_REG(priv, regRDINTCM2, 0);    /*cpu_to_le32(rcm.val)); */

    DBG("TDINTCM =%08x\n", priv->tdintcm);  /*NOTE: test script uses this */
    WRITE_REG(priv, regTDINTCM0, priv->tdintcm);    /* old val = 0x300064 */

    /* Enable timer interrupt once in 2 secs. */
    /*WRITE_REG(priv, regGTMR0, ((GTMR_SEC * 2) & GTMR_DATA)); */
    bdx_restore_mac(priv->ndev, priv);
    /* Pause frame */
    WRITE_REG(priv, 0x12E0, 0x28);
    WRITE_REG(priv, regPAUSE_QUANT, 0xFFFF);
    WRITE_REG(priv, 0x6064, 0xF);

    WRITE_REG(priv, regGMAC_RXF_A, GMAC_RX_FILTER_OSEN | GMAC_RX_FILTER_TXFC |
              GMAC_RX_FILTER_AM | GMAC_RX_FILTER_AB);

  //  bdx_setAffinity(priv->pdev->irq);
    bdx_link_changed(priv);
    bdx_enable_interrupts(priv);
    LUXOR__POLL_ENABLE(priv->ndev);
    priv->state &= ~BDX_STATE_HW_STOPPED;

    RET(0);

} // bdx_hw_start()

//-------------------------------------------------------------------------------------------------

static void bdx_hw_stop(struct bdx_priv *priv)
{
    ENTER;

    if ((priv->state & BDX_STATE_HW_STOPPED) == 0)
    {
		priv->state |= BDX_STATE_HW_STOPPED;
		bdx_disable_interrupts(priv);
		LUXOR__POLL_DISABLE(priv->ndev);
		netif_carrier_off(priv->ndev);
		netif_stop_queue(priv->ndev);
    }

    RET();

} // bdx_hw_stop()

//-------------------------------------------------------------------------------------------------

static int bdx_hw_reset_direct(void __iomem *regs)
{
    u32 val, i;
    ENTER;

    /* Reset sequences: read, write 1, read, write 0 */
    val = readl(regs + regCLKPLL);
    writel((val | CLKPLL_SFTRST) + 0x8, regs + regCLKPLL);
    udelay(50);
    val = readl(regs + regCLKPLL);
    writel(val & ~CLKPLL_SFTRST, regs + regCLKPLL);

    /* Check that the PLLs are locked and reset ended */
    for (i = 0; i < 70; i++, mdelay(10))
        if ((readl(regs + regCLKPLL) & CLKPLL_LKD) == CLKPLL_LKD)
        {
            udelay(50);
            /* do any PCI-E read transaction */
            readl(regs + regRXD_CFG0_0);
            RET(0);
        }
    ERR("HW reset failed\n");

    RET(1);       /* failure */

} // bdx_hw_reset_direct()

//-------------------------------------------------------------------------------------------------

static int bdx_hw_reset(struct bdx_priv *priv)
{
    u32 val, i;
    ENTER;

    if (priv->port == 0)
    {
        /* Reset sequences: read, write 1, read, write 0 */
        val = READ_REG(priv, regCLKPLL);
        WRITE_REG(priv, regCLKPLL, (val | CLKPLL_SFTRST) + 0x8);
        udelay(50);
        val = READ_REG(priv, regCLKPLL);
        WRITE_REG(priv, regCLKPLL, val & ~CLKPLL_SFTRST);
    }
    /* Check that the PLLs are locked and reset ended */
    for (i = 0; i < 70; i++, mdelay(10))
    {
        if ((READ_REG(priv, regCLKPLL) & CLKPLL_LKD) == CLKPLL_LKD)
        {
            udelay(50);
            /* Do any PCI-E read transaction */
            READ_REG(priv, regRXD_CFG0_0);
            return 0;

        }
    }
    ERR("HW reset failed\n");

    return 1;       /* Failure */

} // bdx_hw_reset()

//-------------------------------------------------------------------------------------------------

static int bdx_sw_reset(struct bdx_priv *priv)
{
    int i;

    ENTER;
    /* 1. load MAC (obsolete) */
    /* 2. disable Rx (and Tx) */
    WRITE_REG(priv, regGMAC_RXF_A, 0);
    mdelay(100);
    /* 3. Disable port */
    WRITE_REG(priv, regDIS_PORT, 1);
    /* 4. Disable queue */
    WRITE_REG(priv, regDIS_QU, 1);
    /* 5. Wait until hw is disabled */
    for (i = 0; i < 50; i++)
    {
        if (READ_REG(priv, regRST_PORT) & 1)
            break;
        mdelay(10);
    }
    if (i == 50)
    {
        ERR("%s SW reset timeout. continuing anyway\n", priv->ndev->name);
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
    //for (i = regTXD_WPTR_0; i <= regTXF_RPTR_3; i += 0x10)
    //    DBG("%x = %x\n", i, READ_REG(priv, i) & TXF_WPTR_WR_PTR);
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
    /* 15. Save MAC (obsolete) */
    //for (i = regTXD_WPTR_0; i <= regTXF_RPTR_3; i += 0x10)
    //{
    //    DBG("%x = %x\n", i, READ_REG(priv, i) & TXF_WPTR_WR_PTR);
    //}

    RET(0);

} // bdx_sw_reset()

//-------------------------------------------------------------------------------------------------

/* bdx_reset - Perform the right type of reset depending on hw type */
static int bdx_reset(struct bdx_priv *priv)
{
    ENTER;
    //  RET((priv->pdev->device == 0x4010) ? bdx_hw_reset(priv) : bdx_sw_reset(priv));
    RET(bdx_hw_reset(priv));

} // bdx_reset()

//-------------------------------------------------------------------------------------------------

static int bdx_start(struct bdx_priv *priv, int bLoadFw)
{
    int	rc = 0;

    if ((priv->state & BDX_STATE_STARTED) == 0)
    {
    	priv->state |= BDX_STATE_STARTED;
		do
		{
			rc = -1;
			if (bdx_tx_init(priv))
			{
				break;
			}
			if (bdx_rx_init(priv))
			{
				break;
			}
			if (bdx_rx_alloc_pages(priv))
			{
				break;
			}
			bdx_rx_alloc_buffers(priv);
			if ( request_irq(priv->pdev->irq, &bdx_isr_napi, BDX_IRQ_TYPE, priv->ndev->name, priv->ndev))
			{
				break;
			}
			if (bLoadFw && bdx_fw_load(priv))
			{
				break;
			}
			bdx_init_rss(priv);
			rc = 0;
		} while(0);
    }
    if (rc == 0)
    {
		if (priv->state & BDX_STATE_OPEN)
		{
			rc = bdx_hw_start(priv);
		}
    }
    return rc;

} // bdx_start()

//-------------------------------------------------------------------------------------------------

static void bdx_stop(struct bdx_priv *priv)
{
	if (priv->state & BDX_STATE_STARTED)
	{
		priv->state &= ~BDX_STATE_STARTED;
		bdx_hw_stop(priv);
		free_irq(priv->pdev->irq, priv->ndev);
#ifdef TN40_IRQ_MSI
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
		pci_free_irq_vectors(priv->pdev);
#endif
#endif
		bdx_sw_reset(priv);
		bdx_rx_free(priv);
		bdx_rx_free_pages(priv);
		bdx_tx_free(priv);
	}

} // bdx_stop()

//-------------------------------------------------------------------------------------------------

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

    ENTER;
    priv = netdev_priv(ndev);
    bdx_stop(priv);
    LUXOR__NAPI_DISABLE(&priv->napi);
    priv->state &= ~BDX_STATE_OPEN;
    RET(0);

} // bdx_close()

//-------------------------------------------------------------------------------------------------

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

    ENTER;
    priv = netdev_priv(ndev);
    priv->state |= BDX_STATE_OPEN;
    bdx_sw_reset(priv);
    if (netif_running(ndev))
    {
        netif_stop_queue(priv->ndev);
    }
    if ((rc = bdx_start(priv, NO_FW_LOAD)) == 0)
    {
    	LUXOR__NAPI_ENABLE(&priv->napi);
    }
    else
    {
    	bdx_close(ndev);
    }

    RET(rc);

} // bdx_open()

//-------------------------------------------------------------------------------------------------

#ifdef __BIG_ENDIAN
static void __init bdx_firmware_endianess(void)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(s_firmLoad); i++)
    {
        s_firmLoad[i] = CPU_CHIP_SWAP32(s_firmLoad[i]);
    }
}
#endif

//-------------------------------------------------------------------------------------------------

static int bdx_range_check(struct bdx_priv *priv, u32 offset)
{
    return ((offset > (u32) (BDX_REGS_SIZE / priv->nic->port_num)) ?
            -EINVAL : 0);
}


//-------------------------------------------------------------------------------------------------
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

    ENTER;
    DBG("vid =%d value =%d\n", (int)vid, enable);
    if (unlikely(vid >= 4096))
    {
        ERR("invalid VID: %u (> 4096)\n", vid);
        RET();
    }
    reg = regVLAN_0 + (vid / 32) * 4;
    bit = 1 << vid % 32;
    val = READ_REG(priv, reg);
    DBG("reg =%x, val =%x, bit =%d\n", reg, val, bit);
    if (enable)
        val |= bit;
    else
        val &= ~bit;
    DBG("new val %x\n", val);
    WRITE_REG(priv, reg, val);
    RET();
}

//-------------------------------------------------------------------------------------------------
/*
 * bdx_vlan_rx_add_vid - A kernel hook for adding VLAN vid to hw filtering
 *           table
 * @ndev - Network device
 * @vid  - Vlan vid to add
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3,0)
static void bdx_vlan_rx_add_vid(struct net_device *ndev, uint16_t vid)
{
    __bdx_vlan_rx_vid(ndev, vid, 1);
}
#else
#ifdef NETIF_F_HW_VLAN_CTAG_TX
static int bdx_vlan_rx_add_vid(struct net_device *ndev,__always_unused __be16 proto, u16 vid)
#else /* !NETIF_F_HW_VLAN_CTAG_TX */
static int bdx_vlan_rx_add_vid(struct net_device *ndev, u16 vid)
#endif /* NETIF_F_HW_VLAN_CTAG_TX */
{
    __bdx_vlan_rx_vid(ndev, vid, 1);
    return 0;
}
#endif

//-------------------------------------------------------------------------------------------------
/*
 * bdx_vlan_rx_kill_vid - A kernel hook for killing VLAN vid in hw filtering
 *            table
 *
 * @ndev - Network device
 * @vid  - Vlan vid to kill
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3,0)
static void bdx_vlan_rx_kill_vid(struct net_device *ndev, unsigned short vid)
{
    __bdx_vlan_rx_vid(ndev, vid, 0);
}
#else
#ifdef NETIF_F_HW_VLAN_CTAG_RX
static int bdx_vlan_rx_kill_vid(struct net_device *ndev,
				  __always_unused __be16 proto, u16 vid)
#else /* !NETIF_F_HW_VLAN_CTAG_RX */
static int bdx_vlan_rx_kill_vid(struct net_device *ndev, u16 vid)
#endif /* NETIF_F_HW_VLAN_CTAG_RX */
{
    __bdx_vlan_rx_vid(ndev, vid, 0);
    return 0;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 1,0)
//-------------------------------------------------------------------------------------------------
/*
 * bdx_vlan_rx_register - A kernel hook for adding VLAN group
 *
 * @ndev - Network device
 * @grp  - VLAN group
 */
static void
bdx_vlan_rx_register(struct net_device *ndev, struct vlan_group *grp)
{
    struct bdx_priv *priv = netdev_priv(ndev);

    ENTER;
    DBG("device ='%s', group ='%p'\n", ndev->name, grp);
    priv->vlgrp = grp;
    RET();
}
#endif

//-------------------------------------------------------------------------------------------------
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
    ENTER;

    if (new_mtu == ndev->mtu)
        RET(0);

    /* enforce minimum frame size */
    if (new_mtu < ETH_ZLEN)
    {
        ERR("%s mtu %d is less then minimal %d\n", ndev->name, new_mtu, ETH_ZLEN);
        RET(-EINVAL);
    }
    else if (new_mtu > BDX_MAX_MTU)
    {
        ERR("%s mtu %d is greater then max mtu %d\n", ndev->name, new_mtu, BDX_MAX_MTU);
        RET(-EINVAL);
    }

    ndev->mtu = new_mtu;

    if (netif_running(ndev))
    {
        bdx_close(ndev);
        bdx_open(ndev);
    }
    RET(0);
}

//-------------------------------------------------------------------------------------------------

static void bdx_setmulti(struct net_device *ndev)
{
    struct bdx_priv *priv = netdev_priv(ndev);

     u32 rxf_val =
        GMAC_RX_FILTER_AM | GMAC_RX_FILTER_AB | GMAC_RX_FILTER_OSEN | GMAC_RX_FILTER_TXFC;
    int i;

   ENTER;
    /* IMF - imperfect (hash) rx multicast filter */
    /* PMF - perfect rx multicast filter */

    /* FIXME: RXE(OFF) */
    if (ndev->flags & IFF_PROMISC)
    {
        rxf_val |= GMAC_RX_FILTER_PRM;
    }
    else if (ndev->flags & IFF_ALLMULTI)
    {
        /* set IMF to accept all multicast frames */
        for (i = 0; i < MAC_MCST_HASH_NUM; i++)
        {
            WRITE_REG(priv, regRX_MCST_HASH0 + i * 4, ~0);
        }
    }
    else if (netdev_mc_count(ndev))
    {
        u8 hash;
        struct dev_mc_list *mclist;
        u32 reg, val;

        /* Set IMF to deny all multicast frames */
        for (i = 0; i < MAC_MCST_HASH_NUM; i++)
        {
            WRITE_REG(priv, regRX_MCST_HASH0 + i * 4, 0);
        }
        /* Set PMF to deny all multicast frames */
        for (i = 0; i < MAC_MCST_NUM; i++)
        {
            WRITE_REG(priv, regRX_MAC_MCST0 + i * 8, 0);
            WRITE_REG(priv, regRX_MAC_MCST1 + i * 8, 0);
        }

        /* Use PMF to accept first MAC_MCST_NUM (15) addresses */
        /* TBD: Sort the addresses and write them in ascending order into
                 *  RX_MAC_MCST regs. we skip this phase now and accept ALL
         *  multicast frames through IMF. Accept the rest of
                 *      addresses throw IMF.
                 */
        netdev_for_each_mc_addr(mclist, ndev)
        {
            hash = 0;
            for (i = 0; i < ETH_ALEN; i++)
            {
                hash ^= mclist->dmi_addr[i];

            }
            reg = regRX_MCST_HASH0 + ((hash >> 5) << 2);
            val = READ_REG(priv, reg);
            val |= (1 << (hash % 32));
            WRITE_REG(priv, reg, val);
        }

    }
    else
    {
        //DBG("only own mac %d\n", ndev->mc.count);
        rxf_val |= GMAC_RX_FILTER_AB;
    }
    WRITE_REG(priv, regGMAC_RXF_A, rxf_val);
    /* Enable RX */
    /* FIXME: RXE(ON) */

    RET();
}

//-------------------------------------------------------------------------------------------------

static int bdx_set_mac(struct net_device *ndev, void *p)
{
    struct bdx_priv *priv = netdev_priv(ndev);
    struct sockaddr *addr = p;

    ENTER;
    /*
       if (netif_running(dev))
       return -EBUSY
     */
    memcpy(ndev->dev_addr, addr->sa_data, ndev->addr_len);
    bdx_restore_mac(ndev, priv);
    RET(0);
}

//-------------------------------------------------------------------------------------------------

static int bdx_read_mac(struct bdx_priv *priv)
{
    u16 macAddress[3], i;
    ENTER;

    macAddress[2] = READ_REG(priv, regUNC_MAC0_A);
    macAddress[2] = READ_REG(priv, regUNC_MAC0_A);
    macAddress[1] = READ_REG(priv, regUNC_MAC1_A);
    macAddress[1] = READ_REG(priv, regUNC_MAC1_A);
    macAddress[0] = READ_REG(priv, regUNC_MAC2_A);
    macAddress[0] = READ_REG(priv, regUNC_MAC2_A);
    for (i = 0; i < 3; i++)
    {
        priv->ndev->dev_addr[i * 2 + 1] = macAddress[i];
        priv->ndev->dev_addr[i * 2] = macAddress[i] >> 8;
    }
    RET(0);
}

//-------------------------------------------------------------------------------------------------

static u64 bdx_read_l2stat(struct bdx_priv *priv, int reg)
{
    u64 val;

    val  = READ_REG(priv, reg);
    val |= ((u64) READ_REG(priv, reg + 8)) << 32;
    return val;
}

//-------------------------------------------------------------------------------------------------
/*Do the statistics-update work*/

static void bdx_update_stats(struct bdx_priv *priv)
{
    struct bdx_stats *stats = &priv->hw_stats;
    u64 *stats_vector       = (u64 *) stats;
    int i;
    int addr;

    /*Fill HW structure */
    addr = 0x7200;

    /*First 12 statistics - 0x7200 - 0x72B0 */
    for (i = 0; i < 12; i++)
    {
        stats_vector[i] = bdx_read_l2stat(priv, addr);
        addr += 0x10;
    }
    BDX_ASSERT(addr != 0x72C0);

    /* 0x72C0-0x72E0 RSRV */
    addr = 0x72F0;
    for (; i < 16; i++)
    {
        stats_vector[i] = bdx_read_l2stat(priv, addr);
        addr += 0x10;
    }
    BDX_ASSERT(addr != 0x7330);

    /* 0x7330-0x7360 RSRV */
    addr = 0x7370;
    for (; i < 19; i++)
    {
        stats_vector[i] = bdx_read_l2stat(priv, addr);
        addr += 0x10;
    }
    BDX_ASSERT(addr != 0x73A0);

    /* 0x73A0-0x73B0 RSRV */
    addr = 0x73C0;
    for (; i < 23; i++)
    {
        stats_vector[i] = bdx_read_l2stat(priv, addr);
        addr += 0x10;
    }

    BDX_ASSERT(addr != 0x7400);
    BDX_ASSERT((sizeof(struct bdx_stats) / sizeof(u64)) != i);
}

//-------------------------------------------------------------------------------------------------

static struct net_device_stats *bdx_get_stats(struct net_device *ndev)
{
    struct bdx_priv *priv             = netdev_priv(ndev);
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
	if (db->pkt)
	{
		vfree(db->pkt);
	}
    if (db)
    {
        vfree(db);
    }
}

//-------------------------------------------------------------------------------------------------

static struct rxdb *bdx_rxdb_create(int nelem, u16 pktSize)
{
    struct rxdb *db;
    int i;
    size_t size = sizeof(struct rxdb) + (nelem * sizeof(int)) +
                  (  nelem * sizeof(struct rx_map));

    db = vmalloc(size);
    if (likely(db != NULL))
    {
        memset(db, 0, size);
        db->stack = (int *)(db + 1);
        db->elems = (void *)(db->stack + nelem);
        db->nelem = nelem;
        db->top = nelem;
        for (i = 0; i < nelem; i++)
        {
            db->stack[i] = nelem - i - 1; /* To make the first
                                                       * alloc close to db
                                                       * struct */
        }
    }
    if ((db->pkt = vmalloc(pktSize)) == NULL)
    {
    	bdx_rxdb_destroy(db);
    	db = NULL;
    }


    return db;
}

//-------------------------------------------------------------------------------------------------

static inline int bdx_rxdb_alloc_elem(struct rxdb *db)
{
    BDX_ASSERT(db->top <= 0);
    TN40_ASSERT((db->top > 0), "top %d\n",db->top);
    return db->stack[--(db->top)];
}

//-------------------------------------------------------------------------------------------------

static inline void *bdx_rxdb_addr_elem(struct rxdb *db, unsigned n)
{
    BDX_ASSERT((n >= (unsigned)db->nelem));
    TN40_ASSERT((n < (unsigned)db->nelem), "n %d nelem %d\n",n , db->nelem);
    return db->elems + n;
}

//-------------------------------------------------------------------------------------------------

static inline int bdx_rxdb_available(struct rxdb *db)
{
    return db->top;
}

//-------------------------------------------------------------------------------------------------

static inline void bdx_rxdb_free_elem(struct rxdb *db, unsigned n)
{
    BDX_ASSERT((n >= (unsigned)db->nelem));
    db->stack[(db->top)++] = n;
}

/*************************************************************************
 *     Rx Engine                             *
 *************************************************************************/
#ifdef USE_PAGED_BUFFERS

//-------------------------------------------------------------------------------------------------

static void bdx_rx_vlan(struct bdx_priv *priv, struct sk_buff *skb, u32 rxd_val1, u16 rxd_vlan)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 1,0)
	if (priv->vlgrp && GET_RXD_VTAG(rxd_val1))/* Vlan case */
	{
		vlan_gro_frags(&priv->napi, priv->vlgrp, GET_RXD_VLAN_TCI(rxd_vlan));
	}
#elif  LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	if (GET_RXD_VTAG(rxd_val1))/* Vlan case */
	{
		__vlan_hwaccel_put_tag(skb, le16_to_cpu(GET_RXD_VLAN_TCI(rxd_vlan)));
	}
#else
	if (GET_RXD_VTAG(rxd_val1))/* Vlan case */
	{
		__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), le16_to_cpu(GET_RXD_VLAN_TCI(rxd_vlan)));
	}
#endif
} // bdx_rx_vlan()

#ifdef RX_REUSE_PAGES

//-------------------------------------------------------------------------------------------------

static void dbg_printRxPage(char newLine, struct bdx_page *bdx_page)
{
	if (g_dbg)
	{
		if (newLine == '\n')
		{
			printk("\n");
		}
		if (bdx_page)
		{
			printk(KERN_CONT "bxP %p P %p C %d bxC %d R %d %c\n",
					bdx_page, bdx_page->page, page_count(bdx_page->page), bdx_page->ref_count, bdx_page->reuse_tries, bdx_page->status);
		}
		else
		{
			printk(KERN_CONT "NULL page\n");
		}
	}
} // dbg_printRxPage()

//-------------------------------------------------------------------------------------------------

static void dbg_printRxPageTable(struct bdx_priv *priv)
{
	int j;
	struct bdx_page	*bdx_page;
	if (g_dbg && priv->rx_page_table.bdx_pages)
	{
		printk("rx_page_table\n");
		printk("=============\n");
		printk("nPages %d pSize %d buf size %d bufs in page %d\n",
				priv->rx_page_table.nPages, priv->rx_page_table.page_size, priv->rx_page_table.buf_size, priv->rx_page_table.nBufInPage);
		printk("nFrees %d max_frees %d\n",priv->rx_page_table.nFrees, priv->rx_page_table.max_frees);
		for (j = 0; j < priv->rx_page_table.nPages; j++)
		{
			bdx_page = &priv->rx_page_table.bdx_pages[j];
			printk("%3d. ",j);
			dbg_printRxPage(0, bdx_page);
		}
		printk("\n");
	}

} // dbg_rx_printPageTable()

//-------------------------------------------------------------------------------------------------

static inline struct bdx_page *bdx_rx_page(struct rx_map *dm)
{
	return dm->bdx_page;

} // bdx_get_dm_page

//-------------------------------------------------------------------------------------------------

static int bdx_rx_set_page_size(int buf_size, int *page_used)
{
	int	page_unused   = buf_size;
	int min_page_size = PAGE_SIZE * (1 << get_order(buf_size));
	int	page_size 	  = 0;
	int	psz;

	for (psz = LUXOR__MAX_PAGE_SIZE; psz >= min_page_size; psz >>= 1)
	{
		int unused = psz - ((psz / buf_size) * buf_size);
		if ( unused <= page_unused)
		{
			page_unused = unused;
			page_size   = psz;
		}
		DBG("psz %d page_size %d unused %d \n", psz, page_size, unused);
	}
	*page_used = page_size - page_unused;

	return page_size;

} // bdx_rx_set_page_size()

//-------------------------------------------------------------------------------------------------

static int bdx_rx_get_page_size(struct bdx_priv *priv)
{
	return priv->rx_page_table.page_size;

} // bdx_rx_get_page_size()

//-------------------------------------------------------------------------------------------------

static int bdx_rx_alloc_page(struct bdx_priv *priv, struct bdx_page *bdx_page)
{
	int rVal = -1;

	bdx_page->page = alloc_pages(priv->rx_page_table.gfp_mask, priv->rx_page_table.page_order);
	if (likely(bdx_page->page != NULL))
	{
		get_page(bdx_page->page);
		bdx_page->ref_count = 1;
		bdx_page->dma = pci_map_page(priv->pdev, bdx_page->page, 0L, priv->rx_page_table.page_size, PCI_DMA_FROMDEVICE);
		if (!pci_dma_mapping_error(priv->pdev,bdx_page->dma))
		{
			rVal = 0;
		}
		else
		{
			__free_pages(bdx_page->page, priv->rx_page_table.page_order);
			ERR("Failed to map page %p !\n", bdx_page->page);
		}
	}

	return rVal;

} // bdx_rx_alloc_page()

//-------------------------------------------------------------------------------------------------

static int bdx_rx_alloc_pages(struct bdx_priv *priv)
{
	int					page_used, nPages, j;
	struct rxf_fifo 	*f 			= &priv->rxf_fifo0;
	struct rxdb 		*db			= priv->rxdb0;
	int 				buf_size 	= ROUND_UP(f->m.pktsz, SMP_CACHE_BYTES);
	int					rVal 		= -1;

	do
	{
//		buf_size = 4000; /// Debug
		priv->rx_page_table.page_size = bdx_rx_set_page_size(buf_size, &page_used);
		nPages = (db->nelem * buf_size + (page_used -1)) / page_used;
		priv->rx_page_table.gfp_mask = GFP_ATOMIC | __GFP_NOWARN;
		if (priv->rx_page_table.page_size > PAGE_SIZE)
		{
			priv->rx_page_table.gfp_mask |= __GFP_COMP;
		}
		priv->rx_page_table.page_order = get_order(priv->rx_page_table.page_size);
		DBG("buf_size %d nelem %d page_size %d page_used %d nPages %d\n", buf_size, db->nelem, priv->rx_page_table.page_size, page_used, nPages);
		priv->rx_page_table.bdx_pages = vmalloc(sizeof(struct bdx_page) * nPages);
		if (priv->rx_page_table.bdx_pages == NULL)
		{
			ERR("Cannot allocate page table !\n");
			break;
		}
		else
		{
			memset(priv->rx_page_table.bdx_pages, 0, sizeof(struct bdx_page) * nPages);
		}
		INIT_LIST_HEAD(&priv->rx_page_table.free_list);
		for (j = 0; j < nPages; j++)
		{
			struct bdx_page *bdx_page = &priv->rx_page_table.bdx_pages[j];
			bdx_page->page_index = j;
			if (bdx_rx_alloc_page(priv, bdx_page) == 0)
			{
				list_add_tail(&bdx_page->free, &priv->rx_page_table.free_list);
				bdx_page->status = 'F';
				priv->rx_page_table.nFrees += 1;
			}
			else
			{
				ERR("Allocated only %d pages out of %d\n", j, nPages);
				nPages = j;
				break;
			}
		}
		priv->rx_page_table.nPages     = nPages;
		priv->rx_page_table.buf_size   = buf_size;
		priv->rx_page_table.nBufInPage = priv->rx_page_table.page_size / buf_size;
		priv->rx_page_table.max_frees  = priv->rx_page_table.nPages / 2;
		if (priv->rx_page_table.nPages)
		{
			rVal = 0;
		}

	} while (0);

	return rVal;

} // bdx_rx_alloc_pages()

//-------------------------------------------------------------------------------------------------

static void bdx_rx_free_bdx_page(struct bdx_priv *priv, struct bdx_page	*bdx_page)
{

	pci_unmap_page(priv->pdev, bdx_page->dma, priv->rx_page_table.page_size, PCI_DMA_FROMDEVICE);
	put_page(bdx_page->page);

} // bdx_rx_free_bdx_page()

//-------------------------------------------------------------------------------------------------

static void bdx_rx_free_page(struct bdx_priv *priv, struct bdx_page	*bdx_page)
{
	int j;

	bdx_rx_free_bdx_page(priv, bdx_page);
	for (j = 0; j < bdx_page->ref_count; j++)
	{
		put_page(bdx_page->page);
	}

} // bdx_rx_free_page()

//-------------------------------------------------------------------------------------------------

static void bdx_rx_free_pages(struct bdx_priv *priv)
{
	int j;

	dbg_printRxPageTable(priv);
	if (priv->rx_page_table.bdx_pages != NULL)
	{
		for (j = 0; j < priv->rx_page_table.nPages; j++)
		{
			bdx_rx_free_page(priv, &priv->rx_page_table.bdx_pages[j]);
		}
		vfree(priv->rx_page_table.bdx_pages);
		priv->rx_page_table.bdx_pages = NULL;
	}

} // bdx_rx_free_pages()

//-------------------------------------------------------------------------------------------------

static void bdx_rx_ref_page(struct bdx_page *bdx_page)
{
	get_page(bdx_page->page);
	bdx_page->ref_count += 1;

} // bdx_rx_ref_page()

//-------------------------------------------------------------------------------------------------

static struct bdx_page *bdx_rx_get_page(struct bdx_priv *priv)
{
	int 			nLoops = 0;
	struct bdx_page	*rPage = NULL, *firstPage;
	struct list_head* pos, *temp;

	if (!list_empty(&priv->rx_page_table.free_list))
	{
		list_for_each_safe(pos, temp, &priv->rx_page_table.free_list)
		{
			if (likely(page_count(((struct bdx_page*)pos)->page) == 2))
			{
				rPage = ((struct bdx_page*)pos);
				DBG("nLoops %d ", nLoops);
				dbg_printRxPage(0, rPage);
				list_del(pos);
				rPage->status = ' ';
				priv->rx_page_table.nFrees -= 1;
				bdx_rx_ref_page(rPage);
				break;
			}
			else
			{
				((struct bdx_page*)pos)->reuse_tries += 1;
			}
			nLoops += 1;
		}
		if ((rPage == NULL) && (priv->rx_page_table.nFrees > priv->rx_page_table.max_frees))
		{
			firstPage = list_first_entry(&priv->rx_page_table.free_list, struct bdx_page, free);
			DBG("Replacing - loops %d nFrees %d\n",nLoops,priv->rx_page_table.nFrees);
			dbg_printRxPage('\n', firstPage);
			bdx_rx_free_bdx_page(priv, firstPage);
			if (bdx_rx_alloc_page(priv, firstPage) == 0)
			{
				rPage = firstPage;
				list_del((struct list_head*)rPage);
				rPage->status = ' ';
				priv->rx_page_table.nFrees -= 1;
				bdx_rx_ref_page(rPage);
			}
			else
			{
				MSG("Page alloc failed nFrees %d max_frees %d order %d\n",
					priv->rx_page_table.nFrees, priv->rx_page_table.max_frees, priv->rx_page_table.page_order);
			}
		}
	}

	return rPage;

} // bdx_rx_get_page()


//-------------------------------------------------------------------------------------------------

static void bdx_rx_reuse_page(struct bdx_priv *priv, struct rx_map *dm)
{
    if (--dm->bdx_page->ref_count == 1)
    {
    	dm->bdx_page->reuse_tries = 0;
		list_add_tail(&dm->bdx_page->free, &priv->rx_page_table.free_list);
		dm->bdx_page->status = 'F';
		priv->rx_page_table.nFrees += 1;
    }

} // bdx_rx_reuse_page()

//-------------------------------------------------------------------------------------------------

static void bdx_rx_put_page(struct bdx_priv *priv, struct rx_map *dm)
{
	// DO NOTHING

} // bdx_rx_put_page()

//-------------------------------------------------------------------------------------------------

static void bdx_rx_set_dm_page(register struct rx_map *dm, struct bdx_page *bdx_page)
{
	dm->bdx_page = bdx_page;

} // bdx_rx_set_dm_page()

#else // USE_PAGED_BUFFERS without RX_REUSE_PAGES

//-------------------------------------------------------------------------------------------------

static inline struct bdx_page *bdx_rx_page(struct rx_map *dm)
{
	return &dm->bdx_page;

} // bdx_get_dm_page

//-------------------------------------------------------------------------------------------------

static int bdx_rx_alloc_pages(struct bdx_priv *priv)
{
    struct rxf_fifo *f	 = &priv->rxf_fifo0;
	int				rVal = 0;

	priv->rx_page_table.buf_size   = ROUND_UP(f->m.pktsz, SMP_CACHE_BYTES);
	priv->rx_page_table.bdx_pages  = vmalloc(sizeof(struct bdx_page));
	if (priv->rx_page_table.bdx_pages == NULL)
	{
		ERR("Cannot allocate page table !\n");
		rVal = -1;
	}

	return rVal;

} // bdx_rx_alloc_pages()

//-------------------------------------------------------------------------------------------------

static void bdx_rx_free_pages(struct bdx_priv *priv)
{

	vfree(priv->rx_page_table.bdx_pages);

} // bdx_rx_free_pages()

//-------------------------------------------------------------------------------------------------

static struct bdx_page *bdx_rx_get_page(struct bdx_priv *priv)
{
	gfp_t 					gfp_mask;
	int 					page_size = priv->rx_page_table.page_size;
	struct bdx_page			*bdx_page = priv->rx_page_table.bdx_pages;

    gfp_mask  = GFP_ATOMIC | __GFP_NOWARN;
    if (page_size > PAGE_SIZE)
    {
    	gfp_mask |= __GFP_COMP;
    }
    bdx_page->page = alloc_pages(gfp_mask, get_order(page_size));
	if (likely(bdx_page->page != NULL))
	{
		DBG("map page %p size %d\n",bdx_page->page, page_size);
		bdx_page->dma = pci_map_page(priv->pdev, bdx_page->page, 0L, page_size, PCI_DMA_FROMDEVICE);
		if (unlikely(pci_dma_mapping_error(priv->pdev,bdx_page->dma)))
		{
			ERR("Failed to map page %p !\n", bdx_page->page);
			__free_pages(bdx_page->page, get_order(page_size));
			bdx_page = NULL;
		}
	}
	else
	{
		bdx_page = NULL;
	}

    return bdx_page;

} // bdx_rx_get_page()

//-------------------------------------------------------------------------------------------------

static int bdx_rx_get_page_size(struct bdx_priv *priv)
{
    struct rxdb	*db	= priv->rxdb0;
    int 		dno = bdx_rxdb_available(db) - 1;

	priv->rx_page_table.page_size  = MIN(LUXOR__MAX_PAGE_SIZE, dno * priv->rx_page_table.buf_size);

	return priv->rx_page_table.page_size;

} // bdx_rx_get_page_size()

//-------------------------------------------------------------------------------------------------

static void bdx_rx_reuse_page(struct bdx_priv *priv, struct rx_map *dm)
{

	DBG("dm size %d off %d dma %p\n", dm->size, dm->off, (void *)dm->dma);
    if (dm->off == 0)
    {
    	DBG("umap page %p size %d\n",(void *)dm->dma, dm->size);
        pci_unmap_page(priv->pdev, dm->dma, dm->size, PCI_DMA_FROMDEVICE);
    }

} // bdx_rx_reuse_page()

//-------------------------------------------------------------------------------------------------

static void bdx_rx_ref_page(struct bdx_page *bdx_page)
{
	get_page(bdx_page->page);

} // bdx_rx_ref_page()

//-------------------------------------------------------------------------------------------------

static void bdx_rx_put_page(struct bdx_priv *priv, struct rx_map *dm)
{
	if (dm->off == 0)
	{
		pci_unmap_page(priv->pdev, dm->dma, dm->size, PCI_DMA_FROMDEVICE);
	}
	put_page(dm->bdx_page.page);

} // bdx_rx_put_page()

//-------------------------------------------------------------------------------------------------

static void bdx_rx_set_dm_page(register struct rx_map *dm, struct bdx_page *bdx_page)
{
	dm->bdx_page.page = bdx_page->page;


} // bdx_rx_set_dm_page()

#endif // RX_REUSE_PAGES

#else	// not USE_PAGED_BUFFERS and not RX_REUSE_PAGES

//-------------------------------------------------------------------------------------------------

static void bdx_rx_vlan(struct bdx_priv *priv, struct sk_buff *skb, u32 rxd_val1, u16 rxd_vlan)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 1,0)
		if (priv->vlgrp && GET_RXD_VTAG(rxd_val1))/* Vlan case */
		{
		   LUXOR__VLAN_RECEIVE(&priv->napi, priv->vlgrp, GET_RXD_VLAN_TCI(rxd_vlan),skb);
		}
		else                       /* Regular case */
#elif  LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
		if (GET_RXD_VTAG(rxd_val1))/* Vlan case */
		{
			__vlan_hwaccel_put_tag(skb, le16_to_cpu(GET_RXD_VLAN_TCI(rxd_vlan)));
		}
#else
		if (GET_RXD_VTAG(rxd_val1))/* Vlan case */
		{
			__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), le16_to_cpu(GET_RXD_VLAN_TCI(rxd_vlan)));
		}
#endif
} // bdx_rx_vlan()

//-------------------------------------------------------------------------------------------------

static int 						bdx_rx_get_page_size(struct bdx_priv *priv) {return 0;}
static void 					bdx_rx_put_page		(struct bdx_priv *priv, struct rx_map *dm) {}
static int 						bdx_rx_alloc_pages	(struct bdx_priv *priv) {return 0;}
static void 					bdx_rx_free_pages	(struct bdx_priv *priv) {}
static inline struct bdx_page 	*bdx_rx_page		(struct rx_map *dm) 	{return NULL;}

#endif // RX_REUSE_PAGES
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
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
    ENTER;

    if (bdx_fifo_init(priv, &priv->rxd_fifo0.m, priv->rxd_size,
                      regRXD_CFG0_0, regRXD_CFG1_0,
                      regRXD_RPTR_0, regRXD_WPTR_0))
        goto err_mem;
    if (bdx_fifo_init(priv, &priv->rxf_fifo0.m, priv->rxf_size,
                      regRXF_CFG0_0, regRXF_CFG1_0,
                      regRXF_RPTR_0, regRXF_WPTR_0))
        goto err_mem;
    priv->rxf_fifo0.m.pktsz = priv->ndev->mtu + VLAN_ETH_HLEN;
    if (!(priv->rxdb0 = bdx_rxdb_create(priv->rxf_fifo0.m.memsz / sizeof(struct rxf_desc), priv->rxf_fifo0.m.pktsz)))
        goto err_mem;

    return 0;

err_mem:
    ERR("%s: Rx init failed\n", priv->ndev->name);
    return -ENOMEM;
}

//-------------------------------------------------------------------------------------------------
/* bdx_rx_free_buffers - Free and unmap all the fifo allocated buffers.
 *
 * @priv - NIC private structure
 * @f    - RXF fifo
 */
static void bdx_rx_free_buffers(struct bdx_priv *priv, struct rxdb *db,
                                struct rxf_fifo *f)
{
    struct rx_map 	*dm;
    u16 i;

    ENTER;
    DBG("total =%d free =%d busy =%d\n", db->nelem, bdx_rxdb_available(db), db->nelem - bdx_rxdb_available(db));
    while (bdx_rxdb_available(db) > 0)
    {
        i       = bdx_rxdb_alloc_elem(db);
        dm      = bdx_rxdb_addr_elem(db, i);
        dm->dma = 0;
    }
    for (i = 0; i < db->nelem; i++)
    {
        dm       = bdx_rxdb_addr_elem(db, i);
        if (dm->dma)
        {
            if (dm->skb)
            {
                pci_unmap_single(priv->pdev, dm->dma, f->m.pktsz, PCI_DMA_FROMDEVICE);
                dev_kfree_skb(dm->skb);
            }
            else
            {
            	struct bdx_page *bdx_page = bdx_rx_page(dm);
				if (bdx_page)
				{
					bdx_rx_put_page(priv, dm);
				}
			}
        }
    }

} // bdx_rx_free_buffers()

//-------------------------------------------------------------------------------------------------
/* bdx_rx_free - Release all Rx resources.
 *
 * @priv       - NIC private structure
 *
 * Note: This functuion assumes that Rx is disabled in HW.
 */
static void bdx_rx_free(struct bdx_priv *priv)
{
    ENTER;
    if (priv->rxdb0)
    {
        bdx_rx_free_buffers(priv, priv->rxdb0, &priv->rxf_fifo0);
        bdx_rxdb_destroy(priv->rxdb0);
        priv->rxdb0 = NULL;
    }
    bdx_fifo_free(priv, &priv->rxf_fifo0.m);
    bdx_fifo_free(priv, &priv->rxd_fifo0.m);

    RET();
}


//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------

static void _bdx_rx_alloc_buffers(struct bdx_priv *priv)
{
    int 						dno, delta, idx;
    register struct rxf_desc 	*rxfd;
    register struct rx_map 		*dm;
    int 						page_size;
    struct rxdb 				*db			= priv->rxdb0;
    struct rxf_fifo 			*f			= &priv->rxf_fifo0;
    int							nPages		= 0;
#ifdef USE_PAGED_BUFFERS
    struct bdx_page 			*bdx_page	= NULL;
    int 						buf_size	= priv->rx_page_table.buf_size;
    int							page_off 	= -1;
    u64 						dma   		= 0ULL;
#else
	struct sk_buff 				*skb  		= NULL;
#endif

    ENTER;

    DBG("_bdx_rx_alloc_buffers is at %p\n",_bdx_rx_alloc_buffers);
    dno = bdx_rxdb_available(db) - 1;
    page_size = bdx_rx_get_page_size(priv);
    DBG("dno %d page_size %d buf_size %d\n", dno, page_size, priv->rx_page_table.buf_size);
    while (dno > 0)
    {
#ifdef USE_PAGED_BUFFERS

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
		if (unlikely(page_off < 0))
		{
			bdx_page = bdx_rx_get_page(priv);
			if (bdx_page  == NULL)
			{
				u32 timeout;
				timeout = 1000000;      // 1/5 sec
				WRITE_REG(priv, 0x5154,timeout);
				DBG("warning - system memory is temporary low\n");
				break;
			}
			page_off   = ((page_size / buf_size) - 1)  * buf_size;
			dma = bdx_page->dma;
			nPages += 1;
		}
		else
		{
			bdx_rx_ref_page(bdx_page);
			//TN40_ASSERT(bdx_page->ref_count <= 1 + priv->rx_page_table.nBufInPage, "page %p ref %d\n", bdx_page->page, bdx_page->ref_count);
			/*
			 * Page is already allocated and mapped, just
			 * increment the page usage count.
			 */
		}
        rxfd     = (struct rxf_desc *)(f->m.va + f->m.wptr);
        idx      = bdx_rxdb_alloc_elem(db);
        dm       = bdx_rxdb_addr_elem(db, idx);
		dm->size = page_size;
		bdx_rx_set_dm_page(dm, bdx_page);
		dm->off   = page_off;
		dm->dma   = dma + page_off;
		DBG("dm size %d off %d dma %p\n", dm->size, dm->off, (void *)dm->dma);
		page_off -= buf_size;
		//TN40_ASSERT(dm->bdx_page->status == ' ',"bxP %p s %c\n", dm->bdx_page, dm->bdx_page->status);

#else
        rxfd     = (struct rxf_desc *)(f->m.va + f->m.wptr);
        idx      = bdx_rxdb_alloc_elem(db);
        dm       = bdx_rxdb_addr_elem(db, idx);
		if (!(skb = netdev_alloc_skb(priv->ndev, f->m.pktsz + SMP_CACHE_BYTES)))
		{
			ERR("NO MEM: dev_alloc_skb failed\n");
			break;
		}
		skb->dev = priv->ndev;
		skb_reserve(skb, (PTR_ALIGN(skb->data, SMP_CACHE_BYTES) - skb->data));
		dm->skb  = skb;
		dm->dma  = pci_map_single(priv->pdev, skb->data, f->m.pktsz, PCI_DMA_FROMDEVICE);
		nPages 	+= 1;
#endif    // USE_PAGED_BUFFERS
        rxfd->info  = CPU_CHIP_SWAP32(0x10003); /* INFO =1 BC =3 */
        rxfd->va_lo = idx;
        rxfd->pa_lo = CPU_CHIP_SWAP32(L32_64(dm->dma));
        rxfd->pa_hi = CPU_CHIP_SWAP32(H32_64(dm->dma));
        rxfd->len   = CPU_CHIP_SWAP32(f->m.pktsz);
        print_rxfd(rxfd);
        f->m.wptr += sizeof(struct rxf_desc);
        delta      = f->m.wptr - f->m.memsz;
        if (unlikely(delta >= 0))
        {
            f->m.wptr = delta;
            if (delta > 0)
            {
                memcpy(f->m.va, f->m.va + f->m.memsz, delta);
                DBG("Wrapped descriptor\n");
            }
        }
        dno--;
    }
    DBG("nPages %d\n", nPages);
//    if (nPages > 0)
//    {
		WRITE_REG(priv, f->m.reg_WPTR, f->m.wptr & TXF_WPTR_WR_PTR);
		DBG("WRITE_REG 0x%04x f->m.reg_WPTR 0x%x\n", f->m.reg_WPTR, f->m.wptr & TXF_WPTR_WR_PTR);
		DBG("READ_REG  0x%04x f->m.reg_RPTR=0x%x\n", f->m.reg_RPTR, READ_REG(priv, f->m.reg_RPTR));
		DBG("READ_REG  0x%04x f->m.reg_WPTR=0x%x\n", f->m.reg_WPTR, READ_REG(priv, f->m.reg_WPTR));
		dbg_printFifo(&priv->rxf_fifo0.m, (char *)"RXF");
//    }

    RET();

} // _bdx_rx_alloc_buffers()

//-------------------------------------------------------------------------------------------------

static void bdx_rx_alloc_buffers(struct bdx_priv *priv)
{
	_bdx_rx_alloc_buffers(priv);

}	// bdx_rx_alloc_buffers()

//-------------------------------------------------------------------------------------------------

static void bdx_recycle_skb(struct bdx_priv *priv, struct rxd_desc *rxdd)
{
    struct rxdb 	*db 		= priv->rxdb0;
    struct rx_map 	*dm 		= bdx_rxdb_addr_elem(db, rxdd->va_lo);
    struct rxf_fifo *f  		= &priv->rxf_fifo0;
    struct rxf_desc *rxfd		= (struct rxf_desc *)(f->m.va + f->m.wptr);
    int 			delta;

    ENTER;

    rxfd->info  = CPU_CHIP_SWAP32(0x10003); /* INFO=1 BC=3 */
    rxfd->va_lo = rxdd->va_lo;
    rxfd->pa_lo = CPU_CHIP_SWAP32(L32_64(dm->dma));
    rxfd->pa_hi = CPU_CHIP_SWAP32(H32_64(dm->dma));
    rxfd->len   = CPU_CHIP_SWAP32(f->m.pktsz);
    print_rxfd(rxfd);
    f->m.wptr  += sizeof(struct rxf_desc);
    delta       = f->m.wptr - f->m.memsz;
    if (unlikely(delta >= 0))
    {
        f->m.wptr = delta;
        if (delta > 0)
        {
            memcpy(f->m.va, f->m.va + f->m.memsz, delta);
            DBG("wrapped descriptor\n");
        }
    }

    RET();

} // bdx_recycle_skb()

//-------------------------------------------------------------------------------------------------

static inline u16 tcpCheckSum(u16 *buf, u16 len, u16 *saddr, u16 *daddr, u16 proto)
{
	u32 		sum;
	u16			j = len;

	sum = 0;
	while (j > 1)
	{
		sum += *buf++;
		if (sum & 0x80000000)
		{
			sum = (sum & 0xFFFF) + (sum >> 16);
		}
		j -= 2;
	}
	if ( j & 1 )
	{
		sum += *((u8 *)buf);
	}
	// Add the tcp pseudo-header
	sum += *(saddr++);
	sum += *saddr;
	sum += *(daddr++);
	sum += *daddr;
	sum += __constant_htons(proto);
	sum += __constant_htons(len);
	// Fold 32-bit sum to 16 bits
	while (sum >> 16)
	{
			sum = (sum & 0xFFFF) + (sum >> 16);
	}
	// One's complement of sum

	//return ( (u16)(~sum)  );

	return ( (u16)(sum)  );

} // tcpCheckSum()

//-------------------------------------------------------------------------------------------------

#if defined(USE_PAGED_BUFFERS)
static void bdx_skb_add_rx_frag(struct sk_buff *skb, int i, struct page *page, int off, int len)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0) || (defined(RHEL_RELEASE_CODE) && (RHEL_RELEASE_CODE > 1539))
			skb_add_rx_frag(skb, 0, page, off, len,  SKB_TRUESIZE(len));
#else
	#if (!defined SKB_DATA_ALIGN)
		#define SKB_DATA_ALIGN(X)	(((X) + (SMP_CACHE_BYTES - 1)) & ~(SMP_CACHE_BYTES - 1))
	#endif
	#if (!defined SKB_TRUESIZE)
		#define SKB_TRUESIZE(X) ((X) + SKB_DATA_ALIGN(sizeof(struct sk_buff)) +	SKB_DATA_ALIGN(sizeof(struct skb_shared_info)))
	#endif
		skb_fill_page_desc(skb, i, page, off, len);
		skb->len      += len;
		skb->data_len += len;
		skb->truesize += SKB_TRUESIZE(len);
	#endif
}
#endif

//-------------------------------------------------------------------------------------------------

#define PKT_ERR_LEN		(70)

static int bdx_rx_error(char *pkt, u32 rxd_err, u16 len)
{
	struct ethhdr *eth = (struct ethhdr *)pkt;
	struct iphdr  *iph = (struct iphdr *) (pkt +  sizeof(struct ethhdr) + ((eth->h_proto == __constant_htons(ETH_P_8021Q)) ? VLAN_HLEN : 0));
	int 		  rVal = 1;

	if (rxd_err  == 0x8) 					  		// UDP checksum error
	{
		struct udphdr *udp = (struct udphdr *)((u8 *)iph + sizeof(struct iphdr));
		if (udp->check == 0)
		{
			DBG("false rxd_err = 0x%x\n", rxd_err);
			rVal = 0;           					// Work around H/W false error indication
		}
		else if (len < PKT_ERR_LEN)
		{
			u16 udpSum;
			udpSum = tcpCheckSum((u16 *)udp, htons(iph->tot_len) - (iph->ihl * sizeof(u32)), (u16 *)&iph->saddr, (u16 *)&iph->daddr, IPPROTO_UDP);
			if (udpSum == 0xFFFF)
			{
				DBG("false rxd_err = 0x%x\n", rxd_err);
				rVal = 0;           				// Work around H/W false error indication
			}
		}
	}
	else if ((rxd_err == 0x10) && (len < PKT_ERR_LEN))     	// TCP checksum error
	{
		u16 tcpSum;
		struct tcphdr *tcp = (struct tcphdr *)((u8 *)iph + sizeof(struct iphdr));
		tcpSum = tcpCheckSum((u16 *)tcp, htons(iph->tot_len) - (iph->ihl * sizeof(u32)), (u16 *)&iph->saddr, (u16 *)&iph->daddr, IPPROTO_TCP);
		if (tcpSum == 0xFFFF)
		{
			DBG("false rxd_err = 0x%x\n", rxd_err);
			rVal = 0;           					// Work around H/W false error indication
		}
	}

	return rVal;

} // bdx_rx_error()

//-------------------------------------------------------------------------------------------------
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
    struct sk_buff 	*skb;
    struct rxd_desc *rxdd;
    struct rx_map 	*dm;
    struct bdx_page *bdx_page;
    struct rxf_fifo *rxf_fifo;
    u32 			rxd_val1, rxd_err;
    u16 			len;
    u16 			rxd_vlan;
    u32 			pkt_id;
    int 			tmp_len, size;
	char 			*pkt;
    int 			done	= 0;
    struct rxdb 	*db 	= NULL;

    ENTER;

#if (!defined(RHEL_RELEASE_CODE) && (LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)) || (defined(RHEL_RELEASE_CODE) && (RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(7,5)))) 
    priv->ndev->last_rx = jiffies;
#endif
    f->m.wptr       = READ_REG(priv, f->m.reg_WPTR) & TXF_WPTR_WR_PTR;
    size = f->m.wptr - f->m.rptr;
    if (size < 0)
    {
        size += f->m.memsz;     /* Size is negative :-) */
    }
    while (size > 0)
    {
        rxdd = (struct rxd_desc *)(f->m.va + f->m.rptr);
        db   = priv->rxdb0;

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
        tmp_len  = GET_RXD_BC(rxd_val1) << 3;
        pkt_id   = GET_RXD_PKT_ID(rxd_val1);
        BDX_ASSERT(tmp_len <= 0);
        size    -= tmp_len;
        /* CHECK FOR A PARTIALLY ARRIVED DESCRIPTOR */
        if (size < 0)
        {
            DBG("bdx_rx_receive() PARTIALLY ARRIVED DESCRIPTOR tmp_len %d\n", tmp_len);
            break;
        }
        /* HAVE WE REACHED THE END OF THE QUEUE? */
        f->m.rptr += tmp_len;
        tmp_len    = f->m.rptr - f->m.memsz;
        if (unlikely(tmp_len >= 0))
        {
            f->m.rptr = tmp_len;
            if (tmp_len > 0)
            {
                /* COPY PARTIAL DESCRIPTOR TO THE END OF THE QUEUE */
                DBG("wrapped desc rptr=%d tmp_len=%d\n", f->m.rptr, tmp_len);
                memcpy(f->m.va + f->m.memsz, f->m.va, tmp_len);
            }
        }
        dm = bdx_rxdb_addr_elem(db, rxdd->va_lo);
        prefetch(dm);
        bdx_page = bdx_rx_page(dm);
        //TN40_ASSERT(bdx_page->status == ' ',"bxP %p status %c\n", bdx_page, bdx_page->status);
        //TN40_ASSERT(dm->idx == rxdd->va_lo ,"bxP %p idx %d vlo %d\n", bdx_page, dm->idx, rxdd->va_lo);
        len  	 = CPU_CHIP_SWAP16(rxdd->len);
        rxd_vlan = CPU_CHIP_SWAP16(rxdd->rxd_vlan);
        print_rxdd(rxdd, rxd_val1, len, rxd_vlan);
        /* CHECK FOR ERRORS */
        if (unlikely(rxd_err = GET_RXD_ERR(rxd_val1)))
        {
            int bErr = 1;

            if (	( !	( rxd_err &  0x4))		&& 								// NOT CRC error
            		(
                		((rxd_err == 0x8)  && (pkt_id == 2))		||    		// UDP checksum error
                		((rxd_err == 0x10) && (len < PKT_ERR_LEN) && (pkt_id == 1))		// TCP checksum error
            		)
          		)
           {
#if defined (USE_PAGED_BUFFERS)
				pkt = ((char *)page_address(bdx_page->page) +  dm->off);
#else
				pkt = db->pkt;
				skb_copy_from_linear_data(dm->skb, pkt, len);
#endif
				bErr = bdx_rx_error(pkt, rxd_err, len);
            }
			if (bErr)
            {
//				pkt = ((char *)page_address(bdx_page->page) +  dm->off);
//				dbg_printPkt(pkt, len);
				ERR("rxd_err = 0x%x\n", rxd_err);
                priv->net_stats.rx_errors++;
                bdx_recycle_skb(priv, rxdd);
                continue;
            }
        }
        DBG("tn40xx: * RX %d *\n",len);
        rxf_fifo = &priv->rxf_fifo0;

#if defined(USE_PAGED_BUFFERS)
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
		if ((skb = napi_get_frags(&priv->napi)) == NULL)
		{
			ERR("napi_get_frags failed\n");
			break;
		}
		skb->ip_summed = (pkt_id == 0) ? CHECKSUM_NONE : CHECKSUM_UNNECESSARY;
		bdx_skb_add_rx_frag(skb, 0, bdx_page->page, dm->off, len);
		bdx_rxdb_free_elem(db, rxdd->va_lo);
		/* PROCESS PACKET */
		bdx_rx_vlan(priv, skb, rxd_val1, rxd_vlan);
		napi_gro_frags(&priv->napi);
		//TN40_ASSERT(dm->bdx_page->ref_count > 1, "page %p count %d\n", dm->bdx_page->page, dm->bdx_page->ref_count);
		bdx_rx_reuse_page(priv, dm);
#else // USE_PAGED_BUFFERS
		{
		struct sk_buff *skb2;

    	/* Handle SKB */
		skb = dm->skb;
		prefetch(skb);
		prefetch(skb->data);
		/* IS THIS A SMALL PACKET? */
		if (len < BDX_COPYBREAK &&
				(skb2 = dev_alloc_skb(len + NET_IP_ALIGN)))
		{
			/* YES, COPY PACKET TO A SMALL SKB AND REUSE THE CURRENT SKB */
			skb_reserve(skb2, NET_IP_ALIGN);
			pci_dma_sync_single_for_cpu(priv->pdev, dm->dma, rxf_fifo->m.pktsz, PCI_DMA_FROMDEVICE);
			memcpy(skb2->data, skb->data, len);
			bdx_recycle_skb(priv, rxdd);
			skb = skb2;
		}
		else
		{
			/* NO, UNMAP THE SKB AND FREE THE FIFO ELEMENT */
			pci_unmap_single(priv->pdev, dm->dma, rxf_fifo->m.pktsz,PCI_DMA_FROMDEVICE);
			bdx_rxdb_free_elem(db, rxdd->va_lo);
		}
		/* UPDATE SKB FIELDS */
		skb_put(skb, len);
		skb->dev       = priv->ndev;
		/*
		 * Note: Non-IP packets aren't checksum-offloaded.
		 */
		skb->ip_summed = (pkt_id == 0) ? CHECKSUM_NONE : CHECKSUM_UNNECESSARY;
		skb->protocol  = eth_type_trans(skb, priv->ndev);
		/* PROCESS PACKET */
		bdx_rx_vlan(priv, skb, rxd_val1, rxd_vlan);
		LUXOR__RECEIVE(&priv->napi, skb);
		}
#endif // USE_PAGED_BUFFERS

#if defined(USE_RSS)

#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0) )
		skb->hash    = CPU_CHIP_SWAP32(rxdd->rss_hash);
#if !defined(RHEL_RELEASE_CODE)
		skb->l4_hash = 1;
#endif
		DBG("rxhash    = 0x%x\n", skb->hash);

#else
		skb->rxhash    = CPU_CHIP_SWAP32(rxdd->rss_hash);
#if !defined(RHEL_RELEASE_CODE)
		skb->l4_rxhash = 1;
#endif
		DBG("rxhash    = 0x%x\n", skb->rxhash);
#endif

#endif // USE_RSS
        priv->net_stats.rx_bytes += len;
/*
// Debug code
	{
		int 	i;
#if defined(USE_PAGED_BUFFERS)
		pkt = ((char *)page_address(bdx_page->page) +  dm->off);
#else
		pkt = db->pkt;
		skb_copy_from_linear_data(dm->skb, pkt, len);
#endif
		dbg_printPkt(pkt, len);
		dbg_printSkb(skb);
	}
// Debug code end
*/
        if (unlikely(++done >= budget))
        {
            break;
        }
    } // while

    /* CLEANUP */
    LUXOR__GRO_FLUSH(&priv->napi);
    priv->net_stats.rx_packets += done;
    /* FIXME: Do something to minimize pci accesses    */
    WRITE_REG(priv, f->m.reg_RPTR, f->m.rptr & TXF_WPTR_WR_PTR);
    bdx_rx_alloc_buffers(priv);

    RET(done);

} // bdx_rx_receive()


/*************************************************************************
 * Debug / Temporary Code                       *
 *************************************************************************/
static void print_rxdd(struct rxd_desc *rxdd, u32 rxd_val1, u16 len,
                       u16 rxd_vlan)
{
    DBG("rxdd bc %d rxfq %d to %d type %d err %d rxp %d "
        "pkt_id %d vtag %d len %d vlan_id %d cfi %d prio %d "
        "va_lo %d va_hi %d\n",
        GET_RXD_BC(rxd_val1), GET_RXD_RXFQ(rxd_val1), GET_RXD_TO(rxd_val1),
        GET_RXD_TYPE(rxd_val1), GET_RXD_ERR(rxd_val1),
        GET_RXD_RXP(rxd_val1), GET_RXD_PKT_ID(rxd_val1),
        GET_RXD_VTAG(rxd_val1), len, GET_RXD_VLAN_ID(rxd_vlan),
        GET_RXD_CFI(rxd_vlan), GET_RXD_PRIO(rxd_vlan), rxdd->va_lo,
        rxdd->va_hi);
}

//-------------------------------------------------------------------------------------------------

static void print_rxfd(struct rxf_desc *rxfd)
{
    //  DBG("=== RxF desc CHIP ORDER/ENDIANESS =============\n"
    //      "info 0x%x va_lo %u pa_lo 0x%x pa_hi 0x%x len 0x%x\n",
    //      rxfd->info, rxfd->va_lo, rxfd->pa_lo, rxfd->pa_hi, rxfd->len);
}

//-------------------------------------------------------------------------------------------------
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
        taken = db->size + 1 + taken;   /* (size + 1) equals memsz */

    return db->size - taken;
}

//-------------------------------------------------------------------------------------------------
/* __bdx_tx_ptr_next - A helper function, increment read/write pointer + wrap.
 *
 * @d   - Tx data base
 * @ptr - Read or write pointer
 */
static inline void __bdx_tx_db_ptr_next(struct txdb *db, struct tx_map **pptr)
{
    BDX_ASSERT(db == NULL || pptr == NULL); /* sanity */
    BDX_ASSERT(*pptr != db->rptr && /* expect either read */
               *pptr != db->wptr);  /* or write pointer */
    BDX_ASSERT(*pptr < db->start || /* pointer has to be */
               *pptr >= db->end);   /* in range */

    ++*pptr;
    if (unlikely(*pptr == db->end))
        *pptr = db->start;
}

//-------------------------------------------------------------------------------------------------
/* bdx_tx_db_inc_rptr - Increment the read pointer.
 *
 * @d - tx data base
 */
static inline void bdx_tx_db_inc_rptr(struct txdb *db)
{
    BDX_ASSERT(db->rptr == db->wptr);   /* can't read from empty db */
    __bdx_tx_db_ptr_next(db, &db->rptr);
}

//-------------------------------------------------------------------------------------------------
/* bdx_tx_db_inc_rptr - Increment the   write pointer.
 *
 * @d - tx data base
 */
static inline void bdx_tx_db_inc_wptr(struct txdb *db)
{
    __bdx_tx_db_ptr_next(db, &db->wptr);
    BDX_ASSERT(db->rptr == db->wptr);   /* we can not get empty db as
                           a result of write */
}

//-------------------------------------------------------------------------------------------------
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
    d->end  = d->start + d->size + 1;   /* just after last element */

    /* All dbs are created empty */
    d->rptr = d->start;
    d->wptr = d->start;

    return 0;
}

//-------------------------------------------------------------------------------------------------
/* bdx_tx_db_close - Close tx db and free all memory.
 *
 * @d - tx data base
 */
static void bdx_tx_db_close(struct txdb *d)
{
    BDX_ASSERT(d == NULL);

    if (d->start)
    {
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
static struct
{
    u16 bytes;
    u16 qwords;     /* qword = 64 bit */
} txd_sizes[MAX_PBL];

//-------------------------------------------------------------------------------------------------
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
	pbl->len   	= CPU_CHIP_SWAP32(len);
	pbl->pa_lo	= CPU_CHIP_SWAP32(L32_64(dmaAddr));
	pbl->pa_hi 	= CPU_CHIP_SWAP32(H32_64(dmaAddr));
	dbg_printPBL(pbl);

} // bdx_setPbl()

//-------------------------------------------------------------------------------------------------

static inline void bdx_setTxdb(struct txdb *db, dma_addr_t dmaAddr, int len)
{
	db->wptr->len      	= len;
	db->wptr->addr.dma 	= dmaAddr;

} // bdx_setTxdb()

//-------------------------------------------------------------------------------------------------

static inline int bdx_tx_map_skb(struct bdx_priv *priv, struct sk_buff *skb, struct txd_desc *txdd, int* nr_frags, unsigned int *pkt_len)
{
	struct skb_frag_struct *frag;
	dma_addr_t	dmaAddr;
    int 		i, len;
    struct txdb *db 		= &priv->txdb;
    struct pbl 	*pbl 		= &txdd->pbl[0];
    int 		nrFrags 	= skb_shinfo(skb)->nr_frags;
    int			copyFrags	= 0;
    int			copyBytes   = 0;

    do
    {
    	DBG("TX skb %p skbLen %d dataLen %d frags %d\n", skb, skb->len, skb->data_len, nrFrags);
    	if (nrFrags > MAX_PBL -1)
    	{
    		ERR("MAX PBL exceeded %d !!!\n", nrFrags);
    		copyBytes = -1;
    		break;
    	}
    	*nr_frags = nrFrags;
    	// initial skb
    	len = skb->len - skb->data_len;
		dmaAddr = pci_map_single(priv->pdev, skb->data, len, PCI_DMA_TODEVICE);
    	bdx_setTxdb(db, dmaAddr, len);
		bdx_setPbl(pbl++, db->wptr->addr.dma, db->wptr->len);
		*pkt_len = db->wptr->len;
#if (defined VM_KLNX)
		// copy frags
	   	copyBytes = bdx_tx_copy_frags(priv, skb, &copyFrags, &pbl, nr_frags);
#endif
		// remaining frags
		for (i = copyFrags; i < nrFrags; i++)
		{

			frag 				= &skb_shinfo(skb)->frags[i];
	#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2,0)
			dmaAddr = pci_map_page(priv->pdev, frag->page,frag->page_offset, frag->size, PCI_DMA_TODEVICE);
	#else
			dmaAddr = skb_frag_dma_map(&priv->pdev->dev, frag, 0, frag->size, PCI_DMA_TODEVICE);
	#endif
			bdx_tx_db_inc_wptr(db);
			bdx_setTxdb(db, dmaAddr, frag->size);
			bdx_setPbl(pbl++, db->wptr->addr.dma, db->wptr->len);
			*pkt_len += db->wptr->len;
		}
		if(skb->len<60)
		{
			++nrFrags;	// SHORT_PKT_FIX
		}
		/* Add skb clean up info. */
		bdx_tx_db_inc_wptr(db);
	    db->wptr->len      = -txd_sizes[nrFrags].bytes;
	    db->wptr->addr.skb = skb;
		bdx_tx_db_inc_wptr(db);

    } while(0);

    return copyBytes;

	//if (copyBytes) return -1; else return 0;

} // bdx_tx_map_skb()

//-------------------------------------------------------------------------------------------------

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
    for (i = 0; i < MAX_PBL; i++)
    {
        lwords = 7 + (i * 3);
        if (lwords & 1)
            lwords++;   /* pad it with 1 lword */
        txd_sizes[i].qwords = lwords >> 1;
        txd_sizes[i].bytes  = lwords << 2;
        DBG("%2d. %d\n", i, txd_sizes[i].bytes);
    }
}

//-------------------------------------------------------------------------------------------------
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

    // SHORT_PKT_FIX
    priv->b0_len=64;
    priv->b0_va = pci_alloc_consistent(priv->pdev,priv->b0_len, &priv->b0_dma);
    memset(priv->b0_va,0,priv->b0_len);
    // SHORT_PKT_FIX end

    priv->tx_level = BDX_MAX_TX_LEVEL;
    priv->tx_update_mark = priv->tx_level - 1024;

    return 0;

err_mem:
    ERR("%s Tx init failed\n", priv->ndev->name);
    return -ENOMEM;
}

//-------------------------------------------------------------------------------------------------
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
    fsize     = f->m.rptr - f->m.wptr;
    if (fsize <= 0)
        fsize = f->m.memsz + fsize;
    return (fsize);
}

//-------------------------------------------------------------------------------------------------

void bdx_tx_timeout (struct net_device *ndev)
{
	DBG("%s: %s: TX timeout\n", BDX_DRV_NAME, ndev->name);
}

//-------------------------------------------------------------------------------------------------
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
    struct bdx_priv 	*priv 			= netdev_priv(ndev);
    struct txd_fifo 	*f    			= &priv->txd_fifo0;
    int 				txd_checksum	= 7;   /* full checksum */
    int 				txd_lgsnd    	= 0;
    int 				txd_vlan_id  	= 0;
    int 				txd_vtag     	= 0;
    int 				txd_mss      	= 0;
    int					rVal			= NETDEV_TX_OK;
    unsigned int 		pkt_len;
    struct txd_desc 	*txdd;
    int 				nr_frags, len, copyBytes;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 7,0)
    unsigned long 		flags;
    int					spinLocked;
#endif
DBG_OFF;
    ENTER;
    if (!(priv->state & BDX_STATE_STARTED))
    {
      return -1;
    }
    do
    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 7,0)
		local_irq_save(flags);
		if (! (spinLocked = spin_trylock(&priv->tx_lock)))
		{
			local_irq_restore(flags);
			DBG("%s: %s TX locked, returning NETDEV_TX_LOCKED\n", BDX_DRV_NAME, ndev->name);
			rVal = NETDEV_TX_LOCKED;
			break;
		}
#endif
		/* Build tx descriptor */
		BDX_ASSERT(f->m.wptr >= f->m.memsz);    /* started with valid wptr */
		txdd = (struct txd_desc *)(f->m.va + f->m.wptr);
		copyBytes = bdx_tx_map_skb(priv, skb, txdd, &nr_frags, &pkt_len);
		if (copyBytes < 0)
		{
			dev_kfree_skb_any(skb);
			break;
		}
		if (unlikely(skb->ip_summed != CHECKSUM_PARTIAL))
		{
			txd_checksum = 0;
		}
		if (skb_shinfo(skb)->gso_size)
		{
			txd_mss = skb_shinfo(skb)->gso_size;
			txd_lgsnd = 1;
			DBG("skb %p pkt len %d gso size = %d\n", skb, pkt_len, txd_mss);
		}
		if (vlan_tx_tag_present(skb))
		{
			/* Don't cut VLAN ID to 12 bits */
			txd_vlan_id = vlan_tx_tag_get(skb);
			txd_vtag = 1;
		}
		txdd->va_hi    = copyBytes;
		txdd->va_lo    = (u32)((u64)skb);
		txdd->length   = CPU_CHIP_SWAP16(pkt_len);
		txdd->mss      = CPU_CHIP_SWAP16(txd_mss);
		txdd->txd_val1 = CPU_CHIP_SWAP32(TXD_W1_VAL(txd_sizes[nr_frags].qwords,
										 txd_checksum, txd_vtag,
										 txd_lgsnd, txd_vlan_id));
		DBG("=== w1 qwords[%d] %d =====\n" , nr_frags, txd_sizes[nr_frags].qwords);
		DBG("=== TxD desc =====================\n");
		DBG("=== w1: 0x%x ================\n", txdd->txd_val1);
		DBG("=== w2: mss 0x%x len 0x%x\n", txdd->mss, txdd->length);
		 // SHORT_PKT_FIX
		if(pkt_len<60)
		{
			struct pbl *pbl = &txdd->pbl[++nr_frags];
			txdd->length    = CPU_CHIP_SWAP16(60);
			txdd->txd_val1  = CPU_CHIP_SWAP32(TXD_W1_VAL(txd_sizes[nr_frags].qwords, txd_checksum, txd_vtag,txd_lgsnd, txd_vlan_id));
			pbl->len        = CPU_CHIP_SWAP32(60-pkt_len);
			pbl->pa_lo 		= CPU_CHIP_SWAP32(L32_64(priv->b0_dma));
			pbl->pa_hi 		= CPU_CHIP_SWAP32(H32_64(priv->b0_dma));
			DBG("=== SHORT_PKT_FIX   ================\n");
			DBG("=== nr_frags : %d   ================\n", nr_frags);
			dbg_printPBL(pbl);
		}
		// SHORT_PKT_FIX end
		/*
		 * Increment TXD write pointer. In case of fifo wrapping copy reminder of
		 *  the descriptor to the beginning
		*/
		f->m.wptr += txd_sizes[nr_frags].bytes;
		len        = f->m.wptr - f->m.memsz;
		if (unlikely(len >= 0))
		{
			f->m.wptr = len;
			if (len > 0)
			{
				BDX_ASSERT(len > f->m.memsz);
				memcpy(f->m.va, f->m.va + f->m.memsz, len);
			}
		}
		BDX_ASSERT(f->m.wptr >= f->m.memsz);    /* finished with valid wptr */
		priv->tx_level -= txd_sizes[nr_frags].bytes;
		BDX_ASSERT(priv->tx_level <= 0 || priv->tx_level > BDX_MAX_TX_LEVEL);
#if (defined(TN40_PTP) && defined(ETHTOOL_GET_TS_INFO))
		skb_tx_timestamp(skb);
#endif
		if (priv->tx_level > priv->tx_update_mark)
		{
			/*
					 * Force memory writes to complete before letting the HW know
					 * there are new descriptors to fetch (might be needed on
					 * platforms like IA64).
			 *  wmb();
					 */
			WRITE_REG(priv, f->m.reg_WPTR, f->m.wptr & TXF_WPTR_WR_PTR);
		}
		else
		{
			if (priv->tx_noupd++ > BDX_NO_UPD_PACKETS)
			{
				priv->tx_noupd = 0;
				WRITE_REG(priv, f->m.reg_WPTR, f->m.wptr & TXF_WPTR_WR_PTR);
			}
		}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0) || (defined(RHEL_RELEASE_CODE) && RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7,4)))
		netif_trans_update(ndev);
#else
		ndev->trans_start = jiffies;
#endif
		priv->net_stats.tx_packets++;
		priv->net_stats.tx_bytes += pkt_len;
		if (priv->tx_level < BDX_MIN_TX_LEVEL)
		{
			DBG("%s: %s: TX Q STOP level %d\n", BDX_DRV_NAME, ndev->name, priv->tx_level);
			netif_stop_queue(ndev);
		}

    } while (0);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 7,0)
    if (spinLocked)
    {
    	spin_unlock_irqrestore(&priv->tx_lock, flags);
    }
#endif
    RET(rVal);

} // bdx_tx_transmit()

//-------------------------------------------------------------------------------------------------

/* bdx_tx_cleanup - Clean the TXF fifo, run in the context of IRQ.
 *
 * @priv - bdx adapter
 *
 * This function scans the TXF fifo for descriptors, frees DMA mappings and
 * reports to the OS that those packets were sent.
 */
static void bdx_tx_cleanup(struct bdx_priv *priv)
{
    struct txf_fifo *f 		 = &priv->txf_fifo0;
    struct txdb 	*db 	 = &priv->txdb;
    int 			tx_level = 0;

    ENTER;
    f->m.wptr = READ_REG(priv, f->m.reg_WPTR) & TXF_WPTR_MASK;
    BDX_ASSERT(f->m.rptr >= f->m.memsz);    /* Started with valid rptr */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 7,0)
    spin_lock(&priv->tx_lock);
#else
    netif_tx_lock(priv->ndev);
#endif
    while (f->m.wptr != f->m.rptr)
    {
#if (defined VM_KLNX)
    	struct txf_desc *txfd  = (struct txf_desc *)(f->m.va + f->m.rptr);
        if (txfd->va_hi != 0)
        {
        	DBG("bdx_freeCmem() skb 0x%x len %d\n", txfd->va_lo, txfd->va_hi);
        	bdx_freeCmem(priv, txfd->va_hi);
        }
#endif
        f->m.rptr += BDX_TXF_DESC_SZ;
        f->m.rptr &= f->m.size_mask;
        /* Unmap all fragments */
        /* First has to come tx_maps containing DMA */
        BDX_ASSERT(db->rptr->len == 0);
        do
        {
            BDX_ASSERT(db->rptr->addr.dma == 0);
            DBG("pci_unmap_page 0x%llx len %d\n", db->rptr->addr.dma, db->rptr->len);
            pci_unmap_page(priv->pdev, db->rptr->addr.dma, db->rptr->len, PCI_DMA_TODEVICE);
            bdx_tx_db_inc_rptr(db);
        } while (db->rptr->len > 0);
        tx_level -= db->rptr->len; /* '-' Because the len is negative */

        /* Now should come skb pointer - free it */
        dev_kfree_skb_any(db->rptr->addr.skb);
        DBG("dev_kfree_skb_any %p %d\n",db->rptr->addr.skb, -db->rptr->len);
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
    if (priv->tx_noupd)
    {
        priv->tx_noupd = 0;
        WRITE_REG(priv, priv->txd_fifo0.m.reg_WPTR,
                  priv->txd_fifo0.m.wptr & TXF_WPTR_WR_PTR);
    }
    if (unlikely(netif_queue_stopped(priv->ndev) &&
                 netif_carrier_ok(priv->ndev) &&
                 (priv->tx_level >= BDX_MAX_TX_LEVEL/2)))
    {
        DBG("%s: %s: TX Q WAKE level %d\n", BDX_DRV_NAME, priv->ndev->name, priv->tx_level);
        netif_wake_queue(priv->ndev);
    }
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 7,0)
    spin_unlock(&priv->tx_lock);
#else
    netif_tx_unlock(priv->ndev);
#endif
    EXIT;

} // bdx_tx_cleanup()

//-------------------------------------------------------------------------------------------------

/* bdx_tx_free_skbs - Free all skbs from TXD fifo.
 *
 * This function is called when the OS shuts down this device, e.g. upon
 * "ifconfig down" or rmmod.
 */
static void bdx_tx_free_skbs(struct bdx_priv *priv)
{
    struct txdb *db = &priv->txdb;

    ENTER;
    while (db->rptr != db->wptr)
    {
        if (likely(db->rptr->len))
            pci_unmap_page(priv->pdev, db->rptr->addr.dma,
                           db->rptr->len, PCI_DMA_TODEVICE);
        else
            dev_kfree_skb(db->rptr->addr.skb);
        bdx_tx_db_inc_rptr(db);
    }
    RET();
}

//-------------------------------------------------------------------------------------------------
/* bdx_tx_free - Free all Tx resources */

static void bdx_tx_free(struct bdx_priv *priv)
{
    ENTER;
    bdx_tx_free_skbs(priv);
    bdx_fifo_free(priv, &priv->txd_fifo0.m);
    bdx_fifo_free(priv, &priv->txf_fifo0.m);
    bdx_tx_db_close(&priv->txdb);
    // SHORT_PKT_FIX
    if(priv->b0_len)
    {
        pci_free_consistent(priv->pdev, priv->b0_len, priv->b0_va, priv->b0_dma);
        priv->b0_len=0;
    }
    // SHORT_PKT_FIX end

}

//-------------------------------------------------------------------------------------------------
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

    if (i > size)
    {
        memcpy(f->m.va + f->m.wptr, data, size);
        f->m.wptr += size;
    }
    else
    {
        memcpy(f->m.va + f->m.wptr, data, i);
        f->m.wptr = size - i;
        memcpy(f->m.va, data + i, f->m.wptr);
    }
    WRITE_REG(priv, f->m.reg_WPTR, f->m.wptr & TXF_WPTR_WR_PTR);
}

//-------------------------------------------------------------------------------------------------
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
    ENTER;

    while (size > 0)
    {
        /*
        * We subtract 8 because when the fifo is full rptr == wptr, which
        * also means that fifo is empty, we can understand the difference,
        * but could the HW do the same ??? :)
        */
        int avail = bdx_tx_space(priv) - 8;
        if (avail <= 0)
        {
            if (timer++ > 300)      /* Prevent endless loop */
            {
                DBG("timeout while writing desc to TxD fifo\n");
                break;
            }
            udelay(50); /* Give the HW a chance to clean the fifo */
            continue;
        }
        avail = MIN(avail, size);
        DBG("about to push  %d bytes starting %p size %d\n", avail,
            data, size);
        bdx_tx_push_desc(priv, data, avail);
        size -= avail;
        data += avail;
    }
    RET();
}
//-------------------------------------------------------------------------------------------------

static int bdx_ioctl_priv(struct net_device *ndev, struct ifreq *ifr, int cmd)
{
    struct bdx_priv *priv = netdev_priv(ndev);
    tn40_ioctl_t tn40_ioctl;
    int error;
    u16 dev,addr;

    ENTER;
    DBG("jiffies =%ld cmd =%d\n", jiffies, cmd);
    if (cmd != SIOCDEVPRIVATE)
    {
        error = copy_from_user(&tn40_ioctl, ifr->ifr_data, sizeof(tn40_ioctl));
        if (error)
        {
            ERR("cant copy from user\n");
            RET(error);
        }
        DBG("%d 0x%x 0x%x 0x%p\n", tn40_ioctl.data[0], tn40_ioctl.data[1], tn40_ioctl.data[2], tn40_ioctl.buf);
    }
    if (!capable(CAP_SYS_RAWIO))
        return -EPERM;

    switch (tn40_ioctl.data[0])
    {
        case OP_INFO:
            switch(tn40_ioctl.data[1])
            {
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
            error = copy_to_user(ifr->ifr_data, &tn40_ioctl, sizeof(tn40_ioctl));
            if (error)
                RET(error);
            break;

        case OP_READ_REG:
            error = bdx_range_check(priv, tn40_ioctl.data[1]);
            if (error < 0)
                return error;
            tn40_ioctl.data[2] = READ_REG(priv, tn40_ioctl.data[1]);
            DBG("read_reg(0x%x)=0x%x (dec %d)\n", tn40_ioctl.data[1], tn40_ioctl.data[2],
                tn40_ioctl.data[2]);
            error = copy_to_user(ifr->ifr_data, &tn40_ioctl, sizeof(tn40_ioctl));
            if (error)
                RET(error);
            break;

        case OP_WRITE_REG:
            error = bdx_range_check(priv, tn40_ioctl.data[1]);
            if (error < 0)
                return error;
            WRITE_REG(priv, tn40_ioctl.data[1], tn40_ioctl.data[2]);
            break;

        case OP_MDIO_READ:
            if(priv->phy_mdio_port==0xFF) return -EINVAL;
            dev=(u16)(0xFFFF&(tn40_ioctl.data[1]>>16));
            addr=(u16)(0xFFFF&tn40_ioctl.data[1]);
            tn40_ioctl.data[2] =0xFFFF&PHY_MDIO_READ(priv,dev,addr);
            error = copy_to_user(ifr->ifr_data, &tn40_ioctl, sizeof(tn40_ioctl));
            if (error)
                RET(error);
            break;

        case OP_MDIO_WRITE:
            if(priv->phy_mdio_port==0xFF) return -EINVAL;
            dev=(u16)(0xFFFF&(tn40_ioctl.data[1]>>16));
            addr=(u16)(0xFFFF&tn40_ioctl.data[1]);
            PHY_MDIO_WRITE(priv,dev,addr,(u16)(tn40_ioctl.data[2]));
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
            char 			*buf;
            uint 			buf_size;
            unsigned long 	bytes;

            error = 0;
 	 	 	buf = memLogGetLine(&buf_size);
          	if (buf != NULL)
         	{

          		tn40_ioctl.data[2] = min(tn40_ioctl.data[1], buf_size);
          		bytes = copy_to_user(ifr->ifr_data, &tn40_ioctl, sizeof(tn40_ioctl));
 				bytes = copy_to_user(tn40_ioctl.buf, buf, tn40_ioctl.data[2]);
          		DBG("copy_to_user %p %u return %lu\n", tn40_ioctl.buf, tn40_ioctl.data[2], bytes);
         	}
         	else
         	{
         		DBG("=================== EOF =================\n");
         		error = -EIO;
         	}
          	RET(error);
         	break;
        }
#endif
#ifdef TN40_DEBUG
        case OP_DBG:
        	switch (tn40_ioctl.data[1])
        	{
#ifdef _DRIVER_RESUME_DBG

        		pm_message_t pmMsg = {0};

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

#ifdef RX_REUSE_PAGES
        		case DBG_PRINT_PAGE_TABLE:
        			dbg_printRxPageTable(priv);
        			break;
#endif
        		default:
        			dbg_printIoctl();
        			break;

        	}
        	break;
#endif // TN40_DEBUG

        default:
            RET(-EOPNOTSUPP);
    }

    return 0;

} // bdx_ioctl_priv()

//-------------------------------------------------------------------------------------------------

static int bdx_ioctl(struct net_device *ndev, struct ifreq *ifr, int cmd)
{
    ENTER;
    if (cmd > SIOCDEVPRIVATE && cmd <= (SIOCDEVPRIVATE + 15))
        RET(bdx_ioctl_priv(ndev, ifr, cmd));
    else
        RET(-EOPNOTSUPP);

} // bdx_ioctl()

//-------------------------------------------------------------------------------------------------

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
static const struct net_device_ops bdx_netdev_ops =
{
    .ndo_open               = bdx_open,
    .ndo_stop               = bdx_close,
    .ndo_start_xmit         = bdx_tx_transmit,
    .ndo_validate_addr      = eth_validate_addr,
    .ndo_do_ioctl           = bdx_ioctl,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
    .ndo_set_multicast_list = bdx_setmulti,
#else
    .ndo_set_rx_mode        = bdx_setmulti,
#endif
    .ndo_get_stats          = bdx_get_stats,
#if defined(RHEL_RELEASE_CODE) && (RHEL_RELEASE_CODE>=RHEL_RELEASE_VERSION(8,0))
    .ndo_change_mtu         = bdx_change_mtu,
#elif defined(RHEL_RELEASE_CODE) && (RHEL_RELEASE_CODE>=RHEL_RELEASE_VERSION(7,5))
    .ndo_change_mtu_rh74    = bdx_change_mtu,
#else
    .ndo_change_mtu         = bdx_change_mtu,
#endif
    .ndo_set_mac_address    = bdx_set_mac,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 1,0)
    .ndo_vlan_rx_register   = bdx_vlan_rx_register,
#endif
    .ndo_vlan_rx_add_vid    = bdx_vlan_rx_add_vid,
    .ndo_vlan_rx_kill_vid   = bdx_vlan_rx_kill_vid,
};
#endif

//-------------------------------------------------------------------------------------------------

static int bdx_get_ports_by_id(int vendor,int device)
{
#if 0
    int i=0;
    for(; bdx_dev_tbl[i].vid; i++)
    {
        if((bdx_dev_tbl[i].vid==vendor) && (bdx_dev_tbl[i].pid==device))
            return  bdx_dev_tbl[i].ports;
    }
#endif
    return 1;
}

//-------------------------------------------------------------------------------------------------
#ifdef TN40_IRQ_MSI
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
static int bdx_support_msi_by_id(int vendor,int device)
{
#if 0
    int i=0;
    for(; bdx_dev_tbl[i].vid; i++)
    {
        if((bdx_dev_tbl[i].vid==vendor) && (bdx_dev_tbl[i].pid==device))
            return  bdx_dev_tbl[i].msi;
    }
    return 0;
#else
    return 1;
#endif
}
#endif
#endif // TN40_IRQ_MSI
//-------------------------------------------------------------------------------------------------

static int bdx_get_phy_by_id(int vendor,int device,int subsystem,int port)
{
    int i=0;
    for(; bdx_dev_tbl[i].vid; i++)
    {
        if((bdx_dev_tbl[i].vid==vendor)
                && (bdx_dev_tbl[i].pid==device)
                && (bdx_dev_tbl[i].subdev==subsystem)
          )
            return  (port==0)?bdx_dev_tbl[i].phya:bdx_dev_tbl[i].phyb;
    }
    return 0;

}

//-------------------------------------------------------------------------------------------------
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

static int __init bdx_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    struct net_device *ndev;
    struct bdx_priv *priv;
    int err, pci_using_dac;
    resource_size_t pciaddr;
    u32 regionSize;
    struct pci_nic *nic;
    int phy;
#ifdef TN40_IRQ_MSI
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
    unsigned int nvec = 1;
#endif
#endif
    ENTER;

    nic = vmalloc(sizeof(*nic));
    if (!nic)
    {
    	ERR("bdx_probe() no memory\n");
         RET(-ENOMEM);

    }

    /************** pci *****************/
    if ((err = pci_enable_device(pdev)))    /* It triggers interrupt, dunno
                                                   why. */
        goto err_pci;           /* it's not a problem though */

    if (!(err = pci_set_dma_mask(pdev, LUXOR__DMA_64BIT_MASK)) &&
            !(err = pci_set_consistent_dma_mask(pdev, LUXOR__DMA_64BIT_MASK)))
    {
        pci_using_dac = 1;
    }
    else
    {
        if ((err = pci_set_dma_mask(pdev, LUXOR__DMA_32BIT_MASK)) ||
                (err = pci_set_consistent_dma_mask(pdev,
                        LUXOR__DMA_32BIT_MASK)))
        {
            ERR("No usable DMA configuration - aborting\n");
            goto err_dma;
        }
        pci_using_dac = 0;
    }

    if ((err = pci_request_regions(pdev, BDX_DRV_NAME)))
        goto err_dma;

    pci_set_master(pdev);

    pciaddr = pci_resource_start(pdev, 0);
    if (!pciaddr)
    {
        err = -EIO;
        ERR("no MMIO resource\n");
        goto err_out_res;
    }
    if ((regionSize = pci_resource_len(pdev, 0)) < BDX_REGS_SIZE)
    {
        //      err = -EIO;
        ERR("MMIO resource (%x) too small\n", regionSize);
        //      goto err_out_res;
    }

    nic->regs = ioremap(pciaddr, regionSize);
    if (!nic->regs)
    {
        err = -EIO;
        ERR("ioremap failed\n");
        goto err_out_res;
    }

    if (pdev->irq < 2)
    {
        err = -EIO;
        ERR("invalid irq (%d)\n", pdev->irq);
        goto err_out_iomap;
    }
    pci_set_drvdata(pdev, nic);

    nic->port_num=bdx_get_ports_by_id(pdev->vendor,pdev->device);
    print_hw_id(pdev);
    bdx_hw_reset_direct(nic->regs);

    nic->irq_type = IRQ_INTX;
#ifdef TN40_IRQ_MSI
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
    nvec = pci_alloc_irq_vectors(pdev, 1, nvec, PCI_IRQ_MSI);
    if (nvec < 0)
    {
    	goto err_out_iomap;
    }
#else

    if (bdx_support_msi_by_id(pdev->vendor,pdev->device))
    {
        if ((err = pci_enable_msi(pdev)))
        {
            ERR("Can't enable msi. Error is %d\n", err);
        }
        else
        {
            nic->irq_type = IRQ_MSI;
        }
    }
    else
    {
        DBG("HW does not support MSI\n");
    }
#endif
#endif // TN40_IRQ_MSI
    /************** netdev **************/
	if (!(ndev = alloc_etherdev(sizeof(struct bdx_priv))))
	{
		err = -ENOMEM;
		ERR("alloc_etherdev failed\n");
		goto err_out_iomap;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
	ndev->netdev_ops     = &bdx_netdev_ops;
	ndev->tx_queue_len   = BDX_NDEV_TXQ_LEN;
#else
	ndev->open       		 = bdx_open;
	ndev->stop       		 = bdx_close;
	ndev->hard_start_xmit    = bdx_tx_transmit;
	ndev->tx_timeout		 = bdx_tx_timeout;
	ndev->do_ioctl       	 = bdx_ioctl;
	ndev->set_multicast_list = bdx_setmulti;
	ndev->get_stats      	 = bdx_get_stats;
	ndev->change_mtu     	 = bdx_change_mtu;
	ndev->set_mac_address    = bdx_set_mac;
	ndev->tx_queue_len   	 = BDX_NDEV_TXQ_LEN;
	ndev->vlan_rx_register   = bdx_vlan_rx_register;
	ndev->vlan_rx_add_vid    = bdx_vlan_rx_add_vid;
	ndev->vlan_rx_kill_vid   = bdx_vlan_rx_kill_vid;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24) && (!defined VM_KLNX)
	ndev->poll       = bdx_poll;
	ndev->weight         = 64;
#endif
#endif

//        bdx_ethtool_ops(ndev);  /* Ethtool interface */

	/*
			 * These fields are used for information purposes only,
	 * so we use the same for all the ports on the board
			 */
	ndev->if_port   = 0;
	ndev->base_addr = pciaddr;
	ndev->mem_start = pciaddr;
	ndev->mem_end = pciaddr + regionSize;
	ndev->irq = pdev->irq;
	ndev->features =  NETIF_F_IP_CSUM        		|
					  NETIF_F_SG             		|
					  NETIF_F_FRAGLIST       		|
					  NETIF_F_TSO            		|
					  NETIF_F_VLAN_TSO       		|
					  NETIF_F_VLAN_CSUM      		|
					  NETIF_F_GRO            		|
#if (!defined VM_KLNX)
					  NETIF_F_RXCSUM               	|
#endif
					  NETIF_F_RXHASH;

#ifdef NETIF_F_HW_VLAN_CTAG_RX
	ndev->features |= NETIF_F_HW_VLAN_CTAG_TX 		|
					  NETIF_F_HW_VLAN_CTAG_RX 		|
					  NETIF_F_HW_VLAN_CTAG_FILTER;
#else
	ndev->features |= NETIF_F_HW_VLAN_TX     		|
					  NETIF_F_HW_VLAN_RX	 		|
					  NETIF_F_HW_VLAN_FILTER;
#endif


	if (pci_using_dac)
		ndev->features |= NETIF_F_HIGHDMA;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 26)
	ndev->vlan_features = (NETIF_F_IP_CSUM   		|
						   NETIF_F_SG    			|
						   NETIF_F_TSO   			|
						   NETIF_F_GRO       		|
						   NETIF_F_RXHASH);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
	if (pci_using_dac)
		ndev->vlan_features |= NETIF_F_HIGHDMA;
#endif
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
	ndev->min_mtu = ETH_ZLEN;
	ndev->max_mtu = BDX_MAX_MTU;
#endif
	/************** PRIV ****************/
	priv = nic->priv = netdev_priv(ndev);

	memset(priv, 0, sizeof(struct bdx_priv));
	priv->drv_name = BDX_DRV_NAME;
	priv->pBdxRegs = nic->regs;
	priv->port = 0;
	priv->pdev = pdev;
	priv->ndev = ndev;
	priv->nic  = nic;
	priv->msg_enable = BDX_DEF_MSG_ENABLE;
	priv->deviceId = pdev->device;
	LUXOR__NAPI_ADD(ndev, &priv->napi, bdx_poll, 64);

	if ((readl(nic->regs + FPGA_VER) & 0xFFF) == 308)
	{
		DBG("HW statistics not supported\n");
		priv->stats_flag = 0;
	}
	else
	{
		priv->stats_flag = 1;
	}
	/*Init PHY*/
	priv->subsystem_vendor = priv->pdev->subsystem_vendor;
	priv->subsystem_device = priv->pdev->subsystem_device;
	phy=bdx_get_phy_by_id(pdev->vendor,pdev->device,pdev->subsystem_device,0);
	if (bdx_mdio_reset(priv, 0, phy) == -1)
	{
		err = -ENODEV;
		goto err_out_iomap;
	}

	bdx_ethtool_ops(ndev);  /* Ethtool interface */

	/* Initialize fifo sizes. */
	priv->txd_size = 3;
	priv->txf_size = 3;
	// priv->rxd_size = 2;
	priv->rxd_size = 3;
	priv->rxf_size = 3;

	/* Initialize the initial coalescing registers. */
	priv->rdintcm = INT_REG_VAL(0x20, 1, 4, 12);
	priv->tdintcm = INT_REG_VAL(0x20, 1, 0, 12);
	priv->state   = BDX_STATE_HW_STOPPED;
	/*
	 * ndev->xmit_lock spinlock is not used.
	 * Private priv->tx_lock is used for synchronization
	 * between transmit and TX irq cleanup.  In addition
	 * set multicast list callback has to use priv->tx_lock.
	 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 7,0)
	spin_lock_init(&priv->tx_lock);
	ndev->features |= NETIF_F_LLTX;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	ndev->hw_features |= ndev->features;
#endif
	/*bdx_hw_reset(priv); */
	if (bdx_read_mac(priv))
	{
		ERR("load MAC address failed\n");
		goto err_out_iomap;
	}
	SET_NETDEV_DEV(ndev, &pdev->dev);
	if ((err = register_netdev(ndev)))
	{
		ERR("register_netdev failed\n");
		goto err_out_free;
	}
	bdx_reset(priv);

	//Set GPIO[9:0] to output 0
		//WRITE_REG(priv, 0x51E0,0x30010004); // GPIO_O WR CMD
		//WRITE_REG(priv, 0x51F0,0x0);
		WRITE_REG(priv, 0x51E0,0x30010006); // GPIO_OE_ WR CMD
		WRITE_REG(priv, 0x51F0,0x0); // GPIO_OE_ DATA

	WRITE_REG(priv, regMDIO_CMD_STAT, 0x3ec8);
	if (netif_running(ndev))
		netif_stop_queue(priv->ndev);

	if ((err = bdx_tx_init(priv)))
		goto err_out_free;

	if ((err = bdx_fw_load(priv)))
		goto err_out_free;

	bdx_tx_free(priv);
#if (defined VM_KLNX)
	if ((err = bdx_initCmem(priv, CMEM_SIZE)))
		goto err_out_free;
#endif

	netif_carrier_off(ndev);
	netif_stop_queue(ndev);

	print_eth_id(ndev);

    bdx_scan_pci();
#ifdef TN40_MEMLOG
    memLogInit();
#endif

    RET(0);
    ERR("bdx_probe() %d\n", err);
err_out_free:
    unregister_netdev(ndev);
    free_netdev(ndev);
err_out_iomap:
    iounmap(nic->regs);
err_out_res:
    pci_release_regions(pdev);
err_dma:
#if  (!defined VM_KLNX)
	if (pci_dev_msi_enabled(pdev))
	{
		pci_disable_msi(pdev);
	}
#else
	pci_disable_msi(pdev);
#endif
    pci_disable_device(pdev);
    pci_set_drvdata(pdev, NULL);
err_pci:
    vfree(nic);

    RET(err);

} // bdx_probe()

/****************** Ethtool interface *********************/
/* Get strings for tests */
static const char
bdx_test_names[][ETH_GSTRING_LEN] =
{
    "No tests defined"
};

//-------------------------------------------------------------------------------------------------
/* Get strings for statistics counters */
static const char
bdx_stat_names[][ETH_GSTRING_LEN] =
{
    "InUCast",      /* 0x7200 */
    "InMCast",      /* 0x7210 */
    "InBCast",      /* 0x7220 */
    "InPkts",       /* 0x7230 */
    "InErrors",     /* 0x7240 */
    "InDropped",        /* 0x7250 */
    "FrameTooLong",     /* 0x7260 */
    "FrameSequenceErrors",  /* 0x7270 */
    "InVLAN",       /* 0x7280 */
    "InDroppedDFE",     /* 0x7290 */
    "InDroppedIntFull", /* 0x72A0 */
    "InFrameAlignErrors",   /* 0x72B0 */

    /* 0x72C0-0x72E0 RSRV */

    "OutUCast",     /* 0x72F0 */
    "OutMCast",     /* 0x7300 */
    "OutBCast",     /* 0x7310 */
    "OutPkts",      /* 0x7320 */

    /* 0x7330-0x7360 RSRV */

    "OutVLAN",      /* 0x7370 */
    "InUCastOctects",   /* 0x7380 */
    "OutUCastOctects",  /* 0x7390 */

    /* 0x73A0-0x73B0 RSRV */

    "InBCastOctects",   /* 0x73C0 */
    "OutBCastOctects",  /* 0x73D0 */
    "InOctects",        /* 0x73E0 */
    "OutOctects",       /* 0x73F0 */
};

//-------------------------------------------------------------------------------------------------
#ifdef ETHTOOL_GLINKSETTINGS

int bdx_get_link_ksettings(struct net_device *netdev, struct ethtool_link_ksettings *cmd)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	bitmap_zero(cmd->link_modes.supported, 		__ETHTOOL_LINK_MODE_MASK_NBITS);
	bitmap_zero(cmd->link_modes.advertising, 	__ETHTOOL_LINK_MODE_MASK_NBITS);
	bitmap_zero(cmd->link_modes.lp_advertising, __ETHTOOL_LINK_MODE_MASK_NBITS);
	cmd->base.speed  	   = priv->link_speed;
	cmd->base.duplex 	   = DUPLEX_FULL;
	cmd->base.phy_address  = priv->phy_mdio_port;
	cmd->base.mdio_support = ETH_MDIO_SUPPORTS_C22 | ETH_MDIO_SUPPORTS_C45;

	return priv->phy_ops.get_link_ksettings(netdev, cmd);

} // bdx_get_link_ksettings()

#else
//-------------------------------------------------------------------------------------------------
/*
 * bdx_get_settings - Get device-specific settings.
 *
 * @netdev
 * @ecmd
 */
static int bdx_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    u32 rdintcm;
    u32 tdintcm;
    struct bdx_priv *priv = netdev_priv(netdev);

	ENTER;

    rdintcm = priv->rdintcm;
    tdintcm = priv->tdintcm;

    priv->phy_ops.get_settings(netdev, ecmd);
    ecmd->phy_address = priv->port;

    /* PCK_TH measures in multiples of FIFO bytes
       We translate to packets */
    ecmd->maxtxpkt =
        ((GET_PCK_TH(tdintcm) * PCK_TH_MULT) / BDX_TXF_DESC_SZ);
    ecmd->maxrxpkt =
        ((GET_PCK_TH(rdintcm) * PCK_TH_MULT) / sizeof(struct rxf_desc));

    return 0;
}

#endif

//-------------------------------------------------------------------------------------------------

#ifdef ETHTOOL_SLINKSETTINGS

int bdx_set_link_ksettings(struct net_device *netdev, const struct ethtool_link_ksettings *cmd)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	return priv->phy_ops.set_link_ksettings(netdev, cmd);

} // bdx_set_link_ksettings()

#else
//-------------------------------------------------------------------------------------------------
/*
 * bdx_set_settings - set device-specific settings.
 *
 * @netdev
 * @ecmd
 */
static int bdx_set_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	ENTER;

	DBG("ecmd->cmd=%x\n", ecmd->cmd);

    return priv->phy_ops.set_settings(netdev, ecmd);

} // bdx_set_settings()

#endif

//-------------------------------------------------------------------------------------------------

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
    strlcpy(drvinfo->bus_info, pci_name(priv->pdev), sizeof(drvinfo->bus_info));

    drvinfo->n_stats = ((priv->stats_flag) ? ARRAY_SIZE(bdx_stat_names) : 0);
    drvinfo->testinfo_len = 0;
    drvinfo->regdump_len = 0;
    drvinfo->eedump_len = 0;
}

//-------------------------------------------------------------------------------------------------

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3,0)
/*
 * bdx_get_rx_csum - Report whether receive checksums are turned on or off.
 *
 * @netdev
 */
static u32 bdx_get_rx_csum(struct net_device *netdev)
{
    return 1;       /* Always on */
}

//-------------------------------------------------------------------------------------------------

/*
 * bdx_get_tx_csum - Report whether transmit checksums are turned on or off.
 *
 * @netdev
 */
static u32 bdx_get_tx_csum(struct net_device *netdev)
{
    return (netdev->features & NETIF_F_IP_CSUM) != 0;
}
#endif

//-------------------------------------------------------------------------------------------------
/*
 * bdx_get_coalesce - Get interrupt coalescing parameters.
 *
 * @netdev
 * @ecoal
 */
static int
bdx_get_coalesce(struct net_device *netdev, struct ethtool_coalesce *ecoal)
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
    ecoal->rx_coalesce_usecs       = GET_INT_COAL(rdintcm) * INT_COAL_MULT;
    ecoal->rx_max_coalesced_frames = ((GET_PCK_TH(rdintcm) * PCK_TH_MULT) /
                                      sizeof(struct rxf_desc));

    ecoal->tx_coalesce_usecs       = GET_INT_COAL(tdintcm) * INT_COAL_MULT;
    ecoal->tx_max_coalesced_frames = ((GET_PCK_TH(tdintcm) * PCK_TH_MULT) /
                                      BDX_TXF_DESC_SZ);

    /* Adaptive parameters are ignored */
    return 0;
}

//-------------------------------------------------------------------------------------------------
/*
 * bdx_set_coalesce - Set interrupt coalescing parameters.
 *
 * @netdev
 * @ecoal
 */
static int
bdx_set_coalesce(struct net_device *netdev, struct ethtool_coalesce *ecoal)
{
    u32 rdintcm;
    u32 tdintcm;
    struct bdx_priv *priv = netdev_priv(netdev);
    int rx_coal;
    int tx_coal;
    int rx_max_coal;
    int tx_max_coal;

    /* Check for valid input */
    rx_coal     = ecoal->rx_coalesce_usecs / INT_COAL_MULT;
    tx_coal     = ecoal->tx_coalesce_usecs / INT_COAL_MULT;
    rx_max_coal = ecoal->rx_max_coalesced_frames;
    tx_max_coal = ecoal->tx_max_coalesced_frames;

    /* Translate from packets to multiples of FIFO bytes */
    rx_max_coal = (((rx_max_coal * sizeof(struct rxf_desc)) +
                    PCK_TH_MULT - 1) / PCK_TH_MULT);
    tx_max_coal = (((tx_max_coal * BDX_TXF_DESC_SZ) + PCK_TH_MULT - 1) /
                   PCK_TH_MULT);

    if ((rx_coal > 0x7FFF)  ||
            (tx_coal > 0x7FFF)  ||
            (rx_max_coal > 0xF) ||
            (tx_max_coal > 0xF))
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

//-------------------------------------------------------------------------------------------------
/* Convert RX fifo size to number of pending packets */

static inline int bdx_rx_fifo_size_to_packets(int rx_size)
{
    return ((FIFO_SIZE * (1 << rx_size)) / sizeof(struct rxf_desc));
}

//-------------------------------------------------------------------------------------------------
/* Convert TX fifo size to number of pending packets */

static inline int bdx_tx_fifo_size_to_packets(int tx_size)
{
    return ((FIFO_SIZE * (1 << tx_size)) / BDX_TXF_DESC_SZ);
}

//-------------------------------------------------------------------------------------------------
/*
 * bdx_get_ringparam - Report ring sizes.
 *
 * @netdev
 * @ring
 */
static void
bdx_get_ringparam(struct net_device *netdev, struct ethtool_ringparam *ring)
{
    struct bdx_priv *priv = netdev_priv(netdev);

    /*max_pending - The maximum-sized FIFO we allow */
    ring->rx_max_pending = bdx_rx_fifo_size_to_packets(3);
    ring->tx_max_pending = bdx_tx_fifo_size_to_packets(3);
    ring->rx_pending     = bdx_rx_fifo_size_to_packets(priv->rxf_size);
    ring->tx_pending     = bdx_tx_fifo_size_to_packets(priv->txd_size);
}

//-------------------------------------------------------------------------------------------------
/*
 * bdx_set_ringparam - Set ring sizes.
 *
 * @netdev
 * @ring
 */
static int
bdx_set_ringparam(struct net_device *netdev, struct ethtool_ringparam *ring)
{
    struct bdx_priv *priv = netdev_priv(netdev);
    int rx_size = 0;
    int tx_size = 0;

    for (; rx_size < 4; rx_size++)
    {
        if (bdx_rx_fifo_size_to_packets(rx_size) >= ring->rx_pending)
            break;
    }
    if (rx_size == 4)
        rx_size = 3;

    for (; tx_size < 4; tx_size++)
    {
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

    if (netif_running(netdev))
    {
        bdx_close(netdev);
        bdx_open(netdev);
    }
    return 0;
}

//-------------------------------------------------------------------------------------------------
/*
 * bdx_get_strings - Return a set of strings that describe the requested
 *           objects.
 * @netdev
 * @data
 */
static void bdx_get_strings(struct net_device *netdev, u32 stringset, u8 *data)
{
    switch (stringset)
    {
        case ETH_SS_TEST:
            memcpy(data, *bdx_test_names, sizeof(bdx_test_names));
            break;

        case ETH_SS_STATS:
            memcpy(data, *bdx_stat_names, sizeof(bdx_stat_names));
            break;
    }
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
//-------------------------------------------------------------------------------------------------
/*
 * bdx_get_sset_count - Return the number of statistics or tests.
 *
 * @netdev
 */
static int bdx_get_sset_count(struct net_device *netdev, int stringset)
{
    struct bdx_priv *priv = netdev_priv(netdev);

    switch (stringset)
    {
        case ETH_SS_STATS:
            BDX_ASSERT(ARRAY_SIZE(bdx_stat_names) !=
                       sizeof(struct bdx_stats) / sizeof(u64));
            return ((priv->stats_flag) ? ARRAY_SIZE(bdx_stat_names) : 0);
        default:
            return -EINVAL;
    }
}

//-------------------------------------------------------------------------------------------------

#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
/*
 * bdx_get_stats_count - Return the number of 64bit statistics counters.
 *
 * @netdev
 */
static int bdx_get_stats_count(struct net_device *netdev)
{
    struct bdx_priv *priv = netdev_priv(netdev);
    BDX_ASSERT(ARRAY_SIZE(bdx_stat_names) !=
               sizeof(struct bdx_stats) / sizeof(u64));
    return ((priv->stats_flag) ? ARRAY_SIZE(bdx_stat_names) : 0);
}
#endif

//-------------------------------------------------------------------------------------------------
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

    if (priv->stats_flag)
    {

        /* Update stats from HW */
        bdx_update_stats(priv);

        /* Copy data to user buffer */
        memcpy(data, &priv->hw_stats, sizeof(priv->hw_stats));
    }
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)

//-------------------------------------------------------------------------------------------------
/* Blink LED's for finding board */

static int bdx_set_phys_id(struct net_device *netdev,
                           enum ethtool_phys_id_state state)
{
    struct bdx_priv *priv = netdev_priv(netdev);
    int rval = 0;

    switch (state)
    {
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

//-------------------------------------------------------------------------------------------------

#else

static void bdx_blink_callback(unsigned long data)
{
    struct bdx_priv *priv = (struct bdx_priv *) data;

    priv->phy_ops.leds[0] = !priv->phy_ops.leds[0];
    if (priv->phy_ops.leds[0])
    {
    	priv->phy_ops.ledset(priv, PHY_LEDS_ON);
    }
    else
    {
    	priv->phy_ops.ledset(priv, PHY_LEDS_OFF);
    }

    mod_timer(&priv->blink_timer, jiffies + HZ);
}

//-------------------------------------------------------------------------------------------------

/*
 * bdx_phys_id - Blink the device led.
 * @netdev
 * @data       - Number of seconds to blink (0 - forever)
 */
static int bdx_phys_id(struct net_device *netdev, u32 data)
{
    struct bdx_priv *priv = netdev_priv(netdev);

    if (data == 0)
        data = INT_MAX;

    if (!priv->blink_timer.function)
    {
        init_timer(&priv->blink_timer);
        priv->blink_timer.function = bdx_blink_callback;
        priv->blink_timer.data     = (unsigned long)priv;
    }
    priv->phy_ops.ledset(priv, PHY_LEDS_SAVE);
    mod_timer(&priv->blink_timer, jiffies);
    msleep_interruptible(data * 1000);
    del_timer_sync(&priv->blink_timer);
    priv->phy_ops.ledset(priv, PHY_LEDS_RESTORE);
    return 0;
}

#endif
#ifdef _EEE_
#ifdef ETHTOOL_GEEE
//-------------------------------------------------------------------------------------------------
/*
 * bdx_get_eee - Get device-specific EEE settings
 *
 * @netdev
 * @ecmd
 */
static int bdx_get_eee(struct net_device *netdev, struct ethtool_eee *edata)
{
	int				err;
    struct bdx_priv *priv = netdev_priv(netdev);

    if(priv->phy_ops.get_eee == NULL)
    {
		ERR("EEE Functionality is not supported\n");
		err = -EOPNOTSUPP;
	}
    else
    {
    	err = priv->phy_ops.get_eee(netdev, edata);
    }

//  ecmd->phy_address = priv->port;

    return err;
    
} // bdx_get_eee()
#endif
#ifdef ETHTOOL_SEEE
//-------------------------------------------------------------------------------------------------
/*
 * bdx_set_eee - set device-specific EEE settings
 *
 * @netdev
 * @ecmd
 */
static int bdx_set_eee(struct net_device *netdev, struct ethtool_eee *edata)
{
	int 			err;
    struct bdx_priv *priv = netdev_priv(netdev);

    if(priv->phy_ops.set_eee == NULL)
    {
		ERR("EEE Functionality is not supported\n");
		err = -EOPNOTSUPP;
	}
    else
    {
    	MSG("Setting EEE\n");
    	err = priv->phy_ops.set_eee(priv);
    }
    return err;

} // bdx_set_eee()
#endif
#endif

//-------------------------------------------------------------------------------------------------
#if (defined(TN40_PTP) && defined(ETHTOOL_GET_TS_INFO))

static int bdx_get_ts_info(struct net_device *netdev, struct ethtool_ts_info *info)
{
	info->so_timestamping |= SOF_TIMESTAMPING_SOFTWARE 		|
							 SOF_TIMESTAMPING_TX_SOFTWARE	|
							 SOF_TIMESTAMPING_RX_SOFTWARE;

	return 0;

} // bdx_get_ts_info()

#endif
//-------------------------------------------------------------------------------------------------
/*
 * bdx_ethtool_ops - Ethtool interface implementation.
 *
 * @netdev
 */
static void bdx_ethtool_ops(struct net_device *netdev)
{

    static struct ethtool_ops bdx_ethtool_ops =
    {
#ifdef ETHTOOL_GLINKSETTINGS
    	.get_link_ksettings= bdx_get_link_ksettings,
#else
        .get_settings      = bdx_get_settings,
#endif
#ifdef ETHTOOL_SLINKSETTINGS
	.set_link_ksettings= bdx_set_link_ksettings,
#else
        .set_settings      = bdx_set_settings,
#endif
        .get_drvinfo       = bdx_get_drvinfo,
        .get_link      	   = ethtool_op_get_link,
        .get_coalesce      = bdx_get_coalesce,
        .set_coalesce      = bdx_set_coalesce,
        .get_ringparam     = bdx_get_ringparam,
        .set_ringparam     = bdx_set_ringparam,
#if defined(_EEE_) && (!defined(RHEL6_ETHTOOL_OPS_EXT_STRUCT))
#ifdef ETHTOOL_GEEE
		.get_eee		   = bdx_get_eee,
#endif
#ifdef ETHTOOL_SEEE
		.set_eee		   = bdx_set_eee,
#endif
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3,0)
        .get_rx_csum       = bdx_get_rx_csum,
        .get_tx_csum       = bdx_get_tx_csum,
        .get_sg            = ethtool_op_get_sg,
        .get_tso           = ethtool_op_get_tso,
#endif
        .get_strings       = bdx_get_strings,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
        .get_sset_count    = bdx_get_sset_count,
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
        .get_stats_count   = bdx_get_stats_count,
#endif
        .get_ethtool_stats = bdx_get_ethtool_stats,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
        .set_phys_id       = bdx_set_phys_id,
#else
        .phys_id       = bdx_phys_id,
#endif
#if (defined(TN40_PTP) && defined(ETHTOOL_GET_TS_INFO))
	.get_ts_info			= bdx_get_ts_info,
#endif
    };
#if defined(_EEE_) && (defined(RHEL6_ETHTOOL_OPS_EXT_STRUCT))
    static struct  ethtool_ops_ext bdx_ethtool_ops_ext =
    {
#ifdef ETHTOOL_GEEE
		.get_eee		   = bdx_get_eee,
#endif
#ifdef ETHTOOL_SEEE
		.set_eee		   = bdx_set_eee,
#endif
    };
    set_ethtool_ops_ext(netdev, &bdx_ethtool_ops_ext);
#endif

    //struct bdx_priv *priv = netdev_priv(netdev);

      //  bdx_ethtool_ops.set_settings      =priv->phy_ops.set_settings;
//ERR("bdx_ethtool_ops.set_settings = %s",bdx_ethtool_ops.set_settings?"defined":" is 0");

#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(3,16,0) )
#define SET_ETHTOOL_OPS(netdev, ops) ((netdev)->ethtool_ops = (ops))
#endif /* >= 3.16.0 */

    SET_ETHTOOL_OPS(netdev, &bdx_ethtool_ops);


} // bdx_ethtool_ops()

//-------------------------------------------------------------------------------------------------

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
    struct pci_nic 		*nic  = pci_get_drvdata(pdev);
    struct net_device 	*ndev = nic->priv->ndev;
    struct bdx_priv 	*priv = nic->priv;

	bdx_hw_stop(priv);
	bdx_sw_reset(priv);
	bdx_rx_free(priv);
	bdx_tx_free(priv);
#if  (defined VM_KLNX)
	bdx_freeAllCmem(priv);
#endif
	unregister_netdev(ndev);
	free_netdev(ndev);
    // bdx_hw_reset_direct(nic->regs);
    if (nic->irq_type == IRQ_MSI)
    {
        pci_disable_msi(pdev);
    }
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
    MSG("Device removed\n");

    RET();

} // bdx_remove()

//-------------------------------------------------------------------------------------------------
#ifdef _DRIVER_RESUME_

#define PCI_PMCR 0x7C

static int bdx_suspend(struct device *dev)
{
	struct pci_dev 		*pdev = to_pci_dev(dev);
    struct pci_nic    	*nic  = pci_get_drvdata(pdev);
    struct bdx_priv     *priv = nic->priv;
    int 				err;

    MSG("System suspend\n");
    bdx_stop(priv);
    err = pci_save_state(pdev);
    DBG("pci_save_state = %d\n", err);
    err = pci_prepare_to_sleep(pdev);
    DBG("pci_prepare_to_sleep = %d\n", err);
    return 0;

 } // bdx_suspend()

//-------------------------------------------------------------------------------------------------

static int bdx_resume(struct device *dev)
{
	struct pci_dev 		*pdev	= to_pci_dev(dev);
    struct pci_nic    	*nic  	= pci_get_drvdata(pdev);
    struct bdx_priv     *priv	= nic->priv;
    int 				rc		= -1;

    MSG("System resume\n");
    do
    {
    	pci_restore_state(pdev);
    	rc = pci_save_state(pdev);
    	DBG("pci_save_state = %d\n", rc);
    	setMDIOSpeed(priv, priv->phy_ops.mdio_speed);
        if (priv->phy_ops.mdio_reset(priv, priv->phy_mdio_port, priv->phy_type) != 0)
        {
        	ERR("bdx_resume() failed to load PHY");
        	break;
        }
        if (bdx_reset(priv) != 0)
        {
         	ERR("bdx_resume() bdx_reset failed\n");
         	break;
        }
        WRITE_REG(priv, regMDIO_CMD_STAT, 0x3ec8);
        if (bdx_start(priv, FW_LOAD))
        {
        	break;
        }
        rc = 0;
    } while(0);

    return rc;

} // bdx_resume()
#endif

//-------------------------------------------------------------------------------------------------
#ifdef _DRIVER_RESUME_
__refdata static struct dev_pm_ops bdx_pm_ops =
{
	.suspend = bdx_suspend,
	.resume_noirq = bdx_resume,
	.freeze = bdx_suspend,
	.restore_noirq = bdx_resume,
};
#endif
__refdata static struct pci_driver bdx_pci_driver =
{
    .name     = BDX_DRV_NAME,
    .id_table = bdx_pci_tbl,
    .probe    = bdx_probe,
    .remove   = __exit_p(bdx_remove),
    .shutdown   = __exit_p(bdx_remove),
#ifdef _DRIVER_RESUME_
    .driver.pm = &bdx_pm_ops,
//	.suspend  = bdx_suspend,
//	.resume   = bdx_resume,
#endif

};

//-------------------------------------------------------------------------------------------------
#ifndef _DRIVER_RESUME_

int bdx_no_hotplug(struct pci_dev *pdev, const struct pci_device_id *ent)
{

	ERR("rescan/hotplug is *NOT* supported!, please use rmmod/insmod instead\n");
	RET(-1);

} // bdx_no_hotplug

#endif
//-------------------------------------------------------------------------------------------------

static void __init bdx_scan_pci(void)
{
	int j, nDevices=0, nLoaded;
	struct pci_dev *dev = NULL;

	// count our devices
	for (j = 0; j < sizeof(bdx_pci_tbl) / sizeof(struct pci_device_id); j++)
	{
		while ((dev = pci_get_subsys(bdx_pci_tbl[j].vendor, bdx_pci_tbl[j].device, bdx_pci_tbl[j].subvendor, bdx_pci_tbl[j].subdevice, dev)))
		{
			nDevices += 1;
			MSG("%d %04x:%04x:%04x:%04x\n",nDevices, bdx_pci_tbl[j].vendor, bdx_pci_tbl[j].device, bdx_pci_tbl[j].subvendor, bdx_pci_tbl[j].subdevice);
			if (nDevices > 20)
			{
				ERR("to many devices detected ?!\n");
				break;
			}
		}

	}
	spin_lock(&g_lock);
	g_ndevices = nDevices;
	g_ndevices_loaded += 1;
	nLoaded = g_ndevices_loaded;
#ifndef _DRIVER_RESUME_
	if (g_ndevices_loaded >= g_ndevices)	// all loaded
	{
		bdx_pci_driver.probe = bdx_no_hotplug;
	}
#endif
	spin_unlock(&g_lock);
	MSG("detected %d cards, %d loaded\n", nDevices, nLoaded);

} // bdx_scan_pci()

//-------------------------------------------------------------------------------------------------

static void bdx_print_phys(void)
{
	MSG("Supported phys : %s %s %s %s %s %s %s %s\n",
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
//		bdx_pci_driver.driver.pm != NULL ? "in driver suspend" : "");

} // bdx_print_phys()


//-------------------------------------------------------------------------------------------------

/*
 * print_driver_id - Print the driver build parameters the .
 */
static void __init print_driver_id(void)
{
    MSG("%s, %s\n", BDX_DRV_DESC, BDX_DRV_VERSION);
    bdx_print_phys();
}

static int __init bdx_module_init(void)
{

    ENTER;
#ifdef __BIG_ENDIAN
    bdx_firmware_endianess();
#endif
    traceInit();
    init_txd_sizes();
    print_driver_id();
    RET(pci_register_driver(&bdx_pci_driver));
}

module_init(bdx_module_init);

//-------------------------------------------------------------------------------------------------

static void __exit bdx_module_exit(void)
{
    ENTER;
    pci_unregister_driver(&bdx_pci_driver);
    MSG("Driver unloaded\n");
    RET();
}


module_exit(bdx_module_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION(BDX_DRV_VERSION);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(BDX_DRV_DESC);
