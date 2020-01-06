/*******************************************************************************
 *
 * tn40xx_ Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *******************************************************************************/
#ifndef _TN40XX_H
#define _TN40XX_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/crc32.h>
#include <linux/msi.h>
#include <linux/list.h>
#ifndef __VMKLNX__
#include <linux/uaccess.h>
#endif
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/sched.h>
#ifndef __VMKLNX__
#include <linux/tty.h>
#endif
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <asm/byteorder.h>
#include "tn40_ioctl.h"
#ifdef __VMKLNX__
#define VM_KLNX
#include <linux/mm.h>
#endif /* __VMKLNX__ */

#ifdef _DRIVER_RESUME_
#undef __init
#define __init
#undef __initdata
#define __initdata
#endif

#define BDX_DRV_VERSION   	"0.3.6.17.2"
#define DRIVER_AUTHOR     	"Tehuti networks"
#define BDX_DRV_DESC      	"Tehuti Network Driver"
#define BDX_NIC_NAME      	"tn40xx"
#define BDX_DRV_NAME      	"tn40xx"

//#define _EEE_
#define TN40_THUNDERBOLT
#define TN40_IRQ_MSI
//#define TN40_PTP
/*
 * Trace log
 */
#ifdef _TRACE_LOG_
#include "trace.h"
#else
#define traceInit()
#define traceOn()
#define traceOff()
#define traceOnce()
#define tracePrint()
#define traceAdd(loc, val)
#endif
/* Debugging Macros */

#define ERR(fmt, args...)  printk(KERN_ERR BDX_DRV_NAME": "fmt, ## args)
#define MSG(fmt, args...)  printk(KERN_ERR BDX_DRV_NAME": "fmt, ## args)

//#define BDX_ASSERT(x) BUG_ON(x)

#define BDX_ASSERT(x)
#define TN40_ASSERT(x, fmt, args...) if (!(x)) printk(KERN_ERR  BDX_DRV_NAME" ASSERT : ""%s:%-5d: " fmt, __func__, __LINE__, ## args)

#define TN40_DEBUG
#define FTRACE
//#define REGLOG
#define WARNING

#ifdef WARNING
#define WRN(fmt, args...)   if (g_dbg)printk(KERN_ERR  BDX_DRV_NAME": " fmt,  ## args)
#else
#define WRN(fmt, args...)
#endif
//				M E M L O G
#ifdef TN40_MEMLOG
#include "memLog.h"
extern int g_memLog;
#define MEMLOG_ON							g_memLog = 1
#define MEMLOG1_ON							g_memLog = 2
#define MEMLOG_OFF							g_memLog = 0
//#define DBG(arg...)		                	if (g_memLog) memLog(TN40_DRV_NAME": " arg)

#else
#define MEMLOG_ON
#define MEMLOG1_ON
#define MEMLOG_OFF
#define memLog(args, ...)
#define memLogInit()
#define memLogDmesg()
#define memLogGetLine(x) (0)
#define memLogPrint()
#endif
//				D E B U G
#define	STRING_FMT							"%s"
#if defined(TN40_DEBUG)
extern int g_dbg;
#define DBG_ON								g_dbg = 1
#define DBG1_ON								g_dbg = 2
#define DBG_OFF								g_dbg = 0
#else
#define DBG_ON
#define DBG_OFF
#endif

#if defined(TN40_DEBUG) && defined(TN40_MEMLOG)
#define DBG(fmt, args...)					if (g_memLog) 	  memLog(fmt, ##args); else if (g_dbg) 	 printk(KERN_ERR  BDX_DRV_NAME": ""%s:%-5d: " fmt, __func__, __LINE__, ## args)
#define DBG1(fmt, args...)		            if (g_memLog > 1) memLog(fmt, ##args); else if (g_dbg > 1) printk(KERN_ERR  BDX_DRV_NAME": ""%s:%-5d: " fmt, __func__, __LINE__, ## args)
#elif defined(TN40_MEMLOG)
#define DBG(fmt, args...)					if (g_memLog) 	  memLog(fmt, ##args)
#define DBG1(fmt, args...)		            if (g_memLog > 1) memLog(fmt, ##args)
#elif defined(TN40_DEBUG)
#define	DBG(fmt, args...)					if (g_dbg) 		  printk(KERN_ERR  BDX_DRV_NAME": ""%s:%-5d: " fmt, __func__, __LINE__, ## args)
#define DBG1(fmt, args...)		            if (g_dbg > 1)    printk(KERN_ERR  BDX_DRV_NAME": ""%s:%-5d: " fmt, __func__, __LINE__, ## args)
#else
#define DBG(fmt, args...)
#define DBG1(fmt, args...)
#endif

//				F T R A C E
#if defined(FTRACE)
extern int g_ftrace;
#define FTRACE_ON		g_ftrace = 1
#define FTRACE_OFF		g_ftrace = 0
//#define ENTER        do { if (g_ftrace) printk(KERN_ERR  "%s:%-5d: - enter\n", __func__, __LINE__); } while (0)
//#define RET(args...) do { if (g_ftrace) printk(KERN_ERR  "%s:%-5d: - return\n", __func__, __LINE__); return args; } while (0)
#define ENTER        do { if (g_ftrace) printk(KERN_ERR  BDX_DRV_NAME": %s: - enter\n" , __func__); } while (0)
#define RET(args...) do { if (g_ftrace) printk(KERN_ERR  BDX_DRV_NAME": %s: - return\n", __func__); return args; } while (0)
#define EXIT         do { if (g_ftrace) printk(KERN_ERR  BDX_DRV_NAME": %s(): - exit\n", __func__);return;} while (0)
#else
#define FTRACE_ON
#define FTRACE_OFF
#define ENTER
#define EXIT
#define RET(args...)   		                return args
#endif

#ifdef REGLOG
extern int g_regLog;
#define REGLOG_ON							g_regLog = 1
#define REGLOG_OFF							g_regLog = 0
u32  bdx_readl(void *base, u32 reg);
#define READ_REG(pp, reg)     bdx_readl(pp->pBdxRegs, reg)
#define WRITE_REG(pp, reg, val) { \
  if (g_regLog) MSG("regW 0x%x 0x%x\n", reg, val );\
  writel(val, pp->pBdxRegs + reg); \
}
#else
#define REGLOG_ON
#define REGLOG_OFF
#ifdef TN40_THUNDERBOLT
struct bdx_priv;
u32 tbReadReg(struct bdx_priv *priv, u32 reg);
#define READ_REG(pp, reg)     tbReadReg(pp, reg)
#else
#define READ_REG(pp, reg)     readl(pp->pBdxRegs + reg)
#endif
#define WRITE_REG(pp, reg, val)   writel(val, pp->pBdxRegs + reg)

#endif

#define TEHUTI_VID  			0x1FC9
#define DLINK_VID  				0x1186
#define ASUS_VID				0x1043
#define EDIMAX_VID				0x1432
#define PROMISE_VID				0x105a
#define BUFFALO_VID				0x1154

#define MDIO_SPEED_1MHZ 		(1)
#define MDIO_SPEED_6MHZ			(6)

/* Driver states */

#define	BDX_STATE_NONE			(0x00000000)
#define	BDX_STATE_HW_STOPPED	(0x00000001)
#define BDX_STATE_STARTED		(0x00000002)
#define BDX_STATE_OPEN			(0x00000004)

enum LOAD_FW
{
	NO_FW_LOAD=0,
	FW_LOAD,
};

/* Supported PHYs */

enum PHY_TYPE
{
    PHY_TYPE_NA,      		/* Port not exists or unknown PHY	*/
    PHY_TYPE_CX4,     		/* PHY works without initialization */
    PHY_TYPE_QT2025,  		/* QT2025 10 Gbps SFP+    			*/
    PHY_TYPE_MV88X3120,  	/* MV88X3120  10baseT  				*/
    PHY_TYPE_MV88X3310,  	/* MV88X3310  10NbaseT 				*/
    PHY_TYPE_MV88E2010,  	/* MV88E2010  5NbaseT  				*/
    PHY_TYPE_TLK10232,		/* TI TLK10232 SFP+					*/
    PHY_TYPE_AQR105,		/* AQR105 10baseT					*/
    PHY_TYPE_CNT
};

enum PHY_LEDS_OP
{
    PHY_LEDS_SAVE,
    PHY_LEDS_RESTORE,
    PHY_LEDS_ON,
    PHY_LEDS_OFF,
    PHY_LEDS_CNT
};

/* Supported devices */
struct bdx_device_descr
{
    short vid;
    short pid;
    short subdev;
    short msi;
    short ports;
    short phya;
    short phyb;
    char  *name;
};

/* Compile Time Switches */
/* start */
//#define USE_TIMERS
#define BDX_MSI
/* end */

#define BDX_DEF_MSG_ENABLE  (NETIF_MSG_DRV   | \
                 NETIF_MSG_PROBE | \
                 NETIF_MSG_LINK)

/* RX copy break size */
#define BDX_COPYBREAK     	257
#define BDX_MSI_STRING 		"msi "
/*
 * netdev tx queue len for Luxor. The default value is 1000.
 * ifconfig eth1 txqueuelen 3000 - to change it at runtime.
 */
#define BDX_NDEV_TXQ_LEN    3000

#define FIFO_SIZE       	4096
#define FIFO_EXTRA_SPACE    1024

#define MIN(x, y)  ((x) < (y) ? (x) : (y))
#define MAX(x, y)  ((x) > (y) ? (x) : (y))

#if BITS_PER_LONG == 64
#       define H32_64(x)  (u32) ((u64)(x) >> 32)
#       define L32_64(x)  (u32) ((u64)(x) & 0xffffffff)
#elif BITS_PER_LONG == 32
#       define H32_64(x)  0
#       define L32_64(x)  ((u32) (x))
#else               /* BITS_PER_LONG == ?? */
#       error BITS_PER_LONG is undefined. Must be 64 or 32
#endif              /* BITS_PER_LONG */

#ifdef __BIG_ENDIAN
#       define CPU_CHIP_SWAP32(x) (swab32(x))
#       define CPU_CHIP_SWAP16(x) (swab16(x))
#else
#       define CPU_CHIP_SWAP32(x) (x)
#       define CPU_CHIP_SWAP16(x) (x)
#endif


#ifndef DMA_64BIT_MASK
#       define DMA_64BIT_MASK  0xffffffffffffffffULL
#endif

#ifndef DMA_32BIT_MASK
#       define DMA_32BIT_MASK  0x00000000ffffffffULL
#endif

#ifndef NET_IP_ALIGN
#       define NET_IP_ALIGN 2
#endif

#ifndef NETDEV_TX_OK
#       define NETDEV_TX_OK 0
#endif

#define LUXOR_MAX_PORT     2
#define BDX_MAX_RX_DONE    150
#define BDX_TXF_DESC_SZ    16
#define BDX_MAX_TX_LEVEL   (priv->txd_fifo0.m.memsz - 16)
#define BDX_MIN_TX_LEVEL   256
#define BDX_NO_UPD_PACKETS 40
#define BDX_MAX_MTU		   (1 << 14)
#define BDX_IRQ_TYPE    IRQF_SHARED
struct pci_nic
{
    int port_num;
    void __iomem *regs;
    int irq_type;
    struct bdx_priv *priv;
};

enum { IRQ_INTX, IRQ_MSI, IRQ_MSIX };

#define PCK_TH_MULT   128
#define INT_COAL_MULT 2

#define BITS_MASK(nbits)            ((1 << nbits)-1)
#define GET_BITS_SHIFT(x, nbits, nshift)    (((x) >> nshift)&BITS_MASK(nbits))
#define BITS_SHIFT_MASK(nbits, nshift)      (BITS_MASK(nbits) << nshift)
#define BITS_SHIFT_VAL(x, nbits, nshift)    (((x) & BITS_MASK(nbits)) << nshift)
#define BITS_SHIFT_CLEAR(x, nbits, nshift)  \
        ((x)&(~BITS_SHIFT_MASK(nbits, nshift)))

#define GET_INT_COAL(x)             GET_BITS_SHIFT(x, 15,  0)
#define GET_INT_COAL_RC(x)          GET_BITS_SHIFT(x,  1, 15)
#define GET_RXF_TH(x)               GET_BITS_SHIFT(x,  4, 16)
#define GET_PCK_TH(x)               GET_BITS_SHIFT(x,  4, 20)

#define INT_REG_VAL(coal, coal_rc, rxf_th, pck_th)  \
    ((coal) | ((coal_rc) << 15) | ((rxf_th) << 16) | ((pck_th) << 20))

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
#define USE_PAGED_BUFFERS             1
//#define RX_REUSE_PAGES
#if defined(RX_REUSE_PAGES) && !defined(USE_PAGED_BUFFERS)
#define USE_PAGED_BUFFERS
#endif

struct fifo
{
    dma_addr_t  da;     /* Physical address of fifo (used by HW) */
    char        *va;    /* Virtual address of fifo (used by SW) */
    u32     rptr, wptr; /* Cached values of RPTR and WPTR registers,
                   they're 32 bits on both 32 and 64 archs */
    u16     reg_CFG0, reg_CFG1;
    u16     reg_RPTR, reg_WPTR;
    u16     memsz;  /* Memory size allocated for fifo */
    u16     size_mask;
    u16     pktsz;  /* Skb packet size to allocate */
    u16     rcvno;  /* Number of buffers that come from this RXF */
};

struct txf_fifo
{
    struct fifo m;   /* The minimal set of variables used by all fifos  */
};

struct txd_fifo
{
    struct fifo m;   /* The minimal set of variables used by all fifos */
};

struct rxf_fifo
{
    struct fifo m;   /* The minimal set of variables used by all fifos */
};

struct rxd_fifo
{
    struct fifo m;   /* The minimal set of variables used by all fifos */
};

struct bdx_page
{
#ifdef RX_REUSE_PAGES
	struct list_head			free;
	int							ref_count;
	int							reuse_tries;
	int							page_index;
	char						status;
#endif
	struct page 				*page;
	u64 						dma;
};
struct rx_map
{
#ifdef RX_REUSE_PAGES
    struct bdx_page *bdx_page;
#else
    struct bdx_page bdx_page;
#endif
    u64 			dma;
    struct sk_buff  *skb;
#ifdef USE_PAGED_BUFFERS
    u32     		off;
    u32     		size;   /* Mapped area (i.e. page) size */
#endif
};

struct rxdb
{
    int     *stack;
    struct rx_map   *elems;
    int     nelem;
    int     top;
    void	*pkt;
};

union bdx_dma_addr
{
    dma_addr_t  dma;
    struct sk_buff  *skb;
};

/*
 * Entry in the db.
 * if len == 0 addr is dma
 * if len != 0 addr is skb
 */
struct tx_map
{
    union bdx_dma_addr addr;
    int len;
};

/* tx database - implemented as circular fifo buffer*/
struct txdb
{
    struct tx_map   *start; /* Points to the first element     */
    struct tx_map   *end;   /* Points just AFTER the last element  */
    struct tx_map   *rptr;  /* Points to the next element to read  */
    struct tx_map   *wptr;  /* Points to the next element to write */
    int     size;   /* Number of elements in the db    */
};

/*Internal stats structure*/
struct bdx_stats
{
    u64 InUCast;                /* 0x7200 */
    u64 InMCast;                /* 0x7210 */
    u64 InBCast;                /* 0x7220 */
    u64 InPkts;         /* 0x7230 */
    u64 InErrors;               /* 0x7240 */
    u64 InDropped;              /* 0x7250 */
    u64 FrameTooLong;       /* 0x7260 */
    u64 FrameSequenceErrors;    /* 0x7270 */
    u64 InVLAN;         /* 0x7280 */
    u64 InDroppedDFE;       /* 0x7290 */
    u64 InDroppedIntFull;           /* 0x72A0 */
    u64 InFrameAlignErrors;         /* 0x72B0 */

    /* 0x72C0-0x72E0 RSRV */

    u64 OutUCast;               /* 0x72F0 */
    u64 OutMCast;               /* 0x7300 */
    u64 OutBCast;               /* 0x7310 */
    u64 OutPkts;                /* 0x7320 */

    /* 0x7330-0x7360 RSRV */

    u64 OutVLAN;                /* 0x7370 */
    u64 InUCastOctects;         /* 0x7380 */
    u64 OutUCastOctects;            /* 0x7390 */

    /* 0x73A0-0x73B0 RSRV */

    u64 InBCastOctects;         /* 0x73C0 */
    u64 OutBCastOctects;            /* 0x73D0 */
    u64 InOctects;              /* 0x73E0 */
    u64 OutOctects;             /* 0x73F0 */
};
#define PHY_LEDS				(4)
struct bdx_phy_operations
{
	unsigned short leds[PHY_LEDS];
    int  (*mdio_reset)  	  (struct bdx_priv *, 	int, unsigned short);
	u32  (*link_changed)	  (struct bdx_priv *);
	int  (*get_settings)	  (struct net_device *, struct ethtool_cmd *);
	void (*ledset)      	  (struct bdx_priv *, 	enum   PHY_LEDS_OP);
    int  (*set_settings)	  (struct net_device *, struct ethtool_cmd *);
#ifdef ETHTOOL_GLINKSETTINGS
    int	 (*get_link_ksettings)(struct net_device *, struct ethtool_link_ksettings *);
#endif
#ifdef ETHTOOL_SLINKSETTINGS
    int	 (*set_link_ksettings)(struct net_device *, const struct ethtool_link_ksettings *);
#endif
#ifdef _EEE_
#ifdef ETHTOOL_GEEE
    int  (*get_eee)		(struct net_device *, struct ethtool_eee *);
#endif
#ifdef ETHTOOL_SEEE
    int  (*set_eee)		(struct bdx_priv *);
#endif
    int  (*reset_eee)	(struct bdx_priv *);
#endif
    u32  mdio_speed;
};

struct bdx_rx_page_table
{
	int							page_size;
	int							buf_size;
	struct bdx_page				*bdx_pages;
#ifdef RX_REUSE_PAGES
	int							nPages;
	int							nFrees;
	int							max_frees;
	int							page_order;
	gfp_t 						gfp_mask;
	int							nBufInPage;
	struct list_head			free_list;
#endif

};

struct bdx_priv
{
    void __iomem            	*pBdxRegs;
    struct net_device   		*ndev;
    struct napi_struct 	 		napi;
    /* RX FIFOs: 1 for data (full) descs, and 2 for free descs */
    struct rxd_fifo         	rxd_fifo0;
    struct rxf_fifo         	rxf_fifo0;
    struct rxdb     			*rxdb0;      /* Rx dbs to store skb pointers */
    int         				napi_stop;
    struct vlan_group   		*vlgrp;
    /* Tx FIFOs: 1 for data desc, 1 for empty (acks) desc */
    struct txd_fifo         	txd_fifo0;
    struct txf_fifo         	txf_fifo0;
    struct txdb     			txdb;
    int         				tx_level;
    int         				tx_update_mark;
    int         				tx_noupd;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 7,0)
    spinlock_t      			tx_lock;    /* NETIF_F_LLTX mode */
#endif
    /* Rarely used */
    u8          				port;
    u8          				phy_mdio_port;
    enum PHY_TYPE           	phy_type;
    u32         				msg_enable;
    int         				stats_flag;
    struct bdx_stats   			hw_stats;
    struct net_device_stats 	net_stats;
    struct pci_dev          	*pdev;
    struct pci_nic          	*nic;
    u8          				txd_size;
    u8          				txf_size;
    u8          				rxd_size;
    u8          				rxf_size;
    u32         				rdintcm;
    u32         				tdintcm;
    u16         				id;
    u16         				count;
    struct timer_list   		blink_timer;
    u32  						isr_mask;
    int  						link_speed; // 0- no link or SPEED_100 SPEED_1000 SPEED_10000
    u32  						link_loop_cnt;
    u32							sfp_mod_type; 
    struct bdx_phy_operations 	phy_ops;
    // SHORT_PKT_FIX
    u32         				b0_len;
	dma_addr_t  				b0_dma;	/* Physical address of buffer */
	char	    				*b0_va;	/* Virtual address of buffer */
	char						*drv_name;	/* device driver name */
	u32							errmsg_count;
	u32							state;
	// SHORT_PKT_FIX end
	u16							deviceId;
	u16							subsystem_vendor;
	u16							subsystem_device;
    u32							advertising, autoneg;
#ifdef __ETHTOOL_DECLARE_LINK_MODE_MASK
    __ETHTOOL_DECLARE_LINK_MODE_MASK(link_advertising);
#endif
	u32							eee_enabled;
    struct bdx_cMem				*cMem;
    struct bdx_rx_page_table	rx_page_table;
#ifdef TN40_THUNDERBOLT
    int							bDeviceRemoved;
#endif
};
#if !defined(SPEED_5000)
#define SPEED_5000 				(5000)
#endif
#if !defined(SPEED_2500)
#define SPEED_2500 				(2500)
#endif
#define SPEED_1000X 			(1001)
#define SPEED_100X 				(101)

#define MAX_ERRMSGS				(3)
/* RX FREE descriptor - 64bit*/
struct rxf_desc
{
    u32         info;       /* Buffer Count + Info - described below */
    u32         va_lo;      /* VAdr[31:0]    */
    u32         va_hi;      /* VAdr[63:32]   */
    u32         pa_lo;      /* PAdr[31:0]    */
    u32         pa_hi;      /* PAdr[63:32]   */
    u32         len;        /* Buffer Length */
};

#define GET_RXD_BC(x)       GET_BITS_SHIFT((x), 5, 0)
#define GET_RXD_RXFQ(x)     GET_BITS_SHIFT((x), 2, 8)
#define GET_RXD_TO(x)       GET_BITS_SHIFT((x), 1, 15)
#define GET_RXD_TYPE(x)     GET_BITS_SHIFT((x), 4, 16)
#define GET_RXD_ERR(x)      GET_BITS_SHIFT((x), 6, 21)
#define GET_RXD_RXP(x)      GET_BITS_SHIFT((x), 1, 27)
#define GET_RXD_PKT_ID(x)   GET_BITS_SHIFT((x), 3, 28)
#define GET_RXD_VTAG(x)     GET_BITS_SHIFT((x), 1, 31)
#define GET_RXD_VLAN_ID(x)  GET_BITS_SHIFT((x), 12, 0)
#define GET_RXD_VLAN_TCI(x) GET_BITS_SHIFT((x), 16, 0)
#define GET_RXD_CFI(x)      GET_BITS_SHIFT((x), 1, 12)
#define GET_RXD_PRIO(x)     GET_BITS_SHIFT((x), 3, 13)

#define GET_RSS_FUNC(x)     GET_BITS_SHIFT((x), 2, 0)
#define GET_RSS_TYPE(x)     GET_BITS_SHIFT((x), 8, 8)
#define GET_RSS_TCPU(x)     GET_BITS_SHIFT((x), 8, 16)

struct rxd_desc
{
    u32         rxd_val1;
    u16         len;
    u16         rxd_vlan;
    u32         va_lo;
    u32         va_hi;
    u32         rss_lo;
    u32         rss_hash;
};

#define MAX_PBL				(19)
/* PBL describes each virtual buffer to be transmitted from the host. */
struct pbl
{
    u32 pa_lo;
    u32 pa_hi;
    u32 len;
};

/*
 * First word for TXD descriptor. It means: type = 3 for regular Tx packet,
 * hw_csum = 7 for IP+UDP+TCP HW checksums.
 */
#define TXD_W1_VAL(bc, checksum, vtag, lgsnd, vlan_id)  \
    ((bc) | ((checksum) << 5)  | ((vtag) << 8) |    \
    ((lgsnd) << 9) | (0x30000) | ((vlan_id & 0x0fff) << 20) | (((vlan_id >> 13) & 7) << 13))

struct txd_desc
{
    u32             txd_val1;
    u16             mss;
    u16             length;
    u32             va_lo;
    u32             va_hi;
    struct pbl      pbl[0]; /* Fragments */
} __attribute__ ((packed));

struct txf_desc
{
    u32                                     status;
    u32                                     va_lo;      /* VAdr[31:0]    						 */
    u32                                     va_hi;      /* VAdr[63:32]   						 */
    u32										pad;
}__attribute__ ((packed));

/* Register region size */
#define BDX_REGS_SIZE           0x10000

/* Registers from 0x0000-0x00fc were remapped to 0x4000-0x40fc */
#define regTXD_CFG1_0           0x4000
#define regTXD_CFG1_1           0x4004
#define regTXD_CFG1_2           0x4008
#define regTXD_CFG1_3           0x400C

#define regRXF_CFG1_0           0x4010
#define regRXF_CFG1_1           0x4014
#define regRXF_CFG1_2           0x4018
#define regRXF_CFG1_3           0x401C

#define regRXD_CFG1_0           0x4020
#define regRXD_CFG1_1           0x4024
#define regRXD_CFG1_2           0x4028
#define regRXD_CFG1_3           0x402C

#define regTXF_CFG1_0           0x4030
#define regTXF_CFG1_1           0x4034
#define regTXF_CFG1_2           0x4038
#define regTXF_CFG1_3           0x403C

#define regTXD_CFG0_0           0x4040
#define regTXD_CFG0_1           0x4044
#define regTXD_CFG0_2           0x4048
#define regTXD_CFG0_3           0x404C

#define regRXF_CFG0_0           0x4050
#define regRXF_CFG0_1           0x4054
#define regRXF_CFG0_2           0x4058
#define regRXF_CFG0_3           0x405C

#define regRXD_CFG0_0           0x4060
#define regRXD_CFG0_1           0x4064
#define regRXD_CFG0_2           0x4068
#define regRXD_CFG0_3           0x406C

#define regTXF_CFG0_0           0x4070
#define regTXF_CFG0_1           0x4074
#define regTXF_CFG0_2           0x4078
#define regTXF_CFG0_3           0x407C

#define regTXD_WPTR_0           0x4080
#define regTXD_WPTR_1           0x4084
#define regTXD_WPTR_2           0x4088
#define regTXD_WPTR_3           0x408C

#define regRXF_WPTR_0           0x4090
#define regRXF_WPTR_1           0x4094
#define regRXF_WPTR_2           0x4098
#define regRXF_WPTR_3           0x409C

#define regRXD_WPTR_0           0x40A0
#define regRXD_WPTR_1           0x40A4
#define regRXD_WPTR_2           0x40A8
#define regRXD_WPTR_3           0x40AC

#define regTXF_WPTR_0           0x40B0
#define regTXF_WPTR_1           0x40B4
#define regTXF_WPTR_2           0x40B8
#define regTXF_WPTR_3           0x40BC

#define regTXD_RPTR_0           0x40C0
#define regTXD_RPTR_1           0x40C4
#define regTXD_RPTR_2           0x40C8
#define regTXD_RPTR_3           0x40CC

#define regRXF_RPTR_0           0x40D0
#define regRXF_RPTR_1           0x40D4
#define regRXF_RPTR_2           0x40D8
#define regRXF_RPTR_3           0x40DC

#define regRXD_RPTR_0           0x40E0
#define regRXD_RPTR_1           0x40E4
#define regRXD_RPTR_2           0x40E8
#define regRXD_RPTR_3           0x40EC

#define regTXF_RPTR_0           0x40F0
#define regTXF_RPTR_1           0x40F4
#define regTXF_RPTR_2           0x40F8
#define regTXF_RPTR_3           0x40FC


/* Hardware versioning */
#define  FW_VER             0x5010
#define  SROM_VER           0x5020
#define  FPGA_VER           0x5030
#define  FPGA_SEED          0x5040

/* Registers from 0x0100-0x0150 were remapped to 0x5100-0x5150 */
#define regISR regISR0
#define regISR0             0x5100

#define regIMR regIMR0
#define regIMR0             0x5110

#define regRDINTCM0         0x5120
#define regRDINTCM2         0x5128

#define regTDINTCM0         0x5130

#define regISR_MSK0         0x5140

#define regINIT_SEMAPHORE       0x5170
#define regINIT_STATUS          0x5180

#define regMAC_LNK_STAT         0x0200
#define MAC_LINK_STAT           0x0004      /* Link state */

#define regBLNK_LED         0x0210

#define regGMAC_RXF_A           0x1240

#define regUNC_MAC0_A           0x1250
#define regUNC_MAC1_A           0x1260
#define regUNC_MAC2_A           0x1270

#define regVLAN_0           0x1800

#define regMAX_FRAME_A          0x12C0

#define regRX_MAC_MCST0         0x1A80
#define regRX_MAC_MCST1         0x1A84
#define MAC_MCST_NUM            15
#define regRX_MCST_HASH0        0x1A00
#define MAC_MCST_HASH_NUM        8

#define regVPC              0x2300
#define regVIC              0x2320
#define regVGLB             0x2340

#define regCLKPLL           0x5000


/* MDIO interface */


#define regMDIO_CMD_STAT	0x6030
#define regMDIO_CMD			0x6034
#define regMDIO_DATA		0x6038
#define regMDIO_ADDR		0x603C
#define GET_MDIO_BUSY(x)	GET_BITS_SHIFT(x, 1, 0)
#define GET_MDIO_RD_ERR(x)	GET_BITS_SHIFT(x, 1, 1)

/*for 10G only*/
#define regRSS_CNG      0x000000b0

#define RSS_ENABLED     0x00000001
#define RSS_HFT_TOEPLITZ    0x00000002
#define RSS_HASH_IPV4       0x00000100
#define RSS_HASH_TCP_IPV4   0x00000200
#define RSS_HASH_IPV6       0x00000400
#define RSS_HASH_IPV6_EX    0x00000800
#define RSS_HASH_TCP_IPV6   0x00001000
#define RSS_HASH_TCP_IPV6_EX    0x00002000

#define regRSS_HASH_BASE        0x0400
#define RSS_HASH_LEN            40
#define regRSS_INDT_BASE        0x0600
#define RSS_INDT_LEN               256


#define regREVISION         0x6000
#define regSCRATCH          0x6004
#define regCTRLST           0x6008
#define regMAC_ADDR_0           0x600C
#define regMAC_ADDR_1           0x6010
#define regFRM_LENGTH           0x6014
#define regPAUSE_QUANT          0x6054
#define regRX_FIFO_SECTION      0x601C
#define regTX_FIFO_SECTION      0x6020
#define regRX_FULLNESS          0x6024
#define regTX_FULLNESS          0x6028
#define regHASHTABLE            0x602C

#define regRST_PORT         0x7000
#define regDIS_PORT         0x7010
#define regRST_QU           0x7020
#define regDIS_QU           0x7030

#define regCTRLST_TX_ENA        0x0001
#define regCTRLST_RX_ENA        0x0002
#define regCTRLST_PRM_ENA       0x0010
#define regCTRLST_PAD_ENA       0x0020

#define regCTRLST_BASE          (regCTRLST_PAD_ENA|regCTRLST_PRM_ENA)

#define regRX_FLT               0x1400

/* TXD TXF RXF RXD  CONFIG 0x0000 --- 0x007c*/
#define  TX_RX_CFG1_BASE    0xffffffff      /*0-31 */
#define  TX_RX_CFG0_BASE    0xfffff000      /*31:12 */
#define  TX_RX_CFG0_RSVD    0x00000ffc      /*11:2 */
#define  TX_RX_CFG0_SIZE    0x00000003      /*1:0 */

/*  TXD TXF RXF RXD  WRITE 0x0080 --- 0x00BC */
#define  TXF_WPTR_WR_PTR    0x00007ff8      /*14:3 */

/*  TXD TXF RXF RXD  READ  0x00CO --- 0x00FC */
#define  TXF_RPTR_RD_PTR    0x00007ff8     /*14:3 */

#define TXF_WPTR_MASK 0x7ff0    /* The last 4 bits are dropped
* size is rounded to 16 */

/*  regISR 0x0100 */
/*  regIMR 0x0110 */
#define  IMR_INPROG     0x80000000    /*31 */
#define  IR_LNKCHG1     0x10000000    /*28 */
#define  IR_LNKCHG0     0x08000000    /*27 */
#define  IR_GPIO        0x04000000    /*26 */
#define  IR_RFRSH       0x02000000    /*25 */
#define  IR_RSVD        0x01000000    /*24 */
#define  IR_SWI         0x00800000    /*23 */
#define  IR_RX_FREE_3   0x00400000    /*22 */
#define  IR_RX_FREE_2   0x00200000    /*21 */
#define  IR_RX_FREE_1   0x00100000    /*20 */
#define  IR_RX_FREE_0   0x00080000    /*19 */
#define  IR_TX_FREE_3   0x00040000    /*18 */
#define  IR_TX_FREE_2   0x00020000    /*17 */
#define  IR_TX_FREE_1   0x00010000    /*16 */
#define  IR_TX_FREE_0   0x00008000    /*15 */
#define  IR_RX_DESC_3   0x00004000    /*14 */
#define  IR_RX_DESC_2   0x00002000    /*13 */
#define  IR_RX_DESC_1   0x00001000    /*12 */
#define  IR_RX_DESC_0   0x00000800    /*11 */
#define  IR_PSE         0x00000400    /*10 */
#define  IR_TMR3        0x00000200    /* 9 */
#define  IR_TMR2        0x00000100    /* 8 */
#define  IR_TMR1        0x00000080    /* 7 */
#define  IR_TMR0        0x00000040    /* 6 */
#define  IR_VNT         0x00000020    /* 5 */
#define  IR_RxFL        0x00000010    /* 4 */
#define  IR_SDPERR      0x00000008    /* 3 */
#define  IR_TR          0x00000004    /* 2 */
#define  IR_PCIE_LINK   0x00000002    /* 1 */
#define  IR_PCIE_TOUT   0x00000001    /* 0 */

#define  IR_EXTRA (IR_RX_FREE_0 | IR_LNKCHG0 | IR_LNKCHG1 | IR_PSE | \
    IR_TMR0 | IR_PCIE_LINK | IR_PCIE_TOUT)
#define  IR_RUN (IR_EXTRA | IR_RX_DESC_0 | IR_TX_FREE_0)
#define  IR_ALL 0xfdfffff7

#define  IR_LNKCHG0_ofst    27

#define  GMAC_RX_FILTER_OSEN    0x1000  /* shared OS enable */
#define  GMAC_RX_FILTER_TXFC    0x0400  /* Tx flow control */
#define  GMAC_RX_FILTER_RSV0    0x0200  /* reserved */
#define  GMAC_RX_FILTER_FDA     0x0100  /* filter out direct address */
#define  GMAC_RX_FILTER_AOF     0x0080  /* accept over run */
#define  GMAC_RX_FILTER_ACF     0x0040  /* accept control frames */
#define  GMAC_RX_FILTER_ARUNT   0x0020  /* accept under run */
#define  GMAC_RX_FILTER_ACRC    0x0010  /* accept crc error */
#define  GMAC_RX_FILTER_AM      0x0008  /* accept multicast */
#define  GMAC_RX_FILTER_AB      0x0004  /* accept broadcast */
#define  GMAC_RX_FILTER_PRM     0x0001  /* [0:1] promiscuous mode */

#define  MAX_FRAME_AB_VAL   0x3fff  /* 13:0 */

#define  CLKPLL_PLLLKD      0x0200  /* 9 */
#define  CLKPLL_RSTEND      0x0100  /* 8 */
#define  CLKPLL_SFTRST      0x0001  /* 0 */

#define  CLKPLL_LKD     (CLKPLL_PLLLKD|CLKPLL_RSTEND)

/*
 * PCI-E Device Control Register (Offset 0x88)
 * Source: Luxor Data Sheet, 7.1.3.3.3
 */
#define PCI_DEV_CTRL_REG 0x88
#define GET_DEV_CTRL_MAXPL(x)   GET_BITS_SHIFT(x, 3, 5)
#define GET_DEV_CTRL_MRRS(x)    GET_BITS_SHIFT(x, 3, 12)

/*
 * PCI-E Link Status Register (Offset 0x92)
 * Source: Luxor Data Sheet, 7.1.3.3.7
 */
#define PCI_LINK_STATUS_REG 0x92
#define GET_LINK_STATUS_LANES(x) GET_BITS_SHIFT(x, 6, 4)


#if defined(DMA_BIT_MASK)
#define LUXOR__DMA_64BIT_MASK   DMA_BIT_MASK(64)
#define LUXOR__DMA_32BIT_MASK   DMA_BIT_MASK(32)
#else
#define LUXOR__DMA_64BIT_MASK   DMA_64BIT_MASK
#define LUXOR__DMA_32BIT_MASK   DMA_32BIT_MASK
#endif

#if !defined(netdev_mc_count)
#define netdev_mc_count(dev) ((dev)->mc_count)
#define netdev_for_each_mc_addr(mclist, dev) \
    for (mclist = dev->mc_list; mclist; mclist = mclist->next)
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
#define dev_mc_list     netdev_hw_addr
#define dmi_addr    addr
#endif
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)) || (defined VM_KLNX))
#define LUXOR__SCHEDULE_PREP(napi, dev) napi_schedule_prep(napi)
#define LUXOR__SCHEDULE(napi, dev)  __napi_schedule(napi)
#define LUXOR__POLL_ENABLE(dev)
#define LUXOR__POLL_DISABLE(dev)
#define LUXOR__NAPI_ENABLE(napi)    napi_enable(napi)
#define LUXOR__NAPI_DISABLE(napi)   napi_disable(napi)
#define LUXOR__NAPI_ADD(dev, napi, poll, weight) \
                  netif_napi_add(dev, napi, poll, weight)
#else
#define LUXOR__SCHEDULE_PREP(napi, dev) netif_rx_schedule_prep(dev)
#define LUXOR__SCHEDULE(napi, dev)  __netif_rx_schedule(dev)
#define LUXOR__POLL_ENABLE(dev)     netif_poll_enable(dev)
#define LUXOR__POLL_DISABLE(dev)    netif_poll_disable(dev)
#define LUXOR__NAPI_ENABLE(napi)
#define LUXOR__NAPI_DISABLE(napi)
//#define LUXOR__NAPI_ADD(dev, napi, poll, weight)
#define LUXOR__NAPI_ADD(dev, napi, poll, weight) init_napi(dev, napi)

inline void
init_napi(struct net_device *dev, struct napi_struct *napi)
{
    memset(napi, 0, sizeof(*napi)) ;
    napi->dev       = dev;
}

#endif

#ifndef RHEL_RELEASE_VERSION
#define RHEL_RELEASE_VERSION(a,b) (((a) << 8) + (b))
#endif

/*
 * Note: 32 bit  kernels use 16 bits for page_offset. Do not increase
 *       LUXOR__MAX_PAGE_SIZE beyind 64K!
 */
#if BITS_PER_LONG > 32
#define LUXOR__MAX_PAGE_SIZE    0x40000
#else
#define LUXOR__MAX_PAGE_SIZE    0x10000
#endif
#elif defined(RHEL_RELEASE_CODE) && (RHEL_RELEASE_CODE >= 1285) && defined(NETIF_F_GRO)
#define USE_PAGED_BUFFERS             1
/*
 * Note: RHEL & CentOs use 16 bits for page_offset. Do not increas
 *       LUXOR__MAX_PAGE_SIZE beyind 64K!
 */
#define LUXOR__MAX_PAGE_SIZE    0x10000
void inline skb_add_rx_frag(struct sk_buff *skb, int i, struct page *page,
                            int off, int size)
{
    skb_fill_page_desc(skb, i, page, off, size);
    skb->len      += size;
    skb->data_len += size;
    skb->truesize += size;
}
#elif defined(USE_PAGED_BUFFERS) && USE_PAGED_BUFFERS == 0
#define LUXOR__MAX_PAGE_SIZE    0x10000
#undef USE_PAGED_BUFFERS
#else
#define LUXOR__MAX_PAGE_SIZE    0x10000
#endif
#if (defined(RHEL_RELEASE_CODE) && \
     (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(6,4)) && \
     (RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(7,0)))
#define RHEL6_ETHTOOL_OPS_EXT_STRUCT
#endif /* RHEL >= 6.4 && RHEL < 7.0 */
#if defined(NETIF_F_GRO)
#define LUXOR__VLAN_RECEIVE(napi, grp, vlan_tci, skb) \
    vlan_gro_receive(napi, grp, vlan_tci, skb)
#define LUXOR__RECEIVE(napi, skb)  \
    napi_gro_receive(napi, skb)
#if (defined(RHEL_RELEASE_CODE) && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(6,8)))
#define LUXOR__GRO_FLUSH(napi)  napi_gro_flush(napi,0)
#elif defined(RHEL_RELEASE_CODE) && (RHEL_RELEASE_CODE >= 1285)
#define LUXOR__GRO_FLUSH(napi)  napi_gro_flush(napi)
#else
#define LUXOR__GRO_FLUSH(napi)
#endif
#define IS_GRO(ndev)            ((ndev->features & NETIF_F_GRO) != 0)
#else
#define NETIF_F_GRO         0
#define LUXOR__VLAN_RECEIVE(napi, grp, vlan_tci, skb)   \
    vlan_hwaccel_receive_skb(skb, grp, vlan_tci)

#define LUXOR__RECEIVE(napi, skb) \
    netif_receive_skb(skb)
#define LUXOR__GRO_FLUSH(napi)
#define IS_GRO(ndev)            0
#endif

#if !defined(NETIF_F_VLAN_TSO)
#define NETIF_F_VLAN_TSO        0       /* RHEL specific flag */
#endif
#if !defined(NETIF_F_VLAN_CSUM)
#define NETIF_F_VLAN_CSUM       0       /* RHEL specific flag */
#endif
#if !defined(NETIF_F_RXHASH)
#define NETIF_F_RXHASH          0
#endif

#if !defined(MAX)
#define MAX(a, b)   ((a) > (b) ? (a) : (b))
#endif
#if !defined(MIN)
#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#endif
#if !defined(ROUND_UP)
#define ROUND_UP(n, m)  (((n) + (m - 1)) & ~(m - 1))
#endif


#if defined(USE_TIMERS)
#define CLOCK_FREQ  3000000000ULL
#define CYCLES_2_MSEC   (CLOCK_FREQ/1000)
#define CYCLES_2_USEC   (CLOCK_FREQ/1000000)
#define CYCLES_2_NSEC   (CLOCK_FREQ/1000000000)
#define TIMER_PERIOD    10ULL
#define TIMER_NAME(t)   t##_timer
#define RESET_TIMER(t)  (TIMER_NAME(t).time = TIMER_NAME(t).count = 0ULL,    \
             rdtscll(TIMER_NAME(t).start),               \
             TIMER_NAME(t).begin = TIMER_NAME(t).start)
#define PRINT_TIMER(t)  do { if (likely(TIMER_NAME(t).begin > 0))        \
                printk(KERN_ERR                  \
                       "%s: time = %llu msec count = %llu "  \
                       "period = %llu nsec\n",           \
                       TIMER_NAME(t).name,           \
                       TIMER_NAME(t).time/CYCLES_2_MSEC,     \
                       TIMER_NAME(t).count,          \
                       TIMER_NAME(t).time * CYCLES_2_NSEC/   \
                       MAX(TIMER_NAME(t).count, 1ULL)) ;   \
                 RESET_TIMER(t) ; } while (0)
#define DEF_TIMER(t)    static struct timer TIMER_NAME(t) = { .name = #t }
#define START_TIMER(t)  do { rdtscll(TIMER_NAME(t).start) ;          \
              if (TIMER_NAME(t).start > TIMER_NAME(t).begin +    \
                  TIMER_PERIOD*CLOCK_FREQ)               \
                  PRINT_TIMER(t) ;                   \
              ++TIMER_NAME(t).count ; } while(0)
#define END_TIMER(t)    do { rdtscll(TIMER_NAME(t).stop),            \
                 TIMER_NAME(t).time += TIMER_NAME(t).stop -      \
                 TIMER_NAME(t).start ; }                 \
            while ((0))

struct timer
{
    u64     begin ;
    u64     start ;
    u64     stop ;
    u64     time ;
    u64     count ;
    char            *name ;
} ;
#else
#define DEF_TIMER(name)
#define START_TIMER(t)
#define END_TIMER(t)
#define RESET_TIMER(t)
#define PRINT_TIMER(t)
#endif

#ifdef __VMKLNX__
#define __ALIGN_KERNEL_MASK(x, mask)    (((x) + (mask)) & ~(mask))
#define __ALIGN_KERNEL(x, a)            __ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __refdata
#undef ALIGN
#define ALIGN(x, a)                     __ALIGN_KERNEL((x), (a))
#define PTR_ALIGN(p, a)                 ((typeof(p))ALIGN((unsigned long)(p), (a)))
void get_page(struct page *page);
#endif

u32 bdx_mdio_get		 (struct bdx_priv *priv);
int bdx_mdio_write		 (struct bdx_priv *priv, int device, int port, u16 addr, u16 data);
int bdx_mdio_read		 (struct bdx_priv *priv, int device, int port, u16 addr);
int bdx_mdio_look_for_phy(struct bdx_priv *priv, int port);
int bdx_speed_set		 (struct bdx_priv *priv, u32 speed);
void bdx_speed_changed	 (struct bdx_priv *priv, u32 speed);

#define PHY_MDIO_READ(priv,device,addr) bdx_mdio_read(priv,(device),(priv)->phy_mdio_port,(addr))
#define PHY_MDIO_WRITE(priv,device,addr,data) bdx_mdio_write((priv),(device),(priv)->phy_mdio_port,    (addr), (data))

#define  BDX_MDIO_WRITE(priv, space, addr, val) \
    if (bdx_mdio_write((priv), (space), port, (addr), (val))) {     \
        ERR("bdx: failed to write  to phy at %x.%x val %x\n", \
                    (space),(addr),(val));                            \
        return 1;                                             \
    }

enum PHY_TYPE MV88X3310_register(struct bdx_priv *priv);
enum PHY_TYPE MV88X3120_register(struct bdx_priv *priv);
enum PHY_TYPE QT2025_register	(struct bdx_priv *priv);
enum PHY_TYPE CX4_register		(struct bdx_priv *priv);
enum PHY_TYPE TLK10232_register	(struct bdx_priv *priv);
enum PHY_TYPE AQR105_register	(struct bdx_priv *priv);

#ifndef vlan_tx_tag_present
#define vlan_tx_tag_present skb_vlan_tag_present
#endif
#ifndef vlan_tx_tag_get
#define vlan_tx_tag_get skb_vlan_tag_get
#endif

#endif /* _TN40XX_H */
