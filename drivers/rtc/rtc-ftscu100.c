/*
 *  drivers/rtc/rtc-ftscu100.c
 *
 *  Real Time Clock interface for Faraday FTSCU100
 *
 *  Copyright (C) 2015 Faraday Technology
 *  Copyright (C) 2015 Sanjin Liu <scliu@faraday-tech.com>
 *  Copyright (C) 2018 B.C. Chen <bcchen@faraday-tech.com>
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
 *
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/rtc.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/delay.h>

#include <mach/hardware.h>

#define VERID                   0x014
#define INT_STS                 0x024
#define RTC_TIME1               0x200
#define RTC_TIME2               0x204
#define RTC_ALM_TIME1           0x208
#define RTC_ALM_TIME2           0x20C
#define RTC_CR                  0x210
#define RTC_TRIM                0x214

#define INT_STS_RTC_SEC         (1 << 18)
#define INT_STS_RTC_PER         (1 << 17)
#define INT_STS_RTC_ALM         (1 << 16)

#define RTC_TIME1_WEEKDAY(x)    (((x)&0x07) << 24)
#define RTC_TIME1_HOUR(x)       (((x)&0x3f) << 16)
#define RTC_TIME1_MIN(x)        (((x)&0x7f) << 8)
#define RTC_TIME1_SEC(x)        (((x)&0x7f) << 0)

#define RTC_TIME2_CENTURY(x)    (((x)&0xff) << 24)
#define RTC_TIME2_YEAR(x)       (((x)&0xff) << 16)
#define RTC_TIME2_MONTH(x)      (((x)&0x1f) << 8)
#define RTC_TIME2_DATE(x)       (((x)&0x3f) << 0)

#define RTC_ALM_TIME1_WEEKDAY(x)(((x)&0x07) << 24)
#define RTC_ALM_TIME1_HOUR(x)   (((x)&0x3f) << 16)
#define RTC_ALM_TIME1_MIN(x)    (((x)&0x7f) << 8)
#define RTC_ALM_TIME1_SEC(x)    (((x)&0x7f) << 0)

#define RTC_ALM_TIME2_MONTH(x)  (((x)&0x1f) << 8)
#define RTC_ALM_TIME2_DATE(x)   (((x)&0x3f) << 0)

#define RTC_CR_OSC_PAD_FEB      (1 << 31)
#define RTC_CR_GPO(x)           (((x)&0xff) << 16)
#define RTC_CR_CLK_READY        (1 << 11)
#define RTC_CR_ALM_EN_STS       (1 << 9)
#define RTC_CR_EN_STS           (1 << 8)
#define RTC_CR_SECOUT_EN        (1 << 7)
#define RTC_CR_PERINT_SEL(x)    (((x)&0x07) << 4)
#define RTC_CR_LOCK_EN          (1 << 2)
#define RTC_CR_ALM_EN           (1 << 1)
#define RTC_CR_EN               (1 << 0)

#define rtc_readl(ftscu100_rtc, reg)	\
	__raw_readl((ftscu100_rtc)->base + (reg))
#define rtc_writel(ftscu100_rtc, reg, value)	\
	__raw_writel((value), (ftscu100_rtc)->base + (reg))

struct ftscu100_rtc {
	struct resource     *ress;
	void __iomem        *base;
	int                 irq;
	struct rtc_device   *rtc;
	spinlock_t          lock;       /* Protects this structure */
};

static irqreturn_t ftscu100_rtc_irq(int irq, void *dev_id)
{
	struct platform_device *pdev = to_platform_device(dev_id);
	struct ftscu100_rtc *ftscu100_rtc = platform_get_drvdata(pdev);
	int irqs_handled = 0;
	u32 sts_val;

	spin_lock(&ftscu100_rtc->lock);

	sts_val = rtc_readl(ftscu100_rtc, INT_STS);
	if (sts_val & (INT_STS_RTC_SEC | INT_STS_RTC_PER | INT_STS_RTC_ALM)) {
		if (sts_val & INT_STS_RTC_SEC) {
			rtc_writel(ftscu100_rtc, INT_STS, INT_STS_RTC_SEC);
			rtc_update_irq(ftscu100_rtc->rtc, 1, RTC_UF);
		}

		if (sts_val & INT_STS_RTC_PER) {
			rtc_writel(ftscu100_rtc, INT_STS, INT_STS_RTC_PER);
			rtc_update_irq(ftscu100_rtc->rtc, 1, RTC_PF);
		}

		if (sts_val & INT_STS_RTC_ALM) {
			rtc_writel(ftscu100_rtc, INT_STS, INT_STS_RTC_ALM);
			rtc_update_irq(ftscu100_rtc->rtc, 1, RTC_AF);
		}

		irqs_handled++;
	}

	spin_unlock(&ftscu100_rtc->lock);

	return irqs_handled ? IRQ_HANDLED : IRQ_NONE;
}

static int ftscu100_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	struct ftscu100_rtc *ftscu100_rtc = dev_get_drvdata(dev);
	u32 cr_val;

	spin_lock_irq(&ftscu100_rtc->lock);

	cr_val = rtc_readl(ftscu100_rtc, RTC_CR);
	if (enabled)
		rtc_writel(ftscu100_rtc, RTC_CR, cr_val | RTC_CR_ALM_EN);
	else
		rtc_writel(ftscu100_rtc, RTC_CR, cr_val & ~RTC_CR_ALM_EN);

	spin_unlock_irq(&ftscu100_rtc->lock);

	return 0;
}

static int ftscu100_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct ftscu100_rtc *ftscu100_rtc = dev_get_drvdata(dev);
	u32 time1, time2;

	spin_lock(&ftscu100_rtc->lock);

	time1 = rtc_readl(ftscu100_rtc, RTC_TIME1);
	time2 = rtc_readl(ftscu100_rtc, RTC_TIME2);

	tm->tm_sec  = ((time1 >>  0) & 0x7f);                   /* second */
	tm->tm_min  = ((time1 >>  8) & 0x7f);                   /* minute */
	tm->tm_hour = ((time1 >> 16) & 0x3f);                   /* hour */
	tm->tm_wday = ((time1 >> 24) & 0x07);                   /* day of the week [0-6], Sunday=0 */

	tm->tm_mday = ((time2 >>  0) & 0x3f);                   /* day of the month */
	tm->tm_mon  = ((time2 >>  8) & 0x1f) - 1;               /* month */
	tm->tm_year = ((time2 >> 16) & 0xff);                   /* year */
	tm->tm_year += (((time2 >> 24) & 0xff) * 100) - 1900;   /* century */

	spin_unlock(&ftscu100_rtc->lock);

	return rtc_valid_tm(tm);
}

static int ftscu100_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct ftscu100_rtc *ftscu100_rtc = dev_get_drvdata(dev);
	u32 time1, time2;

	spin_lock(&ftscu100_rtc->lock);

	time1  = (tm->tm_sec << 0);
	time1 |= (tm->tm_min << 8);
	time1 |= (tm->tm_hour << 16);
	time1 |= (tm->tm_wday << 24);
	rtc_writel(ftscu100_rtc, RTC_TIME1, time1);

	time2  = (tm->tm_mday << 0);
	time2 |= (tm->tm_mon  << 8);
	time2 |= ((tm->tm_year % 100) << 16);
	time2 |= ((tm->tm_year / 100) << 24);
	rtc_writel(ftscu100_rtc, RTC_TIME2, time2);

	spin_unlock(&ftscu100_rtc->lock);

	return 0;
}

static int ftscu100_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct ftscu100_rtc *ftscu100_rtc = dev_get_drvdata(dev);
	u32 time1, time2;

	spin_lock(&ftscu100_rtc->lock);

	time1 = rtc_readl(ftscu100_rtc, RTC_ALM_TIME1);
	time2 = rtc_readl(ftscu100_rtc, RTC_ALM_TIME2);

	alrm->time.tm_sec   = ((time1 >>  0) & 0x7f);           /* second */
	alrm->time.tm_min   = ((time1 >>  8) & 0x7f);           /* minute */
	alrm->time.tm_hour  = ((time1 >> 16) & 0x3f);           /* hour */
	alrm->time.tm_wday  = ((time1 >> 24) & 0x07);           /* day of the week [0-6], Sunday=0 */

	alrm->time.tm_mday  = ((time2 >>  0) & 0x3f);           /* day of the month */
	alrm->time.tm_mon   = ((time2 >>  8) & 0x1f);           /* month */

	alrm->enabled = rtc_readl(ftscu100_rtc, RTC_CR) & RTC_CR_ALM_EN;

	spin_unlock(&ftscu100_rtc->lock);

	return 0;
}

static int ftscu100_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct ftscu100_rtc *ftscu100_rtc = dev_get_drvdata(dev);
	u32 time1, time2;

	spin_lock(&ftscu100_rtc->lock);

	time1  = (alrm->time.tm_sec  << 0);
	time1 |= (alrm->time.tm_min  << 8);
	time1 |= (alrm->time.tm_hour << 16);
	time1 |= (alrm->time.tm_wday << 24);
	rtc_writel(ftscu100_rtc, RTC_ALM_TIME1, time1);

	time2  = (alrm->time.tm_mday << 0);
	time2 |= (alrm->time.tm_mon  << 8);
	rtc_writel(ftscu100_rtc, RTC_ALM_TIME2, time2);

	spin_unlock(&ftscu100_rtc->lock);

	return 0;
}

static const struct rtc_class_ops ftscu100_rtc_ops = {
	.read_time = ftscu100_rtc_read_time,
	.set_time = ftscu100_rtc_set_time,
	.read_alarm = ftscu100_rtc_read_alarm,
	.set_alarm = ftscu100_rtc_set_alarm,
	.alarm_irq_enable = ftscu100_alarm_irq_enable,
};

static int __init ftscu100_rtc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ftscu100_rtc *ftscu100_rtc;
	int ret = 0;
	u32 reg_val;

	ftscu100_rtc = devm_kzalloc(dev, sizeof(*ftscu100_rtc), GFP_KERNEL);
	if (!ftscu100_rtc)
		return -ENOMEM;

	spin_lock_init(&ftscu100_rtc->lock);
	platform_set_drvdata(pdev, ftscu100_rtc);

	ftscu100_rtc->ress = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!ftscu100_rtc->ress) {
		dev_err(dev, "No I/O memory resource defined\n");
		return -ENXIO;
	}

	ftscu100_rtc->base = devm_ioremap(dev, ftscu100_rtc->ress->start,
				resource_size(ftscu100_rtc->ress));
	if (!ftscu100_rtc->base) {
		dev_err(dev, "Unable to map ftrtc011 RTC I/O memory\n");
		return -ENOMEM;
	}

	/* Clear control register*/
	rtc_writel(ftscu100_rtc, RTC_CR, 0);

	ftscu100_rtc->irq = platform_get_irq(pdev, 0);
	if (ftscu100_rtc->irq < 0) {
		dev_err(dev, "No IRQ resource defined\n");
		return -ENXIO;
	}

	ret = request_irq(ftscu100_rtc->irq, ftscu100_rtc_irq, IRQF_SHARED,
			  "rtc-ftscu100", dev);
	if (ret < 0) {
		dev_err(dev, "can't get irq %i, err %d\n",
			ftscu100_rtc->irq, ret);
		return ret;
	}

	device_init_wakeup(dev, 1);

	/* set alarm time1 to 23:59:59 */
	rtc_writel(ftscu100_rtc, RTC_ALM_TIME1, 0x06173b3b);

	ftscu100_rtc->rtc = devm_rtc_device_register(&pdev->dev, "rtc-ftscu100",
						&ftscu100_rtc_ops, THIS_MODULE);
	if (IS_ERR(ftscu100_rtc->rtc)) {
		ret = PTR_ERR(ftscu100_rtc->rtc);
		dev_err(dev, "Failed to register RTC device -> %d\n", ret);
		goto err_out;
	}

	ftscu100_rtc->rtc->irq_freq = 1;
	ftscu100_rtc->rtc->max_user_freq = 1;

	/* unlock trimming control */
#ifdef CONFIG_MACH_LEO
	rtc_writel(ftscu100_rtc, VERID, 0x00030100);
#elif CONFIG_MACH_TC12NGRC
	rtc_writel(ftscu100_rtc, VERID, 0x00360000);
#elif CONFIG_MACH_UNISND
	rtc_writel(ftscu100_rtc, VERID, 0x00000001);
#endif
	/* set trimming factor */
	rtc_writel(ftscu100_rtc, RTC_TRIM, 0x01400000);	/* create 100Hz tick from 32.768KHz RTC clock */

	reg_val = RTC_CR_EN;
	/* enable periodic interrupt and triggered at 0 second of every minute */
	reg_val |= RTC_CR_SECOUT_EN;
	reg_val |= RTC_CR_PERINT_SEL(6);
	/* enable alarm */
	reg_val |= RTC_CR_ALM_EN;
	/* enable ftscu100_rtc */
	rtc_writel(ftscu100_rtc, RTC_CR, reg_val);

	return ret;
err_out:
	free_irq(ftscu100_rtc->irq, dev);
	return ret;
}

static int __exit ftscu100_rtc_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ftscu100_rtc *ftscu100_rtc = dev_get_drvdata(dev);

	/* disable ftscu100_rtc */
	rtc_writel(ftscu100_rtc, RTC_CR, 0);

	free_irq(ftscu100_rtc->irq, dev);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ftscu100_rtc_dt_ids[] = {
	{ .compatible = "faraday,rtc-ftscu100" },
	{}
};
MODULE_DEVICE_TABLE(of, ftscu100_rtc_dt_ids);
#endif

#ifdef CONFIG_PM_SLEEP
static int ftscu100_rtc_suspend(struct device *dev)
{
	struct ftscu100_rtc *ftscu100_rtc = dev_get_drvdata(dev);

	if (device_may_wakeup(dev))
		enable_irq_wake(ftscu100_rtc->irq);
	return 0;
}

static int ftscu100_rtc_resume(struct device *dev)
{
	struct ftscu100_rtc *ftscu100_rtc = dev_get_drvdata(dev);

	if (device_may_wakeup(dev))
		disable_irq_wake(ftscu100_rtc->irq);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(ftscu100_rtc_pm_ops, ftscu100_rtc_suspend,
			 ftscu100_rtc_resume);

static struct platform_driver ftscu100_rtc_driver = {
	.remove		= __exit_p(ftscu100_rtc_remove),
	.driver		= {
		.name	= "rtc-ftscu100",
		.of_match_table = of_match_ptr(ftscu100_rtc_dt_ids),
		.pm	= &ftscu100_rtc_pm_ops,
	},
};

module_platform_driver_probe(ftscu100_rtc_driver, ftscu100_rtc_probe);

MODULE_AUTHOR("B.C. Chen <bcchen@faraday-tech.com>");
MODULE_DESCRIPTION("Faraday FTSCU100 Realtime Clock Driver (RTC)");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:rtc-ftscu100");
