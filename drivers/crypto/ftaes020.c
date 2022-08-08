/*
 * Cryptographic API.
 *
 * Support for Faraday FTAES020 AES and DES/DES3 HW acceleration.
 *
 * Copyright (c) 2017 Faraday Tech
 * Author: Bing-Yao, Luo <bjluo@farady-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Some ideas are from omap-aes.c, atmel-aes.c and atmel-tdes.c driver.
 */


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/hw_random.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#include <linux/device.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/crypto.h>
#include <linux/cryptohash.h>
#include <crypto/scatterwalk.h>
#include <crypto/algapi.h>
#include <crypto/aes.h>
#include <crypto/des.h>
#include "ftaes020.h"

#if 0
#define debug	printk
#else
#define debug	pr_debug
#endif

#define CFB_BLOCK_SIZE		1

/* AES flags */
#define FTAES020_FLAGS_MODE_MASK	0x03ff
#define FTAES020_FLAGS_DECRYPT		BIT(0)
#define FTAES020_ALG_FLAGS_AES		BIT(1)
#define FTAES020_ALG_FLAGS_DES		BIT(2)
#define FTAES020_MODE_FLAGS_CBC		BIT(3)
#define FTAES020_MODE_FLAGS_CFB		BIT(4)
#define FTAES020_MODE_FLAGS_OFB		BIT(5)
#define FTAES020_MODE_FLAGS_CTR		BIT(6)

#define FTAES020_FLAGS_INIT		BIT(16)
#define FTAES020_FLAGS_BUSY		BIT(17)
#define FTAES020_FLAGS_COPY		BIT(18)

#define ATMEL_AES_QUEUE_LENGTH	50

struct ftaes020_dev;

struct ftaes020_ctx {
	struct ftaes020_dev *dd;

	int		keylen;
	u32		key[AES_KEYSIZE_256 / sizeof(u32)];

	u16		block_size;
};

struct ftaes020_reqctx {
	unsigned long mode;
};

struct ftaes020_dev {
	struct list_head	list;
	unsigned long		phys_base;
	void __iomem		*io_base;

	struct ftaes020_ctx	*ctx;
	struct device		*dev;
	int			irq;

	unsigned long		flags;
	int			err;

	spinlock_t		lock;
	struct crypto_queue	queue;

	struct tasklet_struct	done_task;
	struct tasklet_struct	queue_task;

	struct ablkcipher_request *req;
	size_t			total;

	struct scatterlist	*in_sg;
	unsigned int		nb_in_sg;
	size_t			in_offset;
	struct scatterlist	*out_sg;
	unsigned int		nb_out_sg;
	size_t			out_offset;

	size_t	bufcnt;
	size_t	buflen;
	size_t	dma_size;
	size_t	copy_size;

	void		*buf_in;
	int		dma_in;
	dma_addr_t	dma_addr_in;

	void		*buf_out;
	int		dma_out;
	dma_addr_t	dma_addr_out;

	u32	hw_version;
};

struct ftaes020_drv {
	struct list_head	dev_list;
	spinlock_t		lock;
};

static struct ftaes020_drv ftaes020_aes = {
	.dev_list = LIST_HEAD_INIT(ftaes020_aes.dev_list),
	.lock = __SPIN_LOCK_UNLOCKED(ftaes020_aes.lock),
};

static inline u32 ftaes020_read(struct ftaes020_dev *dd, u32 offset)
{
	return readl_relaxed(dd->io_base + offset);
}

static inline void ftaes020_write(struct ftaes020_dev *dd,
					u32 offset, u32 value)
{
	writel_relaxed(value, dd->io_base + offset);
}

static struct ftaes020_dev *
		ftaes020_find_dev(struct ftaes020_ctx *ctx)
{
	struct ftaes020_dev *dd = NULL;
	struct ftaes020_dev *tmp;

	spin_lock_bh(&ftaes020_aes.lock);
	if (!ctx->dd) {
		list_for_each_entry(tmp, &ftaes020_aes.dev_list, list) {
			dd = tmp;
			break;
		}
		ctx->dd = dd;
	} else {
		dd = ctx->dd;
	}

	spin_unlock_bh(&ftaes020_aes.lock);

	return dd;
}

static int ftaes020_hw_init(struct ftaes020_dev *dd)
{
	if (!(dd->flags & FTAES020_FLAGS_INIT)) {
		ftaes020_write(dd, FTAES020_FIFO_THRES,
				(FTAES020_FIFO_THRES_IN(1) |
				FTAES020_FIFO_THRES_OUT(1)));
		ftaes020_write(dd, FTAES020_IER, FTAES020_INTR_DMA_DONE);
		dd->flags |= FTAES020_FLAGS_INIT;
		dd->err = 0;
	}

	return 0;
}

static void ftaes020_hw_version_init(struct ftaes020_dev *dd)
{
	ftaes020_hw_init(dd);

	dd->hw_version = ftaes020_read(dd, FTAES020_HW_VERSION);

	dev_info(dd->dev, "version: 0x%x\n", dd->hw_version);
}

static int ftaes020_sg_copy(struct scatterlist **sg, size_t *offset,
			void *buf, size_t buflen, size_t total, int out)
{
	unsigned int count, off = 0;

	debug("%s: sg 0x%08x sg_length %d offset %d\n", __func__,
		(u32)*sg, (*sg)->length, *offset);

	while (buflen && total) {
		count = min((*sg)->length - *offset, total);
		count = min(count, buflen);

		if (!count) {
			return off;
		}
		scatterwalk_map_and_copy(buf + off, *sg, *offset, count, out);

		off += count;
		buflen -= count;
		*offset += count;
		total -= count;

		if (*offset == (*sg)->length) {
			*sg = sg_next(*sg);
			if (*sg) {
				*offset = 0;
			} else
				total = 0;
		}
	}

	debug("%s: sg 0x%08x offset %d cnt %d\n", __func__, (u32)*sg,
		*offset, off);

	return off;
}

static void ftaes020_finish_req(struct ftaes020_dev *dd, int err)
{
	struct ablkcipher_request *req = dd->req;

	debug("%s: err=%d\n", __func__, err);

	dd->flags &= ~FTAES020_FLAGS_BUSY;

	req->base.complete(&req->base, err);
}

static int ftaes020_crypt_dma_stop(struct ftaes020_dev *dd)
{
	int err = 0;
	size_t count;

	ftaes020_write(dd, FTAES020_DMA_CTRL, FTAES020_DMA_CTRL_STOP);

	if (dd->flags & FTAES020_FLAGS_COPY) {
		debug("%s: total=%d dma sz=%d, copy=%d\n", __func__,
			dd->total, dd->dma_size, dd->copy_size);

		dma_sync_single_for_device(dd->dev, dd->dma_addr_out,
			dd->dma_size, DMA_FROM_DEVICE);

		/* copy data */
		count = ftaes020_sg_copy(&dd->out_sg, &dd->out_offset,
			dd->buf_out, dd->buflen, dd->copy_size, 1);
		if (count != dd->copy_size) {
			err = -EINVAL;
			pr_err("not all data converted: %u\n", count);
		}
	} else {
		debug("%s: unmap sg\n", __func__);
		dma_unmap_sg(dd->dev, dd->out_sg, 1, DMA_FROM_DEVICE);
		dma_unmap_sg(dd->dev, dd->in_sg, 1, DMA_TO_DEVICE);
	}

	return err;
}


static int ftaes020_crypt_dma(struct ftaes020_dev *dd,
		dma_addr_t dma_addr_in, dma_addr_t dma_addr_out, int length)
{
	/* HW DMA transfer size must be multiple of block size*/
	dd->dma_size = ALIGN(length, dd->ctx->block_size);

	if (dd->flags & FTAES020_FLAGS_COPY) {

		dd->copy_size = length;
		dma_sync_single_for_device(dd->dev, dma_addr_in, dd->dma_size,
					   DMA_TO_DEVICE);
	}

	ftaes020_write(dd, FTAES020_DMA_SRC, dma_addr_in);
	ftaes020_write(dd, FTAES020_DMA_DST, dma_addr_out);
	ftaes020_write(dd, FTAES020_DMA_SIZE, dd->dma_size);

	debug("%s: len %d trans size %d\n", __func__, length,
		ftaes020_read(dd, FTAES020_DMA_SIZE));

	ftaes020_write(dd, FTAES020_DMA_CTRL, FTAES020_DMA_CTRL_EN);

	return 0;
}

static int ftaes020_check_aligned(struct ftaes020_dev *dd,
				struct scatterlist *sg, int total)
{
	int len = 0;

	if (!IS_ALIGNED(total, dd->ctx->block_size))
		return -EINVAL;

	while (sg) {
		if (!IS_ALIGNED(sg->offset, 4))
			return -1;
		if (!IS_ALIGNED(sg->length, dd->ctx->block_size))
			return -1;

		len += sg->length;
		sg = sg_next(sg);
	}

	if (len != total)
		return -1;

	return 0;
}

static int ftaes020_crypt_dma_start(struct ftaes020_dev *dd)
{
	int err;
	size_t count;
	dma_addr_t addr_in, addr_out;

	debug("%s: in_sg 0x%08x out_sg 0x%08x total %d\n", __func__,
		(u32)dd->in_sg, (u32)dd->out_sg, dd->total);

	if (dd->flags & FTAES020_FLAGS_COPY)  {
		/* use cache buffers */
		count = ftaes020_sg_copy(&dd->in_sg, &dd->in_offset,
				dd->buf_in, dd->buflen, dd->total, 0);

		addr_in = dd->dma_addr_in;
		addr_out = dd->dma_addr_out;

	} else {
//		count = min(dd->total, sg_dma_len(dd->in_sg));
//		count = min(count, sg_dma_len(dd->out_sg));

                count = min_t(size_t, dd->total, dd->in_sg->length);
                count = min_t(size_t, count, dd->out_sg->length);

		err = dma_map_sg(dd->dev, dd->in_sg, 1, DMA_TO_DEVICE);
		if (!err) {
			dev_err(dd->dev, "dma_map_sg() error\n");
			return -EINVAL;
		}

		err = dma_map_sg(dd->dev, dd->out_sg, 1,
				DMA_FROM_DEVICE);
		if (!err) {
			dev_err(dd->dev, "dma_map_sg() error\n");
			dma_unmap_sg(dd->dev, dd->in_sg, 1,
				DMA_TO_DEVICE);
			return -EINVAL;
		}
//		count = min(dd->total, sg_dma_len(dd->in_sg));
//		count = min(count, sg_dma_len(dd->out_sg));

		addr_in = sg_dma_address(dd->in_sg);
		addr_out = sg_dma_address(dd->out_sg);
	}

	dd->total -= count;

	debug("%s: total %d count %d off %d dd->flags 0x%lx\n", __func__,
		dd->total, count, dd->in_offset, dd->flags);

	err = ftaes020_crypt_dma(dd, addr_in, addr_out, count);

	if (err && !(dd->flags & FTAES020_FLAGS_COPY)) {
		dma_unmap_sg(dd->dev, dd->in_sg, 1, DMA_TO_DEVICE);
		dma_unmap_sg(dd->dev, dd->out_sg, 1, DMA_TO_DEVICE);
	}

	return err;
}

static int ftaes020_write_ctrl(struct ftaes020_dev *dd)
{
	u32 i, ctrl = 0;

	ftaes020_hw_init(dd);

	if (dd->flags & FTAES020_ALG_FLAGS_AES) {
		if (dd->ctx->keylen == AES_KEYSIZE_128)
			ctrl |= FTAES020_CR_ALG_AES_128;
		else if (dd->ctx->keylen == AES_KEYSIZE_192)
			ctrl |= FTAES020_CR_ALG_AES_192;
		else
			ctrl |= FTAES020_CR_ALG_AES_256;
	} else {
		if (dd->ctx->keylen == DES_KEY_SIZE)
			ctrl |= FTAES020_CR_ALG_DES;
		else
			ctrl |= FTAES020_CR_ALG_DES3;

	}

	if (dd->flags & FTAES020_MODE_FLAGS_CBC) {
		ctrl |= FTAES020_CR_MODE_CBC;
	} else if (dd->flags & FTAES020_MODE_FLAGS_CFB) {
		ctrl |= FTAES020_CR_MODE_CFB;
	} else if (dd->flags & FTAES020_MODE_FLAGS_OFB) {
		ctrl |= FTAES020_CR_MODE_OFB;
	} else if (dd->flags & FTAES020_MODE_FLAGS_CTR) {
		ctrl |= FTAES020_CR_MODE_CTR;
	} else {
		ctrl |= FTAES020_CR_MODE_ECB;
	}

	if (dd->flags & FTAES020_FLAGS_DECRYPT)
		ctrl |= FTAES020_CR_DECRYPT;

	for (i = 0; i < (dd->ctx->keylen >> 2); i++) {
		u32 value;

		value =  cpu_to_be32(dd->ctx->key[i]);
		ftaes020_write(dd, FTAES020_KEYWR(i), value);
	}

	if (((dd->flags & FTAES020_MODE_FLAGS_CBC) ||
		(dd->flags & FTAES020_MODE_FLAGS_CFB) ||
		(dd->flags & FTAES020_MODE_FLAGS_OFB) ||
		(dd->flags & FTAES020_MODE_FLAGS_CTR)) && dd->req->info) {
		for (i = 0; i < 4; i++) {
			u32 value, iv;

			iv = *((u32 *) dd->req->info + i);
			value =  cpu_to_be32(iv);
			ftaes020_write(dd, FTAES020_IVR(i), value);
		}

		ctrl |= FTAES020_CR_LOAD_IV;
	}

	debug("%s: ctrl 0x%08x\n", __func__, ctrl);

	ftaes020_write(dd, FTAAES020_CR, ctrl);

	return 0;
}

static int ftaes020_handle_queue(struct ftaes020_dev *dd,
			       struct ablkcipher_request *req)
{
	struct crypto_async_request *async_req, *backlog;
	struct ftaes020_ctx *ctx;
	struct ftaes020_reqctx *rctx;
	unsigned long flags;
	int err, ret = 0;

	spin_lock_irqsave(&dd->lock, flags);
	if (req)
		ret = ablkcipher_enqueue_request(&dd->queue, req);
	if (dd->flags & FTAES020_FLAGS_BUSY) {
		spin_unlock_irqrestore(&dd->lock, flags);
		return ret;
	}
	backlog = crypto_get_backlog(&dd->queue);
	async_req = crypto_dequeue_request(&dd->queue);
	if (async_req)
		dd->flags |= FTAES020_FLAGS_BUSY;
	spin_unlock_irqrestore(&dd->lock, flags);

	if (!async_req)
		return ret;

	if (backlog)
		backlog->complete(backlog, -EINPROGRESS);

	req = ablkcipher_request_cast(async_req);

	/* assign new request to device */
	dd->req = req;
	dd->total = req->nbytes;
	dd->in_offset = 0;
	dd->in_sg = req->src;
	dd->out_offset = 0;
	dd->out_sg = req->dst;

	rctx = ablkcipher_request_ctx(req);
	ctx = crypto_ablkcipher_ctx(crypto_ablkcipher_reqtfm(req));
	rctx->mode &= FTAES020_FLAGS_MODE_MASK;
	dd->flags = (dd->flags & ~FTAES020_FLAGS_MODE_MASK) | rctx->mode;
	dd->ctx = ctx;
	ctx->dd = dd;

	if (ftaes020_check_aligned(dd, dd->in_sg, dd->total) ||
	    ftaes020_check_aligned(dd, dd->out_sg, dd->total)) {
		dd->flags |= FTAES020_FLAGS_COPY;
	} else {
		dd->flags &= ~FTAES020_FLAGS_COPY;
	}

	err = ftaes020_write_ctrl(dd);
	if (!err) {
		err = ftaes020_crypt_dma_start(dd);
	}

	if (err) {
		/* task will not finish it, so do it here */
		ftaes020_finish_req(dd, err);
		tasklet_schedule(&dd->queue_task);
	}

	return ret;
}

static int ftaes020_buff_init(struct ftaes020_dev *dd)
{
	int err = -ENOMEM;

	dd->buf_in = (void *)__get_free_pages(GFP_KERNEL, 0);
	dd->buf_out = (void *)__get_free_pages(GFP_KERNEL, 0);
	/* Due to FTAES020 DMA max transfer size is 4095 bytes */
	dd->buflen = PAGE_SIZE - 1;
	dd->buflen &= ~(AES_BLOCK_SIZE - 1);

	if (!dd->buf_in || !dd->buf_out) {
		dev_err(dd->dev, "unable to alloc pages.\n");
		goto err_alloc;
	}

	/* MAP here */
	dd->dma_addr_in = dma_map_single(dd->dev, dd->buf_in,
					dd->buflen, DMA_TO_DEVICE);
	if (dma_mapping_error(dd->dev, dd->dma_addr_in)) {
		dev_err(dd->dev, "dma %d bytes error\n", dd->buflen);
		err = -EINVAL;
		goto err_map_in;
	}

	dd->dma_addr_out = dma_map_single(dd->dev, dd->buf_out,
					dd->buflen, DMA_FROM_DEVICE);
	if (dma_mapping_error(dd->dev, dd->dma_addr_out)) {
		dev_err(dd->dev, "dma %d bytes error\n", dd->buflen);
		err = -EINVAL;
		goto err_map_out;
	}

	return 0;

err_map_out:
	dma_unmap_single(dd->dev, dd->dma_addr_in, dd->buflen,
		DMA_TO_DEVICE);
err_map_in:
	free_page((unsigned long)dd->buf_out);
	free_page((unsigned long)dd->buf_in);
err_alloc:
	if (err)
		pr_err("error: %d\n", err);
	return err;
}

static void ftaes020_buff_cleanup(struct ftaes020_dev *dd)
{
	dma_unmap_single(dd->dev, dd->dma_addr_out, dd->buflen,
			 DMA_FROM_DEVICE);
	dma_unmap_single(dd->dev, dd->dma_addr_in, dd->buflen,
		DMA_TO_DEVICE);
	free_page((unsigned long)dd->buf_out);
	free_page((unsigned long)dd->buf_in);
}

static int ftaes020_crypt(struct ablkcipher_request *req, unsigned long mode)
{
	struct ftaes020_ctx *ctx = crypto_ablkcipher_ctx(
			crypto_ablkcipher_reqtfm(req));
	struct ftaes020_reqctx *rctx = ablkcipher_request_ctx(req);

	if (mode & FTAES020_MODE_FLAGS_CFB) {
		if (!IS_ALIGNED(req->nbytes, CFB_BLOCK_SIZE)) {
			pr_err("request size is not exact amount of CFB blocks\n");
			return -EINVAL;
		}
		ctx->block_size = CFB_BLOCK_SIZE;
	} else {
		if (mode & FTAES020_ALG_FLAGS_AES)
			ctx->block_size = AES_BLOCK_SIZE;
		else if (mode & FTAES020_ALG_FLAGS_DES)
			ctx->block_size = DES_BLOCK_SIZE;
		else {
			pr_err("alg not set correctly in mode\n");
			return -EINVAL;
		}
	}

	debug("%s: nbytes: %d, dec: %d, mode: %lu\n", __func__, req->nbytes,
		  !!(mode & FTAES020_FLAGS_DECRYPT),
		  (mode & FTAES020_FLAGS_MODE_MASK));

	rctx->mode = mode;

	return ftaes020_handle_queue(ctx->dd, req);
}

static int ftaes020_aes_setkey(struct crypto_ablkcipher *tfm, const u8 *key,
			   unsigned int keylen)
{
	struct ftaes020_ctx *ctx = crypto_ablkcipher_ctx(tfm);

	if (keylen != AES_KEYSIZE_128 && keylen != AES_KEYSIZE_192 &&
		   keylen != AES_KEYSIZE_256) {
		crypto_ablkcipher_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);
		return -EINVAL;
	}

	memcpy(ctx->key, key, keylen);
	ctx->keylen = keylen;

	return 0;
}

static int ftaes020_des_setkey(struct crypto_ablkcipher *tfm, const u8 *key,
			   unsigned int keylen)
{
	u32 tmp[DES_EXPKEY_WORDS];
	int err;
	struct crypto_tfm *ctfm = crypto_ablkcipher_tfm(tfm);

	struct ftaes020_ctx *ctx = crypto_ablkcipher_ctx(tfm);

	if (keylen != DES_KEY_SIZE) {
		crypto_ablkcipher_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);
		return -EINVAL;
	}

	err = des_ekey(tmp, key);
	if (err == 0 && (ctfm->crt_flags & CRYPTO_TFM_REQ_WEAK_KEY)) {
		ctfm->crt_flags |= CRYPTO_TFM_RES_WEAK_KEY;
		return -EINVAL;
	}

	memcpy(ctx->key, key, keylen);
	ctx->keylen = keylen;

	return 0;
}

static int ftaes020_tdes_setkey(struct crypto_ablkcipher *tfm, const u8 *key,
			   unsigned int keylen)
{
	struct ftaes020_ctx *ctx = crypto_ablkcipher_ctx(tfm);

	if (keylen != DES3_EDE_KEY_SIZE) {
		crypto_ablkcipher_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);
		return -EINVAL;
	}

	memcpy(ctx->key, key, keylen);
	ctx->keylen = keylen;

	return 0;
}

/* AES encrypt/decrypt function */
static int ftaes020_aes_ecb_encrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_ALG_FLAGS_AES);
}

static int ftaes020_aes_ecb_decrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_FLAGS_DECRYPT |
					FTAES020_ALG_FLAGS_AES);
}

static int ftaes020_aes_cbc_encrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_ALG_FLAGS_AES |
					FTAES020_MODE_FLAGS_CBC);
}

static int ftaes020_aes_cbc_decrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_FLAGS_DECRYPT |
					FTAES020_ALG_FLAGS_AES |
					FTAES020_MODE_FLAGS_CBC);
}

static int ftaes020_aes_ofb_encrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_ALG_FLAGS_AES |
					FTAES020_MODE_FLAGS_OFB);
}

static int ftaes020_aes_ofb_decrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_FLAGS_DECRYPT |
					FTAES020_ALG_FLAGS_AES |
					FTAES020_MODE_FLAGS_OFB);
}

static int ftaes020_aes_cfb_encrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_ALG_FLAGS_AES |
					FTAES020_MODE_FLAGS_CFB);
}

static int ftaes020_aes_cfb_decrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_FLAGS_DECRYPT |
					FTAES020_ALG_FLAGS_AES |
					FTAES020_MODE_FLAGS_CFB);
}

static int ftaes020_aes_ctr_encrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_ALG_FLAGS_AES |
					FTAES020_MODE_FLAGS_CTR);
}

static int ftaes020_aes_ctr_decrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_FLAGS_DECRYPT |
					FTAES020_ALG_FLAGS_AES |
					FTAES020_MODE_FLAGS_CTR);
}

/* DES and Triple DES encrypt/decrypt function */
static int ftaes020_tdes_ecb_encrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_ALG_FLAGS_DES);
}

static int ftaes020_tdes_ecb_decrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_FLAGS_DECRYPT |
					FTAES020_ALG_FLAGS_DES);
}

static int ftaes020_tdes_cbc_encrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_ALG_FLAGS_DES |
					FTAES020_MODE_FLAGS_CBC);
}

static int ftaes020_tdes_cbc_decrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_FLAGS_DECRYPT |
					FTAES020_ALG_FLAGS_DES |
					FTAES020_MODE_FLAGS_CBC);
}

static int ftaes020_tdes_cfb_encrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_ALG_FLAGS_DES |
					FTAES020_MODE_FLAGS_CFB);
}

static int ftaes020_tdes_cfb_decrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_FLAGS_DECRYPT |
					FTAES020_ALG_FLAGS_DES |
					FTAES020_MODE_FLAGS_CFB);
}

static int ftaes020_tdes_ofb_encrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_ALG_FLAGS_DES |
					FTAES020_MODE_FLAGS_OFB);
}

static int ftaes020_tdes_ofb_decrypt(struct ablkcipher_request *req)
{
	return ftaes020_crypt(req, FTAES020_FLAGS_DECRYPT |
					FTAES020_ALG_FLAGS_DES |
					FTAES020_MODE_FLAGS_OFB);
}

static int ftaes020_cra_init(struct crypto_tfm *tfm)
{
	struct ftaes020_ctx *ctx = crypto_tfm_ctx(tfm);
	struct ftaes020_dev *dd = NULL;
	int err;

	/* Find AES020 device, currently picks the first device */
	dd = ftaes020_find_dev(ctx);
	if (!dd)
		return -ENODEV;

	err = pm_runtime_get_sync(dd->dev);
	if (err < 0) {
		dev_err(dd->dev, "%s: failed to get_sync(%d)\n",
			__func__, err);
		return err;
	}

	tfm->crt_ablkcipher.reqsize = sizeof(struct ftaes020_reqctx);

	return 0;
}

static void ftaes020_cra_exit(struct crypto_tfm *tfm)
{
	struct ftaes020_ctx *ctx = crypto_tfm_ctx(tfm);
	struct ftaes020_dev *dd = ctx->dd;

	pm_runtime_put_sync(dd->dev);
}


static struct crypto_alg ftaes020_algs[] = {
{
	.cra_name		= "ecb(aes)",
	.cra_driver_name	= "ftaes020-ecb-aes",
	.cra_priority		= 100,
	.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize		= AES_BLOCK_SIZE,
	.cra_ctxsize		= sizeof(struct ftaes020_ctx),
	.cra_alignmask		= 0,
	.cra_type		= &crypto_ablkcipher_type,
	.cra_module		= THIS_MODULE,
	.cra_init		= ftaes020_cra_init,
	.cra_exit		= ftaes020_cra_exit,
	.cra_u.ablkcipher = {
		.min_keysize	= AES_MIN_KEY_SIZE,
		.max_keysize	= AES_MAX_KEY_SIZE,
		.setkey		= ftaes020_aes_setkey,
		.encrypt	= ftaes020_aes_ecb_encrypt,
		.decrypt	= ftaes020_aes_ecb_decrypt,
	}
},
{
	.cra_name		= "cbc(aes)",
	.cra_driver_name	= "ftaes020-cbc-aes",
	.cra_priority		= 100,
	.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize		= AES_BLOCK_SIZE,
	.cra_ctxsize		= sizeof(struct ftaes020_ctx),
	.cra_alignmask		= 0,
	.cra_type		= &crypto_ablkcipher_type,
	.cra_module		= THIS_MODULE,
	.cra_init		= ftaes020_cra_init,
	.cra_exit		= ftaes020_cra_exit,
	.cra_u.ablkcipher = {
		.min_keysize	= AES_MIN_KEY_SIZE,
		.max_keysize	= AES_MAX_KEY_SIZE,
		.ivsize		= AES_BLOCK_SIZE,
		.setkey		= ftaes020_aes_setkey,
		.encrypt	= ftaes020_aes_cbc_encrypt,
		.decrypt	= ftaes020_aes_cbc_decrypt,
	}
},
{
	.cra_name		= "ofb(aes)",
	.cra_driver_name	= "ftaes020-ofb-aes",
	.cra_priority		= 100,
	.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize		= AES_BLOCK_SIZE,
	.cra_ctxsize		= sizeof(struct ftaes020_ctx),
	.cra_alignmask		= 0,
	.cra_type		= &crypto_ablkcipher_type,
	.cra_module		= THIS_MODULE,
	.cra_init		= ftaes020_cra_init,
	.cra_exit		= ftaes020_cra_exit,
	.cra_u.ablkcipher = {
		.min_keysize	= AES_MIN_KEY_SIZE,
		.max_keysize	= AES_MAX_KEY_SIZE,
		.ivsize		= AES_BLOCK_SIZE,
		.setkey		= ftaes020_aes_setkey,
		.encrypt	= ftaes020_aes_ofb_encrypt,
		.decrypt	= ftaes020_aes_ofb_decrypt,
	}
},
{
	.cra_name		= "cfb(aes)",
	.cra_driver_name	= "ftaes020-cfb-aes",
	.cra_priority		= 100,
	.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize		= CFB_BLOCK_SIZE,
	.cra_ctxsize		= sizeof(struct ftaes020_ctx),
	.cra_alignmask		= 0,
	.cra_type		= &crypto_ablkcipher_type,
	.cra_module		= THIS_MODULE,
	.cra_init		= ftaes020_cra_init,
	.cra_exit		= ftaes020_cra_exit,
	.cra_u.ablkcipher = {
		.min_keysize	= AES_MIN_KEY_SIZE,
		.max_keysize	= AES_MAX_KEY_SIZE,
		.ivsize		= AES_BLOCK_SIZE,
		.setkey		= ftaes020_aes_setkey,
		.encrypt	= ftaes020_aes_cfb_encrypt,
		.decrypt	= ftaes020_aes_cfb_decrypt,
	}
},
{
	.cra_name		= "ctr(aes)",
	.cra_driver_name	= "ftaes020-ctr-aes",
	.cra_priority		= 100,
	.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize		= AES_BLOCK_SIZE,
	.cra_ctxsize		= sizeof(struct ftaes020_ctx),
	.cra_alignmask		= 0,
	.cra_type		= &crypto_ablkcipher_type,
	.cra_module		= THIS_MODULE,
	.cra_init		= ftaes020_cra_init,
	.cra_exit		= ftaes020_cra_exit,
	.cra_u.ablkcipher = {
		.min_keysize	= AES_MIN_KEY_SIZE,
		.max_keysize	= AES_MAX_KEY_SIZE,
		.ivsize		= AES_BLOCK_SIZE,
		.setkey		= ftaes020_aes_setkey,
		.encrypt	= ftaes020_aes_ctr_encrypt,
		.decrypt	= ftaes020_aes_ctr_decrypt,
	}
},
{
	.cra_name		= "ecb(des)",
	.cra_driver_name	= "ftaes020-ecb-des",
	.cra_priority		= 100,
	.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize		= DES_BLOCK_SIZE,
	.cra_ctxsize		= sizeof(struct ftaes020_ctx),
	.cra_alignmask		= 0,
	.cra_type		= &crypto_ablkcipher_type,
	.cra_module		= THIS_MODULE,
	.cra_init		= ftaes020_cra_init,
	.cra_exit		= ftaes020_cra_exit,
	.cra_u.ablkcipher = {
		.min_keysize	= DES_KEY_SIZE,
		.max_keysize	= DES_KEY_SIZE,
		.setkey		= ftaes020_des_setkey,
		.encrypt	= ftaes020_tdes_ecb_encrypt,
		.decrypt	= ftaes020_tdes_ecb_decrypt,
	}
},
{
	.cra_name		= "cbc(des)",
	.cra_driver_name	= "ftaes020-cbc-des",
	.cra_priority		= 100,
	.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize		= DES_BLOCK_SIZE,
	.cra_ctxsize		= sizeof(struct ftaes020_ctx),
	.cra_alignmask		= 0,
	.cra_type		= &crypto_ablkcipher_type,
	.cra_module		= THIS_MODULE,
	.cra_init		= ftaes020_cra_init,
	.cra_exit		= ftaes020_cra_exit,
	.cra_u.ablkcipher = {
		.min_keysize	= DES_KEY_SIZE,
		.max_keysize	= DES_KEY_SIZE,
		.ivsize		= DES_BLOCK_SIZE,
		.setkey		= ftaes020_des_setkey,
		.encrypt	= ftaes020_tdes_cbc_encrypt,
		.decrypt	= ftaes020_tdes_cbc_decrypt,
	}
},
{
	.cra_name		= "cfb(des)",
	.cra_driver_name	= "ftaes020-cfb-des",
	.cra_priority		= 100,
	.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize		= CFB_BLOCK_SIZE,
	.cra_ctxsize		= sizeof(struct ftaes020_ctx),
	.cra_alignmask		= 0,
	.cra_type		= &crypto_ablkcipher_type,
	.cra_module		= THIS_MODULE,
	.cra_init		= ftaes020_cra_init,
	.cra_exit		= ftaes020_cra_exit,
	.cra_u.ablkcipher = {
		.min_keysize	= DES_KEY_SIZE,
		.max_keysize	= DES_KEY_SIZE,
		.ivsize		= DES_BLOCK_SIZE,
		.setkey		= ftaes020_des_setkey,
		.encrypt	= ftaes020_tdes_cfb_encrypt,
		.decrypt	= ftaes020_tdes_cfb_decrypt,
	}
},
{
	.cra_name		= "ofb(des)",
	.cra_driver_name	= "ftaes020-ofb-des",
	.cra_priority		= 100,
	.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize		= DES_BLOCK_SIZE,
	.cra_ctxsize		= sizeof(struct ftaes020_ctx),
	.cra_alignmask		= 0,
	.cra_type		= &crypto_ablkcipher_type,
	.cra_module		= THIS_MODULE,
	.cra_init		= ftaes020_cra_init,
	.cra_exit		= ftaes020_cra_exit,
	.cra_u.ablkcipher = {
		.min_keysize	= DES_KEY_SIZE,
		.max_keysize	= DES_KEY_SIZE,
		.ivsize		= DES_BLOCK_SIZE,
		.setkey		= ftaes020_des_setkey,
		.encrypt	= ftaes020_tdes_ofb_encrypt,
		.decrypt	= ftaes020_tdes_ofb_decrypt,
	}
},
{
	.cra_name		= "ecb(des3_ede)",
	.cra_driver_name	= "ftaes020-ecb-tdes",
	.cra_priority		= 100,
	.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize		= DES_BLOCK_SIZE,
	.cra_ctxsize		= sizeof(struct ftaes020_ctx),
	.cra_alignmask		= 0x7,
	.cra_type		= &crypto_ablkcipher_type,
	.cra_module		= THIS_MODULE,
	.cra_init		= ftaes020_cra_init,
	.cra_exit		= ftaes020_cra_exit,
	.cra_u.ablkcipher = {
		.min_keysize	= DES3_EDE_KEY_SIZE,
		.max_keysize	= DES3_EDE_KEY_SIZE,
		.setkey		= ftaes020_tdes_setkey,
		.encrypt	= ftaes020_tdes_ecb_encrypt,
		.decrypt	= ftaes020_tdes_ecb_decrypt,
	}
},
{
	.cra_name		= "cbc(des3_ede)",
	.cra_driver_name	= "ftaes020-cbc-tdes",
	.cra_priority		= 100,
	.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize		= DES_BLOCK_SIZE,
	.cra_ctxsize		= sizeof(struct ftaes020_ctx),
	.cra_alignmask		= 0,
	.cra_type		= &crypto_ablkcipher_type,
	.cra_module		= THIS_MODULE,
	.cra_init		= ftaes020_cra_init,
	.cra_exit		= ftaes020_cra_exit,
	.cra_u.ablkcipher = {
		.min_keysize	= DES3_EDE_KEY_SIZE,
		.max_keysize	= DES3_EDE_KEY_SIZE,
		.ivsize		= DES_BLOCK_SIZE,
		.setkey		= ftaes020_tdes_setkey,
		.encrypt	= ftaes020_tdes_cbc_encrypt,
		.decrypt	= ftaes020_tdes_cbc_decrypt,
	}
},
{
	.cra_name		= "cfb(des3_ede)",
	.cra_driver_name	= "ftaes020-cfb-tdes",
	.cra_priority		= 100,
	.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize		= CFB_BLOCK_SIZE,
	.cra_ctxsize		= sizeof(struct ftaes020_ctx),
	.cra_alignmask		= 0,
	.cra_type		= &crypto_ablkcipher_type,
	.cra_module		= THIS_MODULE,
	.cra_init		= ftaes020_cra_init,
	.cra_exit		= ftaes020_cra_exit,
	.cra_u.ablkcipher = {
		.min_keysize	= DES3_EDE_KEY_SIZE,
		.max_keysize	= DES3_EDE_KEY_SIZE,
		.ivsize		= DES_BLOCK_SIZE,
		.setkey		= ftaes020_tdes_setkey,
		.encrypt	= ftaes020_tdes_cfb_encrypt,
		.decrypt	= ftaes020_tdes_cfb_decrypt,
	}
},
{
	.cra_name		= "ofb(des3_ede)",
	.cra_driver_name	= "ftaes020-ofb-tdes",
	.cra_priority		= 100,
	.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize		= DES_BLOCK_SIZE,
	.cra_ctxsize		= sizeof(struct ftaes020_ctx),
	.cra_alignmask		= 0,
	.cra_type		= &crypto_ablkcipher_type,
	.cra_module		= THIS_MODULE,
	.cra_init		= ftaes020_cra_init,
	.cra_exit		= ftaes020_cra_exit,
	.cra_u.ablkcipher = {
		.min_keysize	= DES3_EDE_KEY_SIZE,
		.max_keysize	= DES3_EDE_KEY_SIZE,
		.ivsize		= DES_BLOCK_SIZE,
		.setkey		= ftaes020_tdes_setkey,
		.encrypt	= ftaes020_tdes_ofb_encrypt,
		.decrypt	= ftaes020_tdes_ofb_decrypt,
	}
},
};

static void ftaes020_queue_task(unsigned long data)
{
	struct ftaes020_dev *dd = (struct ftaes020_dev *)data;

	ftaes020_handle_queue(dd, NULL);
}

static void ftaes020_done_task(unsigned long data)
{
	struct ftaes020_dev *dd = (struct ftaes020_dev *) data;
	int err;

	err = ftaes020_crypt_dma_stop(dd);

	err = dd->err ? : err;

	if (dd->total && !err) {
		if (!(dd->flags & FTAES020_FLAGS_COPY)) {
			dd->in_sg = sg_next(dd->in_sg);
			dd->out_sg = sg_next(dd->out_sg);
			if (!dd->in_sg || !dd->out_sg)
				err = -EINVAL;
		}
		if (!err)
			err = ftaes020_crypt_dma_start(dd);
		if (!err) {
			//debug("%s: DMA not finishing\n", __func__);
			return;
		}
	}

	ftaes020_finish_req(dd, err);
	ftaes020_handle_queue(dd, NULL);
}

static irqreturn_t ftaes020_irq(int irq, void *dev_id)
{
	struct ftaes020_dev *dd = dev_id;
	u32 reg;

	reg = ftaes020_read(dd, FTAES020_ISR);
	if (reg & ftaes020_read(dd, FTAES020_IMR)) {
		ftaes020_write(dd, FTAES020_ICR, reg);
		if (FTAES020_FLAGS_BUSY & dd->flags) {
			tasklet_schedule(&dd->done_task);
		}
		else
			dev_warn(dd->dev, "irq when no active requests.\n");
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static int ftaes020_register_algs(struct ftaes020_dev *dd)
{
	int err, i, j;

	for (i = 0; i < ARRAY_SIZE(ftaes020_algs); i++) {
		dev_dbg(dd->dev, "reg alg: %s\n", ftaes020_algs[i].cra_name);
		err = crypto_register_alg(&ftaes020_algs[i]);
		if (err)
			goto err_algs;
	}

	return 0;

	i = ARRAY_SIZE(ftaes020_algs);
err_algs:
	for (j = 0; j < i; j++)
		crypto_unregister_alg(&ftaes020_algs[j]);

	return err;
}

#if defined(CONFIG_OF)
static const struct of_device_id ftaes020_dt_ids[] = {
	{ .compatible = "faraday,ftaes020" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ftaes020_dt_ids);
#else
static int ftaes020_get_res_of(struct ftaes020_dev *dd,
		struct device *dev, struct resource *res)
{
	return -EINVAL;
}
#endif

static int ftaes020_get_res_pdev(struct ftaes020_dev *dd,
		struct platform_device *pdev, struct resource *res)
{
	struct device *dev = &pdev->dev;
	struct resource *r;
	int err = 0;

	/* Get the base address */
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r) {
		dev_err(dev, "no MEM resource info\n");
		err = -EINVAL;
		goto err;
	}
	memcpy(res, r, sizeof(*res));

	/* Get the IRQ */
	dd->irq = platform_get_irq(pdev,  0);
	if (dd->irq < 0) {
		dev_err(dev, "no IRQ resource info\n");
		err = dd->irq;
	}
err:
	return err;
}

static int ftaes020_probe(struct platform_device *pdev)
{
	struct ftaes020_dev *dd;
	struct device *dev = &pdev->dev;
	struct resource res;
	struct clk *clk;
	int err;

	dd = devm_kzalloc(dev, sizeof(struct ftaes020_dev), GFP_KERNEL);
	if (dd == NULL) {
		dev_err(dev, "unable to alloc data struct.\n");
		err = -ENOMEM;
		goto err_dd;
	}
	dd->dev = dev;
	platform_set_drvdata(pdev, dd);

	crypto_init_queue(&dd->queue, ATMEL_AES_QUEUE_LENGTH);

	err = ftaes020_get_res_pdev(dd, pdev, &res);
	if (err)
		goto err_res;

	dd->io_base = devm_ioremap_resource(dev, &res);
	if (IS_ERR(dd->io_base)) {
		err = PTR_ERR(dd->io_base);
		goto err_res;
	}
	dd->phys_base = res.start;

	err = devm_request_irq(dev, dd->irq, ftaes020_irq, 0,
			dev_name(dev), dd);
	if (err) {
		dev_err(dev, "Unable to request IRQ\n");
		goto err_irq;
	}

	clk = devm_clk_get(&pdev->dev, "aes_clk");
	if (!IS_ERR(clk)) {
		clk_prepare_enable(clk);
	}

	pm_runtime_enable(dev);
	err = pm_runtime_get_sync(dev);
	if (err < 0) {
		dev_err(dev, "%s: failed to get_sync(%d)\n",
			__func__, err);
		goto err_pm;
	}

	INIT_LIST_HEAD(&dd->list);

	tasklet_init(&dd->done_task, ftaes020_done_task,
					(unsigned long)dd);
	tasklet_init(&dd->queue_task, ftaes020_queue_task,
					(unsigned long)dd);

	err = ftaes020_buff_init(dd);
	if (err)
		goto err_buff;

	spin_lock(&ftaes020_aes.lock);
	list_add_tail(&dd->list, &ftaes020_aes.dev_list);
	spin_unlock(&ftaes020_aes.lock);

	ftaes020_hw_version_init(dd);

	pm_runtime_put_sync(dev);

	dev_info(dev, "FTAES020 AES - io base %p(phys 0x%lx, irq %d)\n",
			dd->io_base, dd->phys_base, dd->irq);

	err = ftaes020_register_algs(dd);
	if (err)
		goto err_algs;

	return 0;

err_algs:
	spin_lock(&ftaes020_aes.lock);
	list_del(&dd->list);
	spin_unlock(&ftaes020_aes.lock);

	ftaes020_buff_cleanup(dd);
err_buff:
	tasklet_kill(&dd->done_task);
	tasklet_kill(&dd->queue_task);
	pm_runtime_disable(dev);
err_pm:
err_irq:
err_res:
	kfree(dd);
	dd = NULL;
err_dd:
	dev_err(dev, "initialization failed.\n");

	return err;
}

static int ftaes020_remove(struct platform_device *pdev)
{
	struct ftaes020_dev *dd = platform_get_drvdata(pdev);
	int i;

	if (!dd)
		return -ENODEV;

	spin_lock(&ftaes020_aes.lock);
	list_del(&dd->list);
	spin_unlock(&ftaes020_aes.lock);

	for (i = 0; i < ARRAY_SIZE(ftaes020_algs); i++)
		crypto_unregister_alg(&ftaes020_algs[i]);

	tasklet_kill(&dd->done_task);
	tasklet_kill(&dd->queue_task);
	pm_runtime_disable(dd->dev);
	dd = NULL;

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ftaes020_suspend(struct device *dev)
{
	pm_runtime_put_sync(dev);
	return 0;
}

static int ftaes020_resume(struct device *dev)
{
	pm_runtime_get_sync(dev);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(ftaes020_pm_ops, ftaes020_suspend, ftaes020_resume);

static struct platform_driver ftaes020_driver = {
	.probe		= ftaes020_probe,
	.remove		= ftaes020_remove,
	.driver		= {
		.name	= "ftaes020",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(ftaes020_dt_ids),
	},
};

module_platform_driver(ftaes020_driver);

MODULE_DESCRIPTION("FTAES020 AES and DES/DES3 hw acceleration support.");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Bing-Yao,Luo - Faraday Tech");
