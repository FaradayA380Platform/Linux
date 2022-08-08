#ifndef __FTAES020_REGS_H__
#define __FTAES020_REGS_H__

#define FTAAES020_CR		0x00
#define FTAES020_CR_DECRYPT		(1 << 0)
#define FTAES020_CR_ALG_DES		(0 << 1)
#define FTAES020_CR_ALG_DES3		(1 << 1)
#define FTAES020_CR_ALG_AES_128		(4 << 1)
#define FTAES020_CR_ALG_AES_192		(5 << 1)
#define FTAES020_CR_ALG_AES_256		(6 << 1)
#define FTAES020_CR_MODE_ECB		(0 << 4)
#define FTAES020_CR_MODE_CBC		(1 << 4)
#define FTAES020_CR_MODE_CTR		(2 << 4)
#define FTAES020_CR_MODE_CFB		(4 << 4)
#define FTAES020_CR_MODE_OFB		(5 << 4)
#define FTAES020_CR_LOAD_IV		(1 << 7)
#define FTAES020_CR_PARITY_CHECK	(1 << 8)

#define	FTAES020_FIFO_STS	0x08
#define FTAES020_PARITY_ERR	0x0C

#define FTAES020_DMA_SRC	0x48
#define FTAES020_DMA_DST	0x4C
#define FTAES020_DMA_SIZE	0x50
#define FTAES020_DMA_CTRL	0x54
#define FTAES020_DMA_CTRL_EN		(1 << 0)
#define FTAES020_DMA_CTRL_STOP		(1 << 2)

#define FTAES020_FIFO_THRES	0x58
#define FTAES020_FIFO_THRES_IN(x)	((x & 0xFF) << 0)
#define FTAES020_FIFO_THRES_OUT(x)	((x & 0XFF) << 8)

#define	FTAES020_IER		0x5C
#define	FTAES020_ISR		0x60
#define	FTAES020_IMR		0x64
#define	FTAES020_ICR		0x68
#define	FTAES020_INTR_DMA_STOP		(1 << 2)
#define	FTAES020_INTR_DMA_ERR		(1 << 1)
#define	FTAES020_INTR_DMA_DONE		(1 << 0)

/* Key Registers from 0x10 ~ 0x2F (6 words) */
#define FTAES020_KEYWR(x)	(0x10 + ((x) * 0x04))
/* Initialization Vector Registers from 0x30 ~ 0x3F */
#define FTAES020_IVR(x)		(0x30 + ((x) * 0x04))

#define FTAES020_HW_VERSION	0x70

#define FTAES020_FEA		0x74
#define FTAES020_FEA_FIFO_DEPTH_MASK	(0xFF << 0)

#define FTAES020_LAST_IV0	0x80
#define FTAES020_LAST_IV1	0x84
#define FTAES020_LAST_IV2	0x88
#define FTAES020_LAST_IV3	0x8C

#endif /* __FTAES020_REGS_H__ */
