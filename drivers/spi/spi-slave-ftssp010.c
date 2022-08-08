/*
 * SPI interface for Faraday SSP010 SPI flash
 *
 * Copyright (c) 2019 Bing-Yao Luo <bjluo@faraday-tech.com>
 *
 * Copyright (c) 2006 Ben Dooks
 * Copyright (c) 2006 Simtec Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>

#include <asm/io.h>

/******************************************************************************
 * SSP010 definitions
 *****************************************************************************/
#define FTSSP010_OFFSET_CR0                 0x00
#define FTSSP010_OFFSET_CR1                 0x04
#define FTSSP010_OFFSET_CR2                 0x08
#define FTSSP010_OFFSET_STATUS              0x0c
#define FTSSP010_OFFSET_ICR                 0x10
#define FTSSP010_OFFSET_ISR                 0x14
#define FTSSP010_OFFSET_DATA                0x18

#define FTSSP010_OFFSET_REVISION            0x60
#define FTSSP010_OFFSET_FEATURE             0x64

/*
 * Control Register 0
 */
#define FTSSP010_CR0_SCLKPH                 BIT(0)	/* SPI SCLK phase */
#define FTSSP010_CR0_SCLKPO                 BIT(1)	/* SPI SCLK polarity */
#define FTSSP010_CR0_OPM                    GENMASK(3, 2)
#define FTSSP010_CR0_OPM_SLAVE_MONO         (0x0)
#define FTSSP010_CR0_OPM_SLAVE_STEREO       (0x1)
#define FTSSP010_CR0_OPM_MASTER_MONO        (0x2)
#define FTSSP010_CR0_OPM_MASTER_STEREO      (0x3)
#define FTSSP010_CR0_FSJSTFY                BIT(4)
#define FTSSP010_CR0_FSPO                   BIT(5)
#define FTSSP010_CR0_LSB                    BIT(6)
#define FTSSP010_CR0_LOOPBACK               BIT(7)
#define FTSSP010_CR0_FSDIST(x)              (((x) & 0x3) << 8)
#define FTSSP010_CR0_SPI_FLASH              BIT(11)	/* SPI application is Flash */
#define FTSSP010_CR0_FFMT_TI_SSP            (0x0 << 12)
#define FTSSP010_CR0_FFMT_SPI               (0x1 << 12)	/* Motorola  SPI */
#define FTSSP010_CR0_FFMT_MICROWIRE         (0x2 << 12)	/* NS  Microwire */
#define FTSSP010_CR0_FFMT_I2S               (0x3 << 12)	/* Philips   I2S */
#define FTSSP010_CR0_FFMT_ACLINK            (0x4 << 12)	/* Intel AC-Link */
#define FTSSP010_CR0_SPIFSPO                BIT(15)	/* f/s polarity for SPI */
#define FTSSP010_CR0_SPI_FLASHTX            BIT(18)	/* Flash Transmit Control */

/*
 * Control Register 1
 */
#define FTSSP010_CR1_SCLKDIV                GENMASK(15, 0)	/* SCLK divider */
#define FTSSP010_CR1_SDL                    GENMASK(22, 16)	/* serial  data length */
#define FTSSP010_CR1_PDL                    GENMASK(31, 24)	/* padding data length */

/*
 * Control Register 2
 */
#define FTSSP010_CR2_SSPEN                  BIT(0)	/* SSP enable */
#define FTSSP010_CR2_TXDOE                  BIT(1)	/* transmit data output enable */
#define FTSSP010_CR2_RXFCLR                 BIT(2)	/* receive  FIFO clear */
#define FTSSP010_CR2_TXFCLR                 BIT(3)	/* transmit FIFO clear */
#define FTSSP010_CR2_ACWRST                 BIT(4)	/* AC-Link warm reset enable */
#define FTSSP010_CR2_ACCRST                 BIT(5)	/* AC-Link cold reset enable */
#define FTSSP010_CR2_SSPRST                 BIT(6)	/* SSP reset */
#define FTSSP010_CR2_RXEN                   BIT(7)	/* Enable Rx */
#define FTSSP010_CR2_TXEN                   BIT(8)	/* Enable Tx */
#define FTSSP010_CR2_FS                     BIT(9)	/* CS line: 0 is low, 1 is high */
#define FTSSP010_CR2_CHIPSEL                GENMASK(11, 10)	/* Chip Select */

/*
 * Status Register
 */
#define FTSSP010_STATUS_RFF                 BIT(0)		/* receive FIFO full */
#define FTSSP010_STATUS_TFNF                BIT(1)		/* transmit FIFO not full */
#define FTSSP010_STATUS_BUSY                BIT(2)		/* bus is busy */
#define FTSSP010_STATUS_GET_RFVE            GENMASK(9, 4)	/* receive  FIFO valid entries */
#define FTSSP010_STATUS_GET_TFVE            GENMASK(17, 12)	/* transmit FIFO valid entries */

/*
 * Interrupt Control Register
 */
#define FTSSP010_ICR_RFOR                   BIT(0)		/* receive  FIFO overrun   interrupt */
#define FTSSP010_ICR_TFUR                   BIT(1)		/* transmit FIFO underrun  interrupt */
#define FTSSP010_ICR_RFTH                   BIT(2)		/* receive  FIFO threshold interrupt */
#define FTSSP010_ICR_TFTH                   BIT(3)		/* transmit FIFO threshold interrupt */
#define FTSSP010_ICR_RFDMA                  BIT(4)		/* receive  DMA request */
#define FTSSP010_ICR_TFDMA                  BIT(5)		/* transmit DMA request */
#define FTSSP010_ICR_AC97FCEN               BIT(6)		/* AC97 frame complete  */
#define FTSSP010_ICR_RFTHOD(x)              (((x) & 0xf) << 7)	/* receive  FIFO threshold */
#define FTSSP010_ICR_TFTHOD(x)              (((x) & 0xf) << 12)	/* transmit FIFO threshold */
#define FTSSP010_ICR_RFTHOD_UNIT            BIT(17)		/* receive FIFO threshold unit */
#define FTSSP010_ICR_TXCIEN                 BIT(18)		/* Transmit Data Complete */

/*
 * Interrupt Status Register
 */
#define FTSSP010_ISR_RFOR                   BIT(0)	/* receive  FIFO overrun  */
#define FTSSP010_ISR_TFUR                   BIT(1)	/* transmit FIFO underrun */
#define FTSSP010_ISR_RFTH                   BIT(2)	/* receive  FIFO threshold */
#define FTSSP010_ISR_TFTH                   BIT(3)	/* transmit FIFO threshold */
#define FTSSP010_ISR_TXCI                   BIT(5)	/* transmit data complete */

/*
 * Revision Register
 */
#define FTSSP010_REVISION_RELEASE(reg)      (((reg) >> 0) & 0xff)
#define FTSSP010_REVISION_MINOR(reg)        (((reg) >> 8) & 0xff)
#define FTSSP010_REVISION_MAJOR(reg)        (((reg) >> 16) & 0xff)

/*
 * Feature Register
 */
#define FTSSP010_FEATURE_WIDTH(reg)         (((reg) >>  0) & 0xff)
#define FTSSP010_FEATURE_RXFIFO_DEPTH(reg)  (((reg) >>  8) & 0xff)
#define FTSSP010_FEATURE_TXFIFO_DEPTH(reg)  (((reg) >> 16) & 0xff)
#define FTSSP010_FEATURE_I2S                BIT(25)
#define FTSSP010_FEATURE_SPI_MWR            BIT(26)
#define FTSSP010_FEATURE_SSP                BIT(27)

/******************************************************************************
 * spi_master (controller) priveate data
 *****************************************************************************/
struct ftssp010_dma {
	int                     dma_status;
	struct dma_chan         *dma_chan;
	struct dma_slave_config dma_cfg;
	wait_queue_head_t       dma_waitq;
};

struct ftssp010_spi {
	struct device           *dev;
	struct clk              *refclk;
	void __iomem            *base;
	int                     irq;
	int                     rxfifo_depth;
	int                     txfifo_depth;

	wait_queue_head_t       waitq;

	struct ftssp010_dma     tx_dma;
	struct ftssp010_dma     rx_dma;
};

#ifdef CONFIG_SPI_SLAVE_FTSSP010_USE_DMA
/******************************************************************************
 * DMA functions for FTSSP010
 *****************************************************************************/
#define DMA_COMPLETE            1
#define DMA_ONGOING             2

#define FTSSP010_DMA_BUF_SIZE   (4 * 1024)

u8 *tx_dum_buf;

static void ftssp010_slave_dma_callback(void *param)
{
	struct ftssp010_dma *dma = (struct ftssp010_dma *)param;

	BUG_ON(dma->dma_status == DMA_COMPLETE);
	dma->dma_status = DMA_COMPLETE;
	wake_up(&dma->dma_waitq);
}

static int ftssp010_slave_dma_wait(struct ftssp010_dma *dma)
{
	wait_event(dma->dma_waitq,
	           dma->dma_status == DMA_COMPLETE);

	return 0;
}

static int ftssp010_slave_dma_prepare(struct ftssp010_spi *ctrl, struct ftssp010_dma *dma,
				      resource_size_t phys_base, const char *name)
{
	struct dma_chan *dma_chan;

	dma_chan = dma_request_slave_channel(ctrl->dev, name);
	if (dma_chan) {
		dev_info(ctrl->dev, "Use DMA(%s) channel %d...\n",
			 dev_name(dma_chan->device->dev), dma_chan->chan_id);
	} else {
		dev_err(ctrl->dev, "allocate %s dma channel failed, fall back to PIO mode\n",
			name);
		return -EBUSY;
	}

	if (strcmp(name,"tx") == 0)
		dma->dma_cfg.direction = DMA_TO_DEVICE;
	else if (strcmp(name,"rx") == 0)
		dma->dma_cfg.direction = DMA_FROM_DEVICE;
	dma->dma_cfg.src_addr = (dma_addr_t)phys_base + FTSSP010_OFFSET_DATA;
	dma->dma_cfg.dst_addr = (dma_addr_t)phys_base + FTSSP010_OFFSET_DATA;
	dma->dma_cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	dma->dma_cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	dma->dma_cfg.src_maxburst = 1;
	dma->dma_cfg.dst_maxburst = 1;
	if (dmaengine_slave_config(dma_chan, &dma->dma_cfg)) {
		dev_err(ctrl->dev, "config %s dma slave failed, fall back to PIO mode\n",
			name);
		return -EBUSY;
	}

	dma->dma_chan = dma_chan;

	init_waitqueue_head(&dma->dma_waitq);

	return 0;
}
#endif

/******************************************************************************
 * internal functions for FTSSP010
 *****************************************************************************/
static void ftssp010_set_bits_per_word(void __iomem * base, int bpw)
{
	unsigned int cr1 = readl(base + FTSSP010_OFFSET_CR1);

	cr1 &= ~FTSSP010_CR1_SDL;
	cr1 |= FIELD_PREP(FTSSP010_CR1_SDL, (bpw - 1));

	writel(cr1, base + FTSSP010_OFFSET_CR1);
}

static void ftssp010_write_word(void __iomem * base, const void *data,
				   int wsize)
{
	unsigned int tmp = 0;

	if (data) {
		switch (wsize) {
		case 1:
			tmp = *(const u8 *)data;
			break;

		case 2:
			tmp = *(const u16 *)data;
			break;

		default:
			tmp = *(const u32 *)data;
			break;
		}
	}

	writel(tmp, base + FTSSP010_OFFSET_DATA);
}

static void ftssp010_read_word(void __iomem * base, void *buf, int wsize)
{
	unsigned int data = readl(base + FTSSP010_OFFSET_DATA);

	if (buf) {
		switch (wsize) {
		case 1:
			*(u8 *) buf = data;
			break;

		case 2:
			*(u16 *) buf = data;
			break;

		default:
			*(u32 *) buf = data;
			break;
		}
	}
}

static unsigned int ftssp010_read_status(void __iomem * base)
{
	return readl(base + FTSSP010_OFFSET_STATUS);
}

static int ftssp010_txfifo_not_full(void __iomem * base)
{
	return ftssp010_read_status(base) & FTSSP010_STATUS_TFNF;
}

static int ftssp010_rxfifo_valid_entries(void __iomem * base)
{
	return FIELD_GET(FTSSP010_STATUS_GET_RFVE, ftssp010_read_status(base));
}

static unsigned int ftssp010_read_feature(void __iomem *base)
{
	return readl(base + FTSSP010_OFFSET_FEATURE);
}

static int ftssp010_rxfifo_depth(void __iomem *base)
{
	return  FTSSP010_FEATURE_RXFIFO_DEPTH(ftssp010_read_feature(base)) + 1;
}

static int ftssp010_txfifo_depth(void __iomem *base)
{
	return  FTSSP010_FEATURE_TXFIFO_DEPTH(ftssp010_read_feature(base)) + 1;
}

/******************************************************************************
 * workqueue
 *****************************************************************************/
#ifdef CONFIG_SPI_SLAVE_FTSSP010_USE_DMA
static int _ftssp010_spi_slave_fill_fifo_dma(struct ftssp010_spi *ctrl,
					     struct spi_transfer *t,
					     int wsize)
{
	struct dma_async_tx_descriptor *tx_desc, *rx_desc;
	struct scatterlist tx_sg, rx_sg;
	dma_addr_t tx_dma_addr, rx_dma_addr;
	u32 len = t->len;
	const void *tx_buf = t->tx_buf;
	void *rx_buf = t->rx_buf;
	u32 cr2, mask;

	mask = FTSSP010_CR2_TXDOE | FTSSP010_CR2_TXEN;

	if (rx_buf)
		mask |= FTSSP010_CR2_RXEN;

	cr2 = readl(ctrl->base + FTSSP010_OFFSET_CR2);

	writel((cr2 | mask), ctrl->base + FTSSP010_OFFSET_CR2);

	// setup dma tx descriptor
	if (tx_buf) {
		tx_dma_addr = dma_map_single(ctrl->dev, (void *)tx_buf, len,
					     DMA_TO_DEVICE);
		sg_init_one(&tx_sg, (void *)tx_buf, len);
	} else {
		tx_dma_addr = dma_map_single(ctrl->dev, (void *)tx_dum_buf, len,
					     DMA_TO_DEVICE);
		sg_init_one(&tx_sg, (void *)tx_dum_buf, len);
	}

	if(dma_mapping_error(ctrl->dev, tx_dma_addr)) {
		dev_err(ctrl->dev, "dma_map_single(w) failed\n");
		dma_unmap_single(ctrl->dev, tx_dma_addr, len, DMA_TO_DEVICE);
		goto out;
	}

	sg_dma_address(&tx_sg) = tx_dma_addr;

	tx_desc = dmaengine_prep_slave_sg(ctrl->tx_dma.dma_chan, &tx_sg, 1,
	                                  DMA_TO_DEVICE,
					  (DMA_PREP_INTERRUPT | DMA_CTRL_ACK));
	if (!tx_desc) {
		dma_unmap_single(ctrl->dev, tx_dma_addr, len, DMA_TO_DEVICE);
		dev_err(ctrl->dev, "prepare_slave_sg(w) failed\n");
		goto out;
	}

	tx_desc->callback = ftssp010_slave_dma_callback;
	tx_desc->callback_param = &ctrl->tx_dma;

	// setup dma rx descriptor
	if (rx_buf) {
		rx_dma_addr = dma_map_single(ctrl->dev, (void *)rx_buf, len,
					     DMA_FROM_DEVICE);
		if(dma_mapping_error(ctrl->dev, rx_dma_addr)) {
			dev_err(ctrl->dev, "dma_map_single(r) failed\n");
			dma_unmap_single(ctrl->dev, rx_dma_addr, len, DMA_FROM_DEVICE);
			goto out;
		}

		sg_init_one(&rx_sg, (void *)rx_buf, len);
		sg_dma_address(&rx_sg) = rx_dma_addr;

		rx_desc = dmaengine_prep_slave_sg(ctrl->rx_dma.dma_chan, &rx_sg, 1,
		                                  DMA_FROM_DEVICE,
						  (DMA_PREP_INTERRUPT | DMA_CTRL_ACK));
		if (rx_desc == NULL) {
			dma_unmap_single(ctrl->dev, rx_dma_addr, len, DMA_FROM_DEVICE);
			dev_err(ctrl->dev, "prep_slave_sg(r) failed\n");
			goto out;
		}

		rx_desc->callback = ftssp010_slave_dma_callback;
		rx_desc->callback_param = &ctrl->rx_dma;

		// start dma rx channel
		ctrl->rx_dma.dma_status = DMA_ONGOING;
		dmaengine_submit(rx_desc);
		dma_async_issue_pending(ctrl->rx_dma.dma_chan);
	}

	// start dma tx channel
	ctrl->tx_dma.dma_status = DMA_ONGOING;
	dmaengine_submit(tx_desc);
	dma_async_issue_pending(ctrl->tx_dma.dma_chan);

	// wait for completion of dma channel
	if (rx_buf) {
		ftssp010_slave_dma_wait(&ctrl->rx_dma);
		dma_unmap_single(ctrl->dev, rx_dma_addr, len, DMA_FROM_DEVICE);
	}

	ftssp010_slave_dma_wait(&ctrl->tx_dma);
	dma_unmap_single(ctrl->dev, tx_dma_addr, len, DMA_TO_DEVICE);
	len = 0;
out:
	writel((cr2 & ~mask), ctrl->base + FTSSP010_OFFSET_CR2);

	cr2 |= (FTSSP010_CR2_TXFCLR | FTSSP010_CR2_RXFCLR);
	writel(cr2, ctrl->base + FTSSP010_OFFSET_CR2);

	return t->len - len;
}
#endif

static int _ftssp010_spi_slave_fill_fifo(struct ftssp010_spi *ctrl,
				   struct spi_transfer *t,
				   int wsize)
{
	int len = t->len;
	const void *tx_buf = t->tx_buf;
	void *rx_buf = t->rx_buf;
	int cr2, mask, size, i;
	unsigned long timeout;

	mask = FTSSP010_CR2_TXEN;

	if (tx_buf)
		mask |= FTSSP010_CR2_TXDOE;

	if (rx_buf)
		mask |= FTSSP010_CR2_RXEN;

	cr2 = readl(ctrl->base + FTSSP010_OFFSET_CR2);

	writel((cr2 | mask), ctrl->base + FTSSP010_OFFSET_CR2);

	while (len > 0) {
		if (len < wsize)
			wsize = 1;

		size = min_t(u32, min_t(u32, ctrl->txfifo_depth, ctrl->rxfifo_depth),
					len / wsize);

		for (i = 0; i < size; ++i) {
			ftssp010_write_word(ctrl->base, tx_buf, wsize);

			if (tx_buf)
				tx_buf += wsize;
		}

		timeout = jiffies + 1000;
		do {
			unsigned int sts;

			if (time_after(jiffies, timeout)) {
				dev_info(ctrl->dev, "xfer error: len %d, " \
					 "tx_buf %px rx_buf %px\n",
					 t->len, t->tx_buf, t->rx_buf);
				len = t->len;
				goto out;
			}

			sts = ftssp010_read_status(ctrl->base);

			if (FIELD_GET(FTSSP010_STATUS_BUSY, sts) == 0) {
				if (tx_buf) {
					if (FIELD_GET(FTSSP010_STATUS_GET_TFVE, sts) == 0)
						break;
				} else {
					break;
				}
			}
		} while (1);

		len -= (size * wsize);

		if (rx_buf) {

			/* wait until sth. in rx fifo */
			for (i = 0; i < size; ++i) {
				while (ftssp010_rxfifo_valid_entries(ctrl->base) == 0) {
					usleep_range(1, 5);
				}

				ftssp010_read_word(ctrl->base, rx_buf, wsize);

				rx_buf += wsize;
			}
		}
	}
out:
	writel((cr2 & ~mask), ctrl->base + FTSSP010_OFFSET_CR2);

	cr2 |= (FTSSP010_CR2_TXFCLR | FTSSP010_CR2_RXFCLR);
	writel(cr2, ctrl->base + FTSSP010_OFFSET_CR2);

	return t->len - len;
}

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
static irqreturn_t ftssp010_spi_interrupt(int irq, void *dev_id)
{
	struct spi_master *master = dev_id;
	struct ftssp010_spi *ctrl;
	u32 isr;

	ctrl = spi_master_get_devdata(master);

	isr = readl(ctrl->base + FTSSP010_OFFSET_ISR);

	if (isr & FTSSP010_ISR_TFUR)
		dev_err(&master->dev, "Tx FIFO Underrun!\n");

	if (isr & FTSSP010_ISR_RFOR)
		dev_err(&master->dev, "Rx FIFO overrun!\n");

	return IRQ_HANDLED;
}

/******************************************************************************
 * struct spi_master functions
 *****************************************************************************/

/* the spi->mode bits understood by this driver: */
#define MODEBITS (SPI_CPOL | SPI_CPHA | SPI_LSB_FIRST)

static void ftssp010_spi_slave_setup_mode(struct spi_device *spi)
{
	struct ftssp010_spi *ctrl = spi_master_get_devdata(spi->master);
	unsigned int cr0;

	cr0 = FTSSP010_CR0_FFMT_SPI | FTSSP010_CR0_FSPO |
	      FIELD_PREP(FTSSP010_CR0_OPM, FTSSP010_CR0_OPM_SLAVE_STEREO);
#ifdef CONFIG_SPI_SLAVE_FTSSP010_FLASH
	cr0 |= FTSSP010_CR0_SPI_FLASH | FTSSP010_CR0_SPI_FLASHTX;
#endif
	if (spi->mode & SPI_CPOL) {
		dev_dbg(&spi->master->dev, "setup: CPOL high\n");
		cr0 |= FTSSP010_CR0_SCLKPO;
	}

	if (spi->mode & SPI_CPHA) {
		dev_dbg(&spi->master->dev, "setup: CPHA high\n");
		cr0 |= FTSSP010_CR0_SCLKPH;
	}

	if (spi->mode & SPI_LSB_FIRST) {
		dev_dbg(&spi->master->dev, "setup: LSB first\n");
		cr0 |= FTSSP010_CR0_LSB;
	}

	writel(cr0, ctrl->base + FTSSP010_OFFSET_CR0);
}

static void ftssp010_spi_slave_setup_clkdiv(struct spi_device *spi,
				   struct spi_transfer *t)
{
	struct ftssp010_spi *ctrl;
	u32 clk_hz, div = 0;
	unsigned int cr1;

	ctrl = spi_master_get_devdata(spi->master);

	clk_hz = clk_get_rate(ctrl->refclk);

	while (div < 0xFFFF) {

		if ((clk_hz / (2 * (div + 1))) <= t->speed_hz)
			break;

		div++;
	}

	cr1 = readl(ctrl->base + FTSSP010_OFFSET_CR1);
	cr1 &= ~FTSSP010_CR1_SCLKDIV;
	cr1 |= FIELD_PREP(FTSSP010_CR1_SCLKDIV, div);
	writel(cr1, ctrl->base + FTSSP010_OFFSET_CR1);
}

static int ftssp010_spi_slave_setup(struct spi_device *spi)
{
	struct ftssp010_spi *ctrl;

	ctrl = spi_master_get_devdata(spi->master);

	if (spi->mode & ~MODEBITS) {
		dev_err(&spi->master->dev,
		        "setup: unsupported mode bits %x\n", spi->mode & ~MODEBITS);
		return -EINVAL;
	}

	if (spi->chip_select > spi->master->num_chipselect) {
		dev_err(&spi->dev,
		        "setup: invalid chipselect %u (%u defined)\n",
		        spi->chip_select, spi->master->num_chipselect);
		return -EINVAL;
	}

	/* check speed */
	if (!spi->max_speed_hz) {
		dev_err(&spi->dev, "setup: max speed not specified\n");
		return -EINVAL;
	}

	if (spi->max_speed_hz > spi->master->max_speed_hz) {
		dev_err(&spi->dev,
		        "setup: invalid max speed hz %u (%u defined)\n",
		        spi->max_speed_hz, spi->master->max_speed_hz);
		return -EINVAL;
	}

	ftssp010_spi_slave_setup_mode(spi);

	ftssp010_set_bits_per_word(ctrl->base, spi->bits_per_word);

	dev_dbg(&spi->dev, "%s: CR0=0x%08x, CR1=0x%08x\n", __func__,
	        readl(ctrl->base + FTSSP010_OFFSET_CR0),
	        readl(ctrl->base + FTSSP010_OFFSET_CR1));

	return 0;
}

/**
 * ftssp010_spi_slave_transfer_one - Initiates the SPI transfer
 * @master:	Pointer to the spi_master structure which provides
 *		information about the controller.
 * @qspi:	Pointer to the spi_device structure
 * @transfer:	Pointer to the spi_transfer structure which provide information
 *		about next transfer parameters
 *
 * This function fills the TX FIFO, starts the transfer, and waits for the
 * transfer to be completed.
 *
 * Return:	Number of bytes transferred in the last transfer
 */
static int ftssp010_spi_slave_transfer_one(struct spi_master *master,
					   struct spi_device *spi,
					   struct spi_transfer *transfer)
{
	struct ftssp010_spi *ctrl = spi_master_get_devdata(master);
	int ret_len;

	dev_dbg(&spi->dev, "%s: len %d, tx_buf %p rx_buf %p\n",
	        __func__, transfer->len, transfer->tx_buf, transfer->rx_buf);

	ftssp010_spi_slave_setup_clkdiv(spi, transfer);

	dev_dbg(&spi->dev, "%s: CR0=0x%08x, CR1=0x%08x,  CR2=0x%08x\n", __func__,
	        readl(ctrl->base + FTSSP010_OFFSET_CR0),
	        readl(ctrl->base + FTSSP010_OFFSET_CR1),
		readl(ctrl->base + FTSSP010_OFFSET_CR2));
#ifdef CONFIG_SPI_SLAVE_FTSSP010_USE_DMA
	if (ctrl->tx_dma.dma_chan && ctrl->rx_dma.dma_chan)
		ret_len = _ftssp010_spi_slave_fill_fifo_dma(ctrl, transfer, 1);
	else
#endif
		ret_len = _ftssp010_spi_slave_fill_fifo(ctrl, transfer, 1);
#if 0
{
	int i, len = transfer->len;
	char *rx_buf = NULL, *tx_buf = NULL;

	if (transfer->tx_buf)
		tx_buf = (char *) transfer->tx_buf;

	if (transfer->rx_buf)
		rx_buf = (char *) transfer->rx_buf;

	dev_info(&spi->dev, "%s: len %d, buffer content:\n", __func__, ret_len);
	for (i = 0; i < len; i++) {

		if (tx_buf)
			printk("tx:%x ", tx_buf[i]);

		if (rx_buf)
			printk("rx:%x ", rx_buf[i]);
	}
}
#endif
	spi_finalize_current_transfer(master);

	return ret_len;
}

/**
 * ftssp010_spi_prepare_transfer_hardware - Prepares hardware for transfer.
 * @master:	Pointer to the spi_master structure which provides
 *		information about the controller.
 *
 * This function enables SPI master controller.
 *
 * Return:	Always 0
 */
static int ftssp010_spi_prepare_transfer_hardware(struct spi_master *master)
{
	struct ftssp010_spi *ctrl = spi_master_get_devdata(master);

	writel(FTSSP010_CR2_TXFCLR | FTSSP010_CR2_RXFCLR | FTSSP010_CR2_SSPEN,
	       ctrl->base + FTSSP010_OFFSET_CR2);

	dev_dbg(&master->dev, "%s: CR2=0x%08x\n", __func__,
	        readl(ctrl->base + FTSSP010_OFFSET_CR2));
	return 0;
}

static int ftssp010_hw_setup(struct ftssp010_spi *ctrl, resource_size_t phys_base)
{
#ifdef CONFIG_SPI_SLAVE_FTSSP010_USE_DMA
	uint32_t val;
#endif

	writel(FTSSP010_ICR_RFOR | FTSSP010_ICR_TFUR | 
	       FTSSP010_ICR_TXCIEN,
	       ctrl->base + FTSSP010_OFFSET_ICR);

#ifdef CONFIG_SPI_SLAVE_FTSSP010_USE_DMA
	tx_dum_buf = kzalloc(FTSSP010_DMA_BUF_SIZE, GFP_KERNEL);
	if (!tx_dum_buf) {
		dev_err(ctrl->dev, "allocate dma buffer failed, fall back to PIO mode\n");
		return 0;
	}

	ftssp010_slave_dma_prepare(ctrl, &ctrl->tx_dma, phys_base, "tx");

	ftssp010_slave_dma_prepare(ctrl, &ctrl->rx_dma, phys_base, "rx");

	val = readl(ctrl->base + FTSSP010_OFFSET_ICR);
	val |= FTSSP010_ICR_TFTHOD(1) | FTSSP010_ICR_RFTHOD(1);
	val |= FTSSP010_ICR_TFDMA | FTSSP010_ICR_RFDMA;
	writel(val, ctrl->base + FTSSP010_OFFSET_ICR);
#endif

	return 0;
}

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftssp010_spi_slave_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	struct ftssp010_spi *priv;
	struct resource *res;
	int ret = 0;
	u32 dev_id, num_cs, clk_hz;

	dev_info(&pdev->dev, "Faraday FTSSP010 Slave driver\n");

	if ((master =
	     spi_alloc_slave(&pdev->dev, sizeof *priv)) == NULL) {
		return -ENOMEM;
	}

	priv = spi_master_get_devdata(master);
	master->dev.of_node = pdev->dev.of_node;
	platform_set_drvdata(pdev, master);

	priv->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->base)) {
		ret = PTR_ERR(priv->base);
		goto err_put_master;
	}

	priv->refclk = devm_clk_get(&pdev->dev, "sspclk");
	if (IS_ERR(priv->refclk)) {
		dev_err(&pdev->dev, "ref_clk clock not found.\n");
		ret = PTR_ERR(priv->refclk);
		goto err_put_master;
	}

	ret = clk_prepare_enable(priv->refclk);
	if (ret) {
		dev_err(&pdev->dev, "Unable to enable device clock.\n");
		goto err_put_master;
	}

	clk_hz = clk_get_rate(priv->refclk);

	//Tx at least divide by 2, SCLKDIV value is 0
	//Rx at least divide by 6, SCLKDIV value is 2
	master->max_speed_hz = clk_hz / 3;

	/* Initialize the hardware */
	priv->rxfifo_depth = ftssp010_rxfifo_depth(priv->base);
	priv->txfifo_depth = ftssp010_txfifo_depth(priv->base);

	if ((priv->irq = platform_get_irq(pdev, 0)) < 0) {
		ret = -ENXIO;
		dev_err(&pdev->dev, "irq resource not found\n");
		goto err_clk_disable;
	}

	if ((ret = devm_request_irq(&pdev->dev, priv->irq, ftssp010_spi_interrupt, 0,
	                            pdev->name, master)) != 0) {
		ret = -ENXIO;
		dev_err(&pdev->dev, "request_irq failed\n");
		goto err_clk_disable;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "num-cs",
	                           &num_cs);
	if (ret < 0)
		master->num_chipselect = 1;
	else
		master->num_chipselect = num_cs;

	dev_info(&pdev->dev, "Faraday FTSSP010 Controller HW version 0x%08x\n",
	         readl(priv->base + FTSSP010_OFFSET_REVISION));

	dev_info(&pdev->dev, "Controller base address 0x%08x (irq %d)\n",
	         (unsigned int)res->start, priv->irq);

	dev_info(&pdev->dev, "Controller max speed Hz %u, num_cs %d\n",
	         master->max_speed_hz, master->num_chipselect);

	ret = of_property_read_u32(pdev->dev.of_node, "dev_id",
	                           &dev_id);
	if (ret < 0)
		master->bus_num = pdev->id;
	else
		master->bus_num = dev_id;

	master->setup = ftssp010_spi_slave_setup;
	master->transfer_one = ftssp010_spi_slave_transfer_one;
	master->prepare_transfer_hardware = ftssp010_spi_prepare_transfer_hardware;

	master->mode_bits = MODEBITS;
	master->bits_per_word_mask = SPI_BPW_MASK(8);

	init_waitqueue_head(&priv->waitq);

	ret = ftssp010_hw_setup(priv, res->start);
	if (ret < 0) {
		dev_err(&pdev->dev, "setup hardware failed\n");
		goto err_clk_disable;
	}

	if ((ret = devm_spi_register_master(&pdev->dev, master)) != 0) {
		dev_err(&pdev->dev, "register master failed\n");
		goto err_clk_disable;
	}

	return 0;

err_clk_disable:
	clk_disable_unprepare(priv->refclk);
err_put_master:
	spi_master_put(master);
	return ret;
}

static int __exit ftssp010_spi_slave_remove(struct platform_device *pdev)
{
	struct spi_master *master;

	master = platform_get_drvdata(pdev);
	spi_unregister_master(master);

	dev_info(&pdev->dev, "FTSSP010 slave unregistered\n");

	return 0;
}

#define ftssp010_spi_slave_suspend NULL
#define ftssp010_spi_slave_resume NULL
#define ftssp010_spi_slave_runtime_suspend NULL
#define ftssp010_spi_slave_runtime_resume NULL
#define ftssp010_spi_slave_runtime_idle NULL

static const struct dev_pm_ops ftssp010_spi_slave_ops = {
#ifdef CONFIG_PM_SLEEP
	.suspend    = ftssp010_spi_slave_suspend,
	.resume     = ftssp010_spi_slave_resume,
#endif
	SET_RUNTIME_PM_OPS(ftssp010_spi_slave_runtime_suspend, ftssp010_spi_slave_runtime_resume,
	                   ftssp010_spi_slave_runtime_idle)
};

#ifdef CONFIG_OF
static const struct of_device_id ftssp010_slave_of_match[] = {
	{ .compatible = "faraday,ftssp010-spi-slave" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ftssp010_slave_of_match);
#endif

static struct platform_driver ftssp010_spi_slave_driver = {
	.remove     = __exit_p(ftssp010_spi_slave_remove),
	.driver     = {
		.name   = "ftssp010_spi_slave",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(ftssp010_slave_of_match),
		.pm     = &ftssp010_spi_slave_ops,
	},
};

module_platform_driver_probe(ftssp010_spi_slave_driver, ftssp010_spi_slave_probe);

MODULE_AUTHOR("Bing-Yao Luo <bjluo@faraday-tech.com>");
MODULE_DESCRIPTION("FTSSP010 SPI Flash Driver");
MODULE_LICENSE("GPL");
