/*
 * Faraday FTPCIESNPS330 PCIE Controller
 *
 * (C) Copyright 2022-2023 Faraday Technology
 * Fu-Tsung Hsu <mark_hs@faraday-tech.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <uapi/linux/synclink.h>
#include "pcie-ftpciesnps330_gc1610.h"
#include "pcie-designware.h"

#define DRIVER_VERSION                  "2-April-2022"

#define IATU_REGION0                    0
#define IATU_REGION1                    1

#define IATU_TYPE_MEM                   0x0
#define IATU_TYPE_IO                    0x2
#define IATU_TYPE_CFG0                  0x4
#define IATU_TYPE_CFG1                  0x5

#define PCIE_SLV_DBI_ENABLE	BIT(21)

#define PCIE_PHY_DATA_CONTROL_OFFSET		0x0
#define PCIE_PHY_DATA_OFFSET				0x4
#define PCIE_SLV_AW_MISC					0x250
#define PCIE_SLV_AR_MISC					0x25C

#define IATU_BASE                       0x300000

#define IATU_OUTBOUND_REGION_CR1(x)     (IATU_BASE + (x)*0x200 + 0x000)
#define IATU_OUTBOUND_REGION_CR2(x)     (IATU_BASE + (x)*0x200 + 0x004)
#define IATU_OUTBOUND_LOWER_BASE(x)     (IATU_BASE + (x)*0x200 + 0x008)
#define IATU_OUTBOUND_UPPER_BASE(x)     (IATU_BASE + (x)*0x200 + 0x00C)
#define IATU_OUTBOUND_LIMIT(x)          (IATU_BASE + (x)*0x200 + 0x010)
#define IATU_OUTBOUND_LOWER_TARGET(x)   (IATU_BASE + (x)*0x200 + 0x014)
#define IATU_OUTBOUND_UPPER_TARGET(x)   (IATU_BASE + (x)*0x200 + 0x018)
#define IATU_OUTBOUND_REGION_CR3(x)     (IATU_BASE + (x)*0x200 + 0x01C)
#define IATU_OUTBOUND_UPPER_LIMIT(x)    (IATU_BASE + (x)*0x200 + 0x020)

#define IATU_INBOUND_REGION_CR1(x)      (IATU_BASE + (x)*0x200 + 0x100)
#define IATU_INBOUND_REGION_CR2(x)      (IATU_BASE + (x)*0x200 + 0x104)
#define IATU_INBOUND_LOWER_BASE(x)      (IATU_BASE + (x)*0x200 + 0x108)
#define IATU_INBOUND_UPPER_BASE(x)      (IATU_BASE + (x)*0x200 + 0x10C)
#define IATU_INBOUND_LIMIT(x)           (IATU_BASE + (x)*0x200 + 0x110)
#define IATU_INBOUND_LOWER_TARGET(x)    (IATU_BASE + (x)*0x200 + 0x114)
#define IATU_INBOUND_UPPER_TARGET(x)    (IATU_BASE + (x)*0x200 + 0x118)
#define IATU_INBOUND_REGION_CR3(x)      (IATU_BASE + (x)*0x200 + 0x11C)
#define IATU_INBOUND_UPPER_LIMIT(x)     (IATU_BASE + (x)*0x200 + 0x120)

#define to_ftpciesnps330_pcie(x)        container_of(x, struct ftpciesnps330_pcie, pci)

struct ftpciesnps330_pcie {
	struct dw_pcie      pci;
	void __iomem        *ctl_base;
	void __iomem        *phy_base;
};

static void ftpciesnps330_pcie_prog_outbound_atu(struct dw_pcie *pci, int index,
		int type, u64 cpu_addr, u64 pci_addr, u32 size)
{
	writel(lower_32_bits(cpu_addr),        pci->dbi_base + IATU_OUTBOUND_LOWER_BASE(index));
	writel(upper_32_bits(cpu_addr),        pci->dbi_base + IATU_OUTBOUND_UPPER_BASE(index));
	writel(lower_32_bits(cpu_addr) + size, pci->dbi_base + IATU_OUTBOUND_LIMIT(index));
	writel(lower_32_bits(pci_addr),        pci->dbi_base + IATU_OUTBOUND_LOWER_TARGET(index));
	writel(upper_32_bits(pci_addr),        pci->dbi_base + IATU_OUTBOUND_UPPER_TARGET(index));
	writel(type,                           pci->dbi_base + IATU_OUTBOUND_REGION_CR1(index));
	writel(0x80000000,                     pci->dbi_base + IATU_OUTBOUND_REGION_CR2(index));
}

static void ftpciesnps330_pcie_prog_inbound_atu(struct dw_pcie *pci, int index,
		int type, u64 cpu_addr, u64 pci_addr, u32 size)
{
	writel(lower_32_bits(cpu_addr),        pci->dbi_base + IATU_INBOUND_LOWER_BASE(index));
	writel(upper_32_bits(cpu_addr),        pci->dbi_base + IATU_INBOUND_UPPER_BASE(index));
	writel(lower_32_bits(cpu_addr) + size, pci->dbi_base + IATU_INBOUND_LIMIT(index));
	writel(lower_32_bits(pci_addr),        pci->dbi_base + IATU_INBOUND_LOWER_TARGET(index));
	writel(upper_32_bits(pci_addr),        pci->dbi_base + IATU_INBOUND_UPPER_TARGET(index));
	writel(type,                           pci->dbi_base + IATU_INBOUND_REGION_CR1(index));
	writel(0x80000000,                     pci->dbi_base + IATU_INBOUND_REGION_CR2(index));
}

#define PCIE_ATU_BUS(x)			(((x) & 0xff) << 24)
#define PCIE_ATU_DEV(x)			(((x) & 0x1f) << 19)
#define PCIE_ATU_FUNC(x)		(((x) & 0x7) << 16)
static int ftpciesnps330_pcie_rd_other_conf(struct pcie_port *pp, struct pci_bus *bus,
			unsigned int devfn, int where, int size, u32 *val)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct ftpciesnps330_pcie *hw = to_ftpciesnps330_pcie(pci);
	int ret, type;
	u32 busdev, cfg_size;
	u32 dbi_val;
	u64 cpu_addr;
	void __iomem *va_cfg_base;

	busdev = PCIE_ATU_BUS(bus->number) | PCIE_ATU_DEV(PCI_SLOT(devfn)) |
		 PCIE_ATU_FUNC(PCI_FUNC(devfn));

	if (bus->parent->number == pp->root_bus_nr) {
		type = IATU_TYPE_CFG0;
		cpu_addr = pp->cfg0_base;
		cfg_size = pp->cfg0_size;
		va_cfg_base = pp->va_cfg0_base;
	} else {
		type = IATU_TYPE_CFG1;
		cpu_addr = pp->cfg1_base;
		cfg_size = pp->cfg1_size;
		va_cfg_base = pp->va_cfg1_base;
	}

	writel(PCIE_SLV_DBI_ENABLE, hw->ctl_base + PCIE_SLV_AW_MISC);
	writel(PCIE_SLV_DBI_ENABLE, hw->ctl_base + PCIE_SLV_AR_MISC);

	ftpciesnps330_pcie_prog_outbound_atu(pci, IATU_REGION0,
				  type, cpu_addr,
				  busdev, cfg_size);

	writel(0, hw->ctl_base + PCIE_SLV_AW_MISC);
	writel(0, hw->ctl_base + PCIE_SLV_AR_MISC);						//disable dbi

	ret = dw_pcie_read(va_cfg_base + where, size, val);

	return ret;
}

static int ftpciesnps330_pcie_wr_other_conf(struct pcie_port *pp, struct pci_bus *bus,
			unsigned int devfn, int where, int size, u32 val)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct ftpciesnps330_pcie *hw = to_ftpciesnps330_pcie(pci);
	int ret, type;
	u32 busdev, cfg_size;
	u64 cpu_addr;
	void __iomem *va_cfg_base;
	u32 dbi_val;

	busdev = PCIE_ATU_BUS(bus->number) | PCIE_ATU_DEV(PCI_SLOT(devfn)) |
		 PCIE_ATU_FUNC(PCI_FUNC(devfn));

	if (bus->parent->number == pp->root_bus_nr) {
		type = IATU_TYPE_CFG0;
		cpu_addr = pp->cfg0_base;
		cfg_size = pp->cfg0_size;
		va_cfg_base = pp->va_cfg0_base;
	} else {
		type = IATU_TYPE_CFG1;
		cpu_addr = pp->cfg1_base;
		cfg_size = pp->cfg1_size;
		va_cfg_base = pp->va_cfg1_base;
	}

	writel(PCIE_SLV_DBI_ENABLE, hw->ctl_base + PCIE_SLV_AR_MISC);					//enable dbi
	writel(PCIE_SLV_DBI_ENABLE, hw->ctl_base + PCIE_SLV_AW_MISC);					//enable dbi
	ftpciesnps330_pcie_prog_outbound_atu(pci, IATU_REGION0,
				  type, cpu_addr,
				  busdev, cfg_size);
	writel(0, hw->ctl_base + PCIE_SLV_AW_MISC);									//disable dbi
	writel(0, hw->ctl_base + PCIE_SLV_AR_MISC);

	ret = dw_pcie_write(va_cfg_base + where, size, val);

	return ret;
}


static int ftpciesnps330_pcie_link_up(struct dw_pcie *pci)
{
	struct ftpciesnps330_pcie *hw = to_ftpciesnps330_pcie(pci);
	u32 val;

	val = (readl(hw->ctl_base + PCIE_SIDEBAND72) >> 4) & 0x3f;

	return val == 0x11 ? 1 : 0;
}

static void pcie_phy_writel(u16 val, u32 base,u16 timing_offset)
{
	writel(val<<16, base + PCIE_PHY_DATA_OFFSET);									//set[31:16] cr_wdata = 0x0402  (Set CM_TIME_OVRD_EN = 1, CM_TIME_OVRD = 1)	
	writel(timing_offset<<16|0x2c04, base + PCIE_PHY_DATA_CONTROL_OFFSET);		//set[31:16] cr_addr = 0x1011 [9:8] cr_cmd = 2'b00 (clr)
	writel(timing_offset<<16|0x2e04, base + PCIE_PHY_DATA_CONTROL_OFFSET);		//set[31:16] cr_addr = 0x1011 [9:8] cr_cmd = 2'b10 (wr)
}

static void ftpciesnps330_pcie_phy_init(struct pcie_port *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct ftpciesnps330_pcie *hw = to_ftpciesnps330_pcie(pci);
	u32 val;
	u16 val16;
	
//	use phy api to configure  :  ex: pcie_phy_writel(0x00000103,	hw->ctl_base, 0x101b);		//example : for lan0

}

 int ftpciesnps330_pcie_host_init(struct pcie_port *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct ftpciesnps330_pcie *hw = to_ftpciesnps330_pcie(pci);
	unsigned int val;



	writel(readl(hw->ctl_base + PCIE_SIDEBAND02)|0xc0000000, hw->ctl_base + PCIE_SIDEBAND02);

	val = readl(hw->ctl_base + PCIE_SIDEBAND93) & BIT31;

	while(val !=BIT31)
		val = readl(hw->ctl_base + PCIE_SIDEBAND93) & BIT31;	

	writel(0x00000004, hw->ctl_base + PCIE_SIDEBAND40);
	writel(0x00000044, hw->ctl_base + PCIE_SIDEBAND59);
	writel(0x04020000, hw->ctl_base + 0x44);
	writel(0x10112c04, hw->ctl_base + PCIE_SIDEBAND01);
	writel(0x10112e04, hw->ctl_base + PCIE_SIDEBAND00);
	writel(0x10112c04, hw->ctl_base + PCIE_SIDEBAND00);
	writel(0x10112f04, hw->ctl_base + PCIE_SIDEBAND00);


	// setup pcie phy ,nothing to set , can ignore
//	ftpciesnps330_pcie_phy_init(pp);

	// wait to link up
	dw_pcie_wait_for_link(pci);

	dw_pcie_setup_rc(pp);


	// setup memory mapping

	writel(PCIE_SLV_DBI_ENABLE, hw->ctl_base + PCIE_SLV_AR_MISC);					//enable dbi
	writel(PCIE_SLV_DBI_ENABLE, hw->ctl_base + PCIE_SLV_AW_MISC);					//enable dbi
	ftpciesnps330_pcie_prog_outbound_atu(pci, IATU_REGION1,
				  PCIE_ATU_TYPE_MEM, pp->mem_base + 0x01000000,
				  pp->mem_base + 0x01000000, pp->mem_size);

	ftpciesnps330_pcie_prog_inbound_atu(pci, IATU_REGION0,
				  PCIE_ATU_TYPE_MEM, PHYS_OFFSET,
				  PHYS_OFFSET, SZ_1G);

	writel(0, hw->ctl_base + PCIE_SLV_AR_MISC);
	writel(0, hw->ctl_base + PCIE_SLV_AW_MISC);

	// setup msi
	if (IS_ENABLED(CONFIG_PCI_MSI))
	{
		writel(INTR_MASK_MSI, hw->ctl_base + MSI_INTR_REG);		
		dw_pcie_msi_init(pp);
	}

	return 0;
}


int ftpciesnps330_pcie_rd_own_conf(struct pcie_port *pp, int where, int size, u32 *val)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct ftpciesnps330_pcie *hw = to_ftpciesnps330_pcie(pci);
	int ret;
	
	writel(PCIE_SLV_DBI_ENABLE, hw->ctl_base + PCIE_SLV_AW_MISC);						//enable dbi
	writel(PCIE_SLV_DBI_ENABLE, hw->ctl_base + PCIE_SLV_AR_MISC);

	ret =  dw_pcie_read(pci->dbi_base + where, size, val);

	writel(0, hw->ctl_base + PCIE_SLV_AW_MISC);						//disable dbi
	writel(0, hw->ctl_base + PCIE_SLV_AR_MISC);
	if(where == 0x8)
		*val = 0x06040001;
	else if(where == 0x18)
		*val = 0x00ff0100;
	return ret;
}
int ftpciesnps330_pcie_wr_own_conf(struct pcie_port *pp,  int where, int size, u32 val)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct ftpciesnps330_pcie *hw = to_ftpciesnps330_pcie(pci);
	u32 dbi_val;
	
	writel(PCIE_SLV_DBI_ENABLE, hw->ctl_base + PCIE_SLV_AW_MISC);						//enable dbi
	writel(PCIE_SLV_DBI_ENABLE, hw->ctl_base + PCIE_SLV_AR_MISC);

	dw_pcie_write(pci->dbi_base + where, size, val);

	writel(0, hw->ctl_base + PCIE_SLV_AW_MISC);						//disable dbi
	writel(0, hw->ctl_base + PCIE_SLV_AR_MISC);
}

static struct dw_pcie_host_ops ftpciesnps330_pcie_host_ops = {
	.rd_other_conf  = ftpciesnps330_pcie_rd_other_conf,
	.wr_other_conf  = ftpciesnps330_pcie_wr_other_conf,
	.rd_own_conf  = ftpciesnps330_pcie_rd_own_conf,
	.wr_own_conf  = ftpciesnps330_pcie_wr_own_conf,
	.host_init      = ftpciesnps330_pcie_host_init,
};

u32 ftpciesnps330_pcie_read_dbi(struct dw_pcie *pci, void __iomem *base,
				u32 reg, size_t size)
{
	struct ftpciesnps330_pcie *hw = to_ftpciesnps330_pcie(pci);
	u32 val,dbi_val;
	
	writel(PCIE_SLV_DBI_ENABLE, hw->ctl_base + PCIE_SLV_AW_MISC);						//enable dbi
	writel(PCIE_SLV_DBI_ENABLE, hw->ctl_base + PCIE_SLV_AR_MISC);

	dw_pcie_read(base + reg, size, &val);

	writel(0, hw->ctl_base + PCIE_SLV_AW_MISC);						//disable dbi
	writel(0, hw->ctl_base + PCIE_SLV_AR_MISC);

	return val;
}

void ftpciesnps330_pcie_write_dbi(struct dw_pcie *pci, void __iomem *base,
				u32 reg, size_t size , u32 val)
{
	struct ftpciesnps330_pcie *hw = to_ftpciesnps330_pcie(pci);
	u32 dbi_val;
	
	writel(PCIE_SLV_DBI_ENABLE, hw->ctl_base + PCIE_SLV_AW_MISC);						//enable dbi
	writel(PCIE_SLV_DBI_ENABLE, hw->ctl_base + PCIE_SLV_AR_MISC);

	dw_pcie_write(base + reg, size, &val);

	writel(0, hw->ctl_base + PCIE_SLV_AW_MISC);						//disable dbi
	writel(0, hw->ctl_base + PCIE_SLV_AR_MISC);
}


static struct dw_pcie_ops ftpciesnps330_pcie_ops = {
	.link_up        = ftpciesnps330_pcie_link_up,
	.read_dbi	     = ftpciesnps330_pcie_read_dbi,
	.write_dbi    = ftpciesnps330_pcie_write_dbi,
};

static irqreturn_t ftpciesnps330_pcie_irq(int irq, void *arg)
{
	struct pcie_port *pp = arg;
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct ftpciesnps330_pcie *hw = to_ftpciesnps330_pcie(pci);
	u32 intr_sts, intr_msk;

	intr_sts = readl(hw->ctl_base + MSI_INTR_REG);

	writel(intr_sts, hw->ctl_base + MSI_INTR_REG);

	if (intr_sts & INTR_STATUS_MSI)
		return dw_handle_msi_irq(pp);

	return IRQ_HANDLED;
}

static int __init ftpciesnps330_pcie_probe(struct platform_device *pdev)
{
	struct ftpciesnps330_pcie *hw;
	struct dw_pcie *pci;
	struct pcie_port *pp;
	struct resource *ctl_base;
//	struct resource *dbi_base;
	struct resource *phy_base;
	struct clk *clk;
	struct reset_control *rstc;
	int ret;

	hw = devm_kzalloc(&pdev->dev, sizeof(*hw),
	                  GFP_KERNEL);
	if (!hw)
		return -ENOMEM;

	pci = &hw->pci;

	pci->dev = &pdev->dev;
	pci->ops = &ftpciesnps330_pcie_ops;

	clk = devm_clk_get(&pdev->dev, "pclk");
	if (!IS_ERR(clk)) {
		clk_prepare_enable(clk);
	}
	clk = devm_clk_get(&pdev->dev, "aclk");
	if (!IS_ERR(clk)) {
		clk_prepare_enable(clk);
	}
	clk = devm_clk_get(&pdev->dev, "slv_clk");
	if (!IS_ERR(clk)) {
		clk_prepare_enable(clk);
	}

	ctl_base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hw->ctl_base = devm_ioremap_resource(&pdev->dev, ctl_base);
	if (IS_ERR(hw->ctl_base)) {
		ret = PTR_ERR(hw->ctl_base);
		goto map_failed;
	}

	pp = &pci->pp;

	pp->irq = platform_get_irq(pdev, 0);
	if (!pp->irq) {
		dev_err(&pdev->dev, "failed to get irq\n");
		return -ENODEV;
	}
	ret = devm_request_irq(&pdev->dev, pp->irq, ftpciesnps330_pcie_irq,
	                       IRQF_SHARED, "ftpciesnps330-pcie", pp);
	if (ret) {
		dev_err(&pdev->dev, "failed to request irq\n");
		return ret;
	}

	pp->root_bus_nr = -1;
	pp->ops = &ftpciesnps330_pcie_host_ops;

	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(&pdev->dev, "failed to initialize host\n");
		return ret;
	}

	platform_set_drvdata(pdev, hw);

	dev_info(&pdev->dev, "version %s\n", DRIVER_VERSION);

	return 0;

map_failed:
	devm_kfree(&pdev->dev, hw);
	dev_err(&pdev->dev, "Faraday FTPCIESNPS330 probe failed\n");

	return ret;
}

static int __exit ftpciesnps330_pcie_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id ftpciesnps330_pcie_of_match[] = {
	{ .compatible = "faraday,ftpciesnps330", },
	{},
};
MODULE_DEVICE_TABLE(of, ftpciesnps330_pcie_of_match);

static struct platform_driver ftpciesnps330_pcie_driver = {
	.remove		= __exit_p(ftpciesnps330_pcie_remove),
	.driver = {
		.name	= "ftpciesnps330",
		.of_match_table = ftpciesnps330_pcie_of_match,
	},
};

/* Exynos PCIe driver does not allow module unload */

static int __init ftpciesnps330_pcie_init(void)
{
	return platform_driver_probe(&ftpciesnps330_pcie_driver, ftpciesnps330_pcie_probe);
}
subsys_initcall(ftpciesnps330_pcie_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Fu-Tsung Hsu <mark_hs@faraday-tech.com>");
MODULE_DESCRIPTION("Faraday FTPCIESNPS330 PCIE Controller");
