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
#ifndef _TN40XX_IOCTL_H
#define _TN40XX_IOCTL_H

/* ioctl ops */

enum
{
    OP_INFO,
	OP_READ_REG,
	OP_WRITE_REG,
	OP_MDIO_READ,
	OP_MDIO_WRITE,
	op_TRACE_ON,
	op_TRACE_OFF,
	op_TRACE_ONCE,
	op_TRACE_PRINT,
	OP_DBG,
	OP_MEMLOG_DMESG,
	OP_MEMLOG_PRINT,
};
enum
{
	DBG_NONE    = 0,
	DBG_SUSPEND = 1,
	DBG_RESUME  = 2,
};
#define IOCTL_DATA_SIZE		(3)

typedef struct _tn40_ioctl_
{
	uint 	data[IOCTL_DATA_SIZE];
	char	*buf;
} tn40_ioctl_t;

#endif /* _TN40XX_IOCTL_H */
