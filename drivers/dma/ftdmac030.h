/*
 *  arch/arm/mach-faraday/include/mach/ftdmac030.h
 *
 *  Faraday FTDMAC030 DMA controller
 *
 *  Copyright (C) 2011 Faraday Technology
 *  Copyright (C) 2011 Po-Yu Chuang <ratbert@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __FTDMAC030_H
#define __FTDMAC030_H

#include <linux/dma/ftdmac030.h>

#define FTDMAC030_OFFSET_ISR		0x0
#define FTDMAC030_OFFSET_TCISR		0x4
#define FTDMAC030_OFFSET_TCICR		0x8
#define FTDMAC030_OFFSET_EAISR		0xc
#define FTDMAC030_OFFSET_EAICR		0x10
#define FTDMAC030_OFFSET_TCRAW		0x14
#define FTDMAC030_OFFSET_EARAW		0x18
#define FTDMAC030_OFFSET_CH_ENABLED	0x1c
#define FTDMAC030_OFFSET_SYNC		0x20
#define FTDMAC030_OFFSET_LDM		0x24
#define FTDMAC030_OFFSET_WDT		0x28
#define FTDMAC030_OFFSET_GE		0x2c
#define FTDMAC030_OFFSET_PSE		0x30
#define FTDMAC030_OFFSET_REVISION	0x34
#define FTDMAC030_OFFSET_FEATURE	0x38
#define FTDMAC030_OFFSET_LDMFFS0	0x3c
#define FTDMAC030_OFFSET_LDMFFS1	0x40
#define FTDMAC030_OFFSET_LDMFFS2	0x44
#define FTDMAC030_OFFSET_LDMFFS3	0x48
#define FTDMAC030_ENDIAN_CONVERSION	0x4c
#define FTDMAC030_WRITE_ONLY_MODE_CONSTANT_VALUE	0x50

#define FTDMAC030_OFFSET_CTRL_CH(x)	(0x100 + (x) * 0x20)
#define FTDMAC030_OFFSET_CFG_CH(x)	(0x104 + (x) * 0x20)
#define FTDMAC030_OFFSET_SRC_CH(x)	(0x108 + (x) * 0x20)
#define FTDMAC030_OFFSET_DST_CH(x)	(0x10c + (x) * 0x20)
#define FTDMAC030_OFFSET_LLP_CH(x)	(0x110 + (x) * 0x20)
#define FTDMAC030_OFFSET_CYC_CH(x)	(0x114 + (x) * 0x20)
#define FTDMAC030_OFFSET_STRIDE_CH(x)	(0x118 + (x) * 0x20)

/*
 * Error/abort interrupt status/clear register
 * Error/abort status register
 */
#define FTDMAC030_EA_ERR_CH(x)		(1 << (x))
#define FTDMAC030_EA_WDT_CH(x)		(1 << ((x) + 8))
#define FTDMAC030_EA_ABT_CH(x)		(1 << ((x) + 16))

/*
 * Control register
 */
#define FTDMAC030_CTRL_WE(x)		((0x1 << (x)) & 0xff)
#define FTDMAC030_CTRL_WSYNC		(0x1 << 8)
#define FTDMAC030_CTRL_SE(x)		(((x) & 0x7) << 9)
#define FTDMAC030_CTRL_SE_ENABLE	(0x1 << 12)
#define FTDMAC030_CTRL_WE_ENABLE	(0x1 << 13)
#define FTDMAC030_CTRL_2D		(0x1 << 14)
#define FTDMAC030_CTRL_EXP		(0x1 << 15)
#define FTDMAC030_CTRL_ENABLE		(0x1 << 16)
#define FTDMAC030_CTRL_WDT_ENABLE	(0x1 << 17)
#define FTDMAC030_CTRL_DST_INC		(0x0 << 18)
#define FTDMAC030_CTRL_DST_FIXED	(0x2 << 18)
#define FTDMAC030_CTRL_SRC_INC		(0x0 << 20)
#define FTDMAC030_CTRL_SRC_FIXED	(0x2 << 20)
#define FTDMAC030_CTRL_DST_WIDTH_8	(0x0 << 22)
#define FTDMAC030_CTRL_DST_WIDTH_16	(0x1 << 22)
#define FTDMAC030_CTRL_DST_WIDTH_32	(0x2 << 22)
#define FTDMAC030_CTRL_DST_WIDTH_64	(0x3 << 22)
#define FTDMAC030_CTRL_SRC_WIDTH_8	(0x0 << 25)
#define FTDMAC030_CTRL_SRC_WIDTH_16	(0x1 << 25)
#define FTDMAC030_CTRL_SRC_WIDTH_32	(0x2 << 25)
#define FTDMAC030_CTRL_SRC_WIDTH_64	(0x3 << 25)
#define FTDMAC030_CTRL_MASK_TC		(0x1 << 28)
#define FTDMAC030_CTRL_1BEAT		(0x0 << 29)
#define FTDMAC030_CTRL_2BEATS		(0x1 << 29)
#define FTDMAC030_CTRL_4BEATS		(0x2 << 29)
#define FTDMAC030_CTRL_8BEATS		(0x3 << 29)
#define FTDMAC030_CTRL_16BEATS		(0x4 << 29)
#define FTDMAC030_CTRL_32BEATS		(0x5 << 29)
#define FTDMAC030_CTRL_64BEATS		(0x6 << 29)
#define FTDMAC030_CTRL_128BEATS		(0x7 << 29)

/*
 * Configuration register
 */
#define FTDMAC030_CFG_MASK_TCI		(1 << 0)	/* mask tc interrupt */
#define FTDMAC030_CFG_MASK_EI		(1 << 1)	/* mask error interrupt */
#define FTDMAC030_CFG_MASK_AI		(1 << 2)	/* mask abort interrupt */
#define FTDMAC030_CFG_SRC_HANDSHAKE(x)	(((x) & 0xf) << 3)
#define FTDMAC030_CFG_SRC_HANDSHAKE_EN	(1 << 7)
#define FTDMAC030_CFG_DST_HANDSHAKE(x)	(((x) & 0xf) << 9)
#define FTDMAC030_CFG_DST_HANDSHAKE_EN	(1 << 13)
#define FTDMAC030_CFG_LLP_CNT(cfg)	(((cfg) >> 16) & 0xf)
#define FTDMAC030_CFG_GW(x)		(((x) & 0xff) << 20)
#define FTDMAC030_CFG_HIGH_PRIO		(1 << 28)
#define FTDMAC030_CFG_WRITE_ONLY_MODE	(1 << 30)
#define FTDMAC030_CFG_UNALIGNMODE	(1 << 31)

/*
 * Transfer size register
 */
#define FTDMAC030_CYC_MASK		0x3fffff
#define FTDMAC030_CYC_TOTAL(x)		((x) & FTDMAC030_CYC_MASK)
#define FTDMAC030_CYC_2D(x, y)		(((x) & 0xffff) | (((y) & 0xffff) << 16))

/*
 * Stride register
 */
#define FTDMAC030_STRIDE_SRC(x)		((x) & 0xffff)
#define FTDMAC030_STRIDE_DST(x)		(((x) & 0xffff) << 16)

/**
 * struct ftdmac030_lld - hardware link list descriptor.
 * @src: source physical address
 * @dst: destination physical addr
 * @next: phsical address to the next link list descriptor
 * @ctrl: control field
 * @cycle: transfer size
 * @stride: stride for 2D mode
 *
 * should be 32 or 64 bits aligned depends on AXI configuration
 */
struct ftdmac030_lld {
	dma_addr_t src:32;
	dma_addr_t dst:32;
	dma_addr_t next:32;
	unsigned int ctrl;
	unsigned int cycle;
	unsigned int stride;
};

#endif	/* __FTDMAC030_H */
