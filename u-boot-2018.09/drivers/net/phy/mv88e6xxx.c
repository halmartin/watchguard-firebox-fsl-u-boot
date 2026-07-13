/*
 * Copyright (C) 2018 Marvell International Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0
 * https://spdx.org/licenses
 */

#include <mv88e6xxx.h>
/* If the switch's ADDR[4:0] strap pins are strapped to zero, it will
 * use all 32 SMI bus addresses on its SMI bus, and all switch registers
 * will be directly accessible on some {device address,register address}
 * pair.  If the ADDR[4:0] pins are not strapped to zero, the switch
 * will only respond to SMI transactions to that specific address, and
 * an indirect addressing mechanism needs to be used to access its
 * registers.
 */

static struct mv88e6xxx_dev soho_dev;
static struct mv88e6xxx_dev *soho_dev_handle;
static int REG_PORT_BASE = REG_PORT_BASE_UNDEFINED;

static int mv88e6xxx_reg_wait_ready(struct mv88e6xxx_dev *dev)
{
	int ret;
	int i;
	int loop_timeout = 16;
	unsigned short val;
	const char *name = miiphy_get_current_dev();

	if (!name)
		return -ENXIO;

	for (i = 0; i < loop_timeout; i++) {
		ret = miiphy_read(name, dev->phy_addr, SMI_CMD, &val);
		if (ret < 0)
			return ret;
		if ((val & SMI_CMD_BUSY) == 0)
			return 0;
	}

	return -ETIMEDOUT;
}

int mv88e6xxx_read_register(struct mv88e6xxx_dev *dev, int port, int reg)
{
	int ret;
	unsigned short val;
	const char *name = miiphy_get_current_dev();

	if (!name)
		return -ENXIO;

	if (!dev)
		return -ENODEV;

	if (dev->addr_mode == 0) {
		ret = miiphy_read(name, port, reg, &val);
		if (ret < 0)
			return ret;
		else
			return (int)val;
	}

	/* Wait for the bus to become free. */
	ret = mv88e6xxx_reg_wait_ready(dev);
	if (ret < 0)
		return ret;

	/* Transmit the read command. */
	ret = miiphy_write(
		name, dev->phy_addr, SMI_CMD,
		SMI_CMD_OP_22_READ |
		((port & SMI_CMD_DEV_ADDR_MASK) << SMI_CMD_DEV_ADDR_SIZE) |
		(reg & SMI_CMD_REG_ADDR_MASK));
	if (ret < 0)
		return ret;

	/* Wait for the read command to complete. */
	ret = mv88e6xxx_reg_wait_ready(dev);
	if (ret < 0)
		return ret;

	/* Read the data. */
	ret = miiphy_read(name, dev->phy_addr, SMI_DATA, &val);
	if (ret < 0)
		return ret;

	return (int)val;
}

int mv88e6xxx_write_register(struct mv88e6xxx_dev *dev, int port, int reg,
			     unsigned short val)
{
	int ret;
	const char *name = miiphy_get_current_dev();

	if (!name)
		return -ENXIO;

	if (!dev)
		return -ENODEV;

	if (dev->addr_mode == 0)
		return miiphy_write(name, port, reg, val);

	/* Wait for the bus to become free. */
	ret = mv88e6xxx_reg_wait_ready(dev);
	if (ret < 0)
		return ret;

	/* Transmit data to write. */
	ret = miiphy_write(name, dev->phy_addr, SMI_DATA, val);
	if (ret < 0)
		return ret;

	/* Transmit the write command. */
	ret = miiphy_write(
		name, dev->phy_addr, SMI_CMD,
		SMI_CMD_OP_22_WRITE |
		((port & SMI_CMD_DEV_ADDR_MASK) << SMI_CMD_DEV_ADDR_SIZE) |
		(reg & SMI_CMD_REG_ADDR_MASK));
	if (ret < 0)
		return ret;

	/* Wait for the read command to complete. */
	ret = mv88e6xxx_reg_wait_ready(dev);
	if (ret < 0)
		return ret;

	return 0;
}

static int mv88e6xxx_reg_wait_ready_indirect(struct mv88e6xxx_dev *dev)
{
	int ret, i;
	int loop_timeout = 16;

	for (i = 0; i < loop_timeout; i++) {
		ret = mv88e6xxx_read_register(dev, REG_GLOBAL2, GLOBAL2_SMI_OP);
		if (ret < 0)
			return ret;
		if (!(ret & GLOBAL2_SMI_OP_BUSY))
			return 0;
	}
	return -ETIMEDOUT;
}

static int mv88e6xxx_read_indirect(struct mv88e6xxx_dev *dev, int port, int reg)
{
	int ret;

	ret = mv88e6xxx_write_register(
		dev, REG_GLOBAL2, GLOBAL2_SMI_OP,
		GLOBAL2_SMI_OP_22_READ |
		((port & SMI_CMD_DEV_ADDR_MASK) << SMI_CMD_DEV_ADDR_SIZE) |
		(reg & SMI_CMD_REG_ADDR_MASK));
	if (ret < 0)
		return ret;

	ret = mv88e6xxx_reg_wait_ready_indirect(dev);
	if (ret < 0)
		return ret;

	ret = mv88e6xxx_read_register(dev, REG_GLOBAL2, GLOBAL2_SMI_DATA);

	return ret;
}

static int mv88e6xxx_write_indirect(struct mv88e6xxx_dev *dev, int port,
				    int reg, unsigned short val)
{
	int ret;

	ret = mv88e6xxx_write_register(dev, REG_GLOBAL2, GLOBAL2_SMI_DATA, val);
	if (ret < 0)
		return ret;

	ret = mv88e6xxx_write_register(
		dev, REG_GLOBAL2, GLOBAL2_SMI_OP,
		GLOBAL2_SMI_OP_22_WRITE |
		((port & SMI_CMD_DEV_ADDR_MASK) << SMI_CMD_DEV_ADDR_SIZE) |
		(reg & SMI_CMD_REG_ADDR_MASK));

	return mv88e6xxx_reg_wait_ready_indirect(dev);
}

static int mv88e6xxx_read_indirect45(struct mv88e6xxx_dev *dev, int port, int dev_id,int reg)
{
	int ret;

	ret = mv88e6xxx_write_register(dev, REG_GLOBAL2, GLOBAL2_SMI_DATA, reg);
	if (ret < 0)
		return ret;

	ret = mv88e6xxx_write_register(
		dev, REG_GLOBAL2, GLOBAL2_SMI_OP,
		GLOBAL2_SMI_OP_45_WRITE_ADDR |
		((port & SMI_CMD_DEV_ADDR_MASK) << SMI_CMD_DEV_ADDR_SIZE) |
		(dev_id & SMI_CMD_REG_ADDR_MASK));
	if (ret < 0)
		return ret;

	ret = mv88e6xxx_reg_wait_ready_indirect(dev);
	if (ret < 0)
		return ret;

	ret = mv88e6xxx_write_register(
		dev, REG_GLOBAL2, GLOBAL2_SMI_OP,
		GLOBAL2_SMI_OP_45_READ_DATA |
		((port & SMI_CMD_DEV_ADDR_MASK) << SMI_CMD_DEV_ADDR_SIZE) |
		(dev_id & SMI_CMD_REG_ADDR_MASK));

	ret = mv88e6xxx_reg_wait_ready_indirect(dev);
	if (ret < 0)
		return ret;
	ret = mv88e6xxx_read_register(dev, REG_GLOBAL2, GLOBAL2_SMI_DATA);

	return ret;
}

static int mv88e6xxx_write_indirect45(struct mv88e6xxx_dev *dev, int port,
				    int dev_id,int reg, unsigned short val)
{
	int ret;
	/*Write Addr first*/
	ret = mv88e6xxx_write_register(dev, REG_GLOBAL2, GLOBAL2_SMI_DATA, reg);
	if (ret < 0)
		return ret;

	ret = mv88e6xxx_write_register(
		dev, REG_GLOBAL2, GLOBAL2_SMI_OP,
		GLOBAL2_SMI_OP_45_WRITE_ADDR |
		((port & SMI_CMD_DEV_ADDR_MASK) << SMI_CMD_DEV_ADDR_SIZE) |
		(dev_id & SMI_CMD_REG_ADDR_MASK));
	/*Write Addr first*/
	if(mv88e6xxx_reg_wait_ready_indirect(dev) == 0)
	{
		/*Write value*/
		ret = mv88e6xxx_write_register(dev, REG_GLOBAL2, GLOBAL2_SMI_DATA, val);
			if (ret < 0)
				return ret;
		ret = mv88e6xxx_write_register(
				dev, REG_GLOBAL2, GLOBAL2_SMI_OP,
				GLOBAL2_SMI_OP_45_WRITE_DATA |
				((port & SMI_CMD_DEV_ADDR_MASK) << SMI_CMD_DEV_ADDR_SIZE) |
				(dev_id & SMI_CMD_REG_ADDR_MASK));
		if (ret < 0)
				return ret;
	}
	 return ret;

}

int mv88e6xxx_read_phy_register(struct mv88e6xxx_dev *dev, int port, int page,
				int reg)
{
	int ret;

	if (!dev)
		return -ENODEV;

	ret = mv88e6xxx_write_indirect(dev, port, SMI_PHY_PAGE_REG, page);
	if (ret < 0)
		goto restore_page_0;

	ret = mv88e6xxx_read_indirect(dev, port, reg);

restore_page_0:
	mv88e6xxx_write_indirect(dev, port, SMI_PHY_PAGE_REG, 0x0);

	return ret;
}

int mv88e6xxx_write_phy_register(struct mv88e6xxx_dev *dev, int port, int page,
				 int reg, unsigned short val)
{
	int ret;

	if (!dev) {
		printf("Soho dev not initialized\n");
		return -1;
	}

	ret = mv88e6xxx_write_indirect(dev, port, SMI_PHY_PAGE_REG, page);
	if (ret < 0)
		goto restore_page_0;

	ret = mv88e6xxx_write_indirect(dev, port, reg, val);

restore_page_0:
	mv88e6xxx_write_indirect(dev, port, SMI_PHY_PAGE_REG, 0x0);

	return ret;
}

/* Set power up/down for SGMII and 1000Base-X */
static int mv88e6390_serdes_power_sgmii(struct mv88e6xxx_dev *dev, int port,
					bool up)
{
	u16 val, new_val;
	int ret;

	val = mv88e6xxx_read_indirect45(dev, port, MDIO_MMD_PHYXS,
				    MV88E6390_SGMII_BMCR);
	if (val < 0)
		return val;

	if (up)
		new_val = val & ~(BMCR_RESET | BMCR_LOOPBACK | BMCR_PDOWN);
	else
		new_val = val | BMCR_PDOWN;

	if (val != new_val)
		ret = mv88e6xxx_write_indirect45(dev, port, MDIO_MMD_PHYXS,
					     MV88E6390_SGMII_BMCR, new_val);

	if (ret < 0)
		return ret;

	return 0;
}

static int mv88e6393x_serdes_power_lane(struct mv88e6xxx_dev *dev, int port,
					bool on)
{
	int ret;

	ret = mv88e6xxx_read_indirect45(dev, port, MDIO_MMD_PHYXS,
				    MV88E6393X_SERDES_CTRL1);
	if (ret < 0)
		return ret;

	if (on)
		ret &= ~(MV88E6393X_SERDES_CTRL1_TX_PDOWN |
			 MV88E6393X_SERDES_CTRL1_RX_PDOWN);
	else
		ret |= MV88E6393X_SERDES_CTRL1_TX_PDOWN |
		       MV88E6393X_SERDES_CTRL1_RX_PDOWN;

	return mv88e6xxx_write_indirect45(dev, port, MDIO_MMD_PHYXS,
				      MV88E6393X_SERDES_CTRL1, (unsigned short)ret);
}

static int mv88e6393x_serdes_erratum_4_6(struct mv88e6xxx_dev *dev, int port)
{
	int ret;

	/* mv88e6393x family errata 4.6:
	 * Cannot clear PwrDn bit on SERDES if device is configured CPU_MGD
	 * mode or P0_mode is configured for [x]MII.
	 * Workaround: Set SERDES register 4.F002 bit 5=0 and bit 15=1.
	 *
	 * It seems that after this workaround the SERDES is automatically
	 * powered up (the bit is cleared), so power it down.
	 */
	ret = mv88e6xxx_read_indirect45(dev, port, MDIO_MMD_PHYXS,
				    MV88E6393X_SERDES_POC);
	if (ret < 0)
		return ret;

	ret &= ~MV88E6393X_SERDES_POC_PDOWN;
	ret |= MV88E6393X_SERDES_POC_RESET;

	ret = mv88e6xxx_write_indirect45(dev, port, MDIO_MMD_PHYXS,
				     MV88E6393X_SERDES_POC,(unsigned short)ret);
	if (ret < 0)
		return ret;

	ret = mv88e6390_serdes_power_sgmii(dev, port, false);
	if (ret < 0)
		return ret;

	return mv88e6393x_serdes_power_lane(dev, port, false);

}

static int mv88e6393x_serdes_setup_errata(struct mv88e6xxx_dev *dev)
{
	int err;

	err = mv88e6393x_serdes_erratum_4_6(dev, MV88E6393X_PORT0_LANE);
	if (err < 0)
		return err;

	err = mv88e6393x_serdes_erratum_4_6(dev, MV88E6393X_PORT9_LANE);
	if (err < 0)
		return err;

	return mv88e6393x_serdes_erratum_4_6(dev, MV88E6393X_PORT10_LANE);
}

static int mv88e6393x_serdes_erratum_4_8(struct mv88e6xxx_dev *dev, int port)
{
	u16 reg, pcs;
	int ret;

	/* mv88e6393x family errata 4.8:
	 * When a SERDES port is operating in 1000BASE-X or SGMII mode link may
	 * not come up after hardware reset or software reset of SERDES core.
	 * Workaround is to write SERDES register 4.F074.14=1 for only those
	 * modes and 0 in all other modes.
	 */
	pcs = mv88e6xxx_read_indirect45(dev, port, MDIO_MMD_PHYXS,
				    MV88E6393X_SERDES_POC);
	if (pcs < 0)
		return pcs;

	pcs &= MV88E6393X_SERDES_POC_PCS_MASK;

	reg = mv88e6xxx_read_indirect45(dev, port, MDIO_MMD_PHYXS,
				    MV88E6393X_ERRATA_4_8_REG);
	if (reg < 0)
		return reg;

	if (pcs == MV88E6393X_SERDES_POC_PCS_1000BASEX ||
	    pcs == MV88E6393X_SERDES_POC_PCS_SGMII_PHY ||
	    pcs == MV88E6393X_SERDES_POC_PCS_SGMII_MAC)
		reg |= MV88E6393X_ERRATA_4_8_BIT;
	else
		reg &= ~MV88E6393X_ERRATA_4_8_BIT;

	return mv88e6xxx_write_indirect45(dev, port, MDIO_MMD_PHYXS,
				      MV88E6393X_ERRATA_4_8_REG, reg);
}


static int mv88e6393x_serdes_fix_2500basex_an(struct mv88e6xxx_dev *dev,
					      int port, u8 cmode, bool on)
{
	int reg;
	int err;

	if (cmode != MV88E6XXX_PORT_STS_CMODE_2500BASEX)
		return 0;

	/* Inband AN is broken on Amethyst in 2500base-x mode when set by
	 * standard mechanism (via cmode).
	 * We can get around this by configuring the PCS mode to 1000base-x
	 * and then writing value 0x58 to register 1e.8000. (This must be done
	 * while SerDes receiver and transmitter are disabled, which is, when
	 * this function is called.)
	 * It seem that when we do this configuration to 2500base-x mode (by
	 * changing PCS mode to 1000base-x and frequency to 3.125 GHz from
	 * 1.25 GHz) and then configure to sgmii or 1000base-x, the device
	 * thinks that it already has SerDes at 1.25 GHz and does not change
	 * the 1e.8000 register, leaving SerDes at 3.125 GHz.
	 * To avoid this, change PCS mode back to 2500base-x when disabling
	 * SerDes from 2500base-x mode.
	 */
	reg = mv88e6xxx_read_indirect45(dev, port, MDIO_MMD_PHYXS,
				    MV88E6393X_SERDES_POC);
	if (reg < 0)
		return reg;

	reg &= ~(MV88E6393X_SERDES_POC_PCS_MASK | MV88E6393X_SERDES_POC_AN);
	if (on)
		reg |= MV88E6393X_SERDES_POC_PCS_1000BASEX |
		       MV88E6393X_SERDES_POC_AN;
	else
		reg |= MV88E6393X_SERDES_POC_PCS_2500BASEX;
	reg |= MV88E6393X_SERDES_POC_RESET;

	err = mv88e6xxx_write_indirect45(dev, port, MDIO_MMD_PHYXS,
				     MV88E6393X_SERDES_POC, (unsigned short)reg);
	if (err < 0)
		return err;

	err = mv88e6xxx_write_indirect45(dev, port, MDIO_MMD_VEND1, 0x8000, 0x58);
	if (err < 0)
		return err;

	return 0;
}

static int mv88e6xxx_port_get_cmode(struct mv88e6xxx_dev *dev, int port, u8 *cmode)
{
	int reg;

	reg = mv88e6xxx_read_register(dev, port, PORT_STATUS);
	if (reg < 0)
		return reg;

	*cmode = reg & MV88E6XXX_PORT_STS_CMODE_MASK;

	return 0;
}


int mv88e6393x_serdes_power(struct mv88e6xxx_dev *dev, int port,
			    bool on)
{
	u8 cmode;
	int err;

	if (port != 0 && port != 9 && port != 10)
		return -EOPNOTSUPP;

	err = mv88e6xxx_port_get_cmode(dev, port, &cmode);
	if (err < 0)
		return err;

	if (on) {
		err = mv88e6393x_serdes_erratum_4_8(dev, port);
		if (err < 0)
			return err;
		err = mv88e6393x_serdes_power_lane(dev, port, true);
		if (err < 0)
			return err;
	}

	switch (cmode) {
	case MV88E6XXX_PORT_STS_CMODE_SGMII:
	case MV88E6XXX_PORT_STS_CMODE_1000BASEX:
	case MV88E6XXX_PORT_STS_CMODE_2500BASEX:
		err = mv88e6390_serdes_power_sgmii(dev, port, on);
		break;
	default:
		err = -EINVAL;
		break;
	}

	if (err < 0)
		return err;

	if (!on) {
		err = mv88e6393x_serdes_power_lane(dev, port, false);
		if (err < 0)
			return err;
	}

	return err;
}


/* Offset 0x16: LED Control Register */
int mv88e6xxx_port_set_led_control(struct mv88e6xxx_dev *dev, int port,
				   u16 led_control)
{
	led_control &= MV88E6XXX_PORT_LED_CONTROL_DATA_MASK;
	led_control |= MV88E6XXX_PORT_LED_CONTROL_POINTER_CONTROL_LED01
		    |  MV88E6XXX_PORT_LED_CONTROL_UPDATE;

	return mv88e6xxx_write_register(dev, port, MV88E6XXX_PORT_LED_CONTROL,
				    led_control);
}

void mv88e6xxx_display_switch_info(struct mv88e6xxx_dev *dev)
{
	unsigned int product_num;

	if (dev->id < 0) {
		printf("No Switch Device Found\n");
		return;
	}

	product_num = ((unsigned int)dev->id) >> 4;
	if (product_num == PORT_SWITCH_ID_PROD_NUM_6390 ||
	    product_num == PORT_SWITCH_ID_PROD_NUM_6390X ||
	    product_num == PORT_SWITCH_ID_PROD_NUM_6290 ||
	    product_num == PORT_SWITCH_ID_PROD_NUM_6190) {
		printf("Switch    : SOHO\n");
		printf("Series    : Peridot\n");
		printf("Product # : %X\n", product_num);
		printf("Revision  : %X\n", dev->id & 0xf);
		if (dev->cpu_port != -1)
			printf("Cpu port  : %d\n", dev->cpu_port);
	} else if (product_num == PORT_SWITCH_ID_PROD_NUM_6141 ||
		   product_num == PORT_SWITCH_ID_PROD_NUM_6341) {
		printf("Switch    : SOHO\n");
		printf("Series    : Topaz\n");
		printf("Product # : %X\n", product_num);
		printf("Revision  : %X\n", dev->id & 0xf);
		if (dev->cpu_port != -1)
			printf("Cpu port  : %d\n", dev->cpu_port);
	} else if (product_num == PORT_SWITCH_ID_PROD_NUM_6191X ||
		   product_num == PORT_SWITCH_ID_PROD_NUM_6193X ||
		   product_num == PORT_SWITCH_ID_PROD_NUM_6361 ||
		   product_num == PORT_SWITCH_ID_PROD_NUM_6393X) {
		printf("Switch    : SOHO\n");
		printf("Series    : Amethyst\n");
		printf("Product # : %X\n", product_num);
		printf("Revision  : %X\n", dev->id & 0xf);
		if (dev->cpu_port != -1)
			printf("Cpu port  : %d\n", dev->cpu_port);
	} else {
		printf("Unknown switch with Device ID: 0x%X\n", dev->id);
	}
}

/* We expect the switch to perform auto negotiation if there is a real phy. */
int mv88e6xxx_get_link_status(struct mv88e6xxx_dev *dev, int port)
{
	int ret;

	ret = mv88e6xxx_read_register(dev, REG_PORT(port), PORT_STATUS);
	if (ret < 0)
		return ret;

	printf("Port: 0x%X, ", port);
	if (ret & PORT_STATUS_LINK) {
		printf("Link: UP, ");
	} else {
		printf("Link: Down\n");
		return 0;
	}

	if (ret & PORT_STATUS_DUPLEX)
		printf("Duplex: FULL, ");
	else
		printf("Duplex: HALF, ");

	if ((ret & PORT_STATUS_SPEED_MASK) == PORT_STATUS_SPEED_10)
		printf("Speed: 10 Mbps\n");
	else if ((ret & PORT_STATUS_SPEED_MASK) == PORT_STATUS_SPEED_100)
		printf("Speed: 100 Mbps\n");
	else if ((ret & PORT_STATUS_SPEED_MASK) == PORT_STATUS_SPEED_1000)
		printf("Speed: 1000 Mbps\n");
	else if ((ret & PORT_STATUS_SPEED_MASK) == PORT_STATUS_SPEED_2500_10G)
		printf("Speed: 10 Gb or 2500 Mbps\n");
	else
		printf("Speed: Unknown\n");

	return 0;
}

int mv88e6xxx_get_switch_id(struct mv88e6xxx_dev *dev)
{
	int id, product_num;

	/* Peridot switch port device address starts from 0
	 * Legacy switch port device address starts from 0x10
	 *
	 * In order to determine which switch is used, we need to
	 * read the ID, but inorder to read the ID, we need to know
	 * the port device address - classic chicken or the egg case.
	 *
	 * Let's read with both port device addresses, if we get 0xFFFF,
	 * the address is incorrect and we need to ready with the second
	 * address.
	 */
	id = mv88e6xxx_read_register(dev, REG_PORT_BASE_LEGACY, PORT_SWITCH_ID);
	if (id == 0xFFFF)
		id = mv88e6xxx_read_register(dev, REG_PORT_BASE_PERIDOT,
					     PORT_SWITCH_ID);

	if (id < 0)
		return id;

	product_num = id >> 4;
	if ((product_num == PORT_SWITCH_ID_PROD_NUM_6190) ||
	    (product_num == PORT_SWITCH_ID_PROD_NUM_6290) ||
	    (product_num == PORT_SWITCH_ID_PROD_NUM_6390) ||
	    (product_num == PORT_SWITCH_ID_PROD_NUM_6390X)) {
		/* Peridot switch port device address starts from 0 */
		REG_PORT_BASE = REG_PORT_BASE_PERIDOT;
		return id;
	} else if (product_num == PORT_SWITCH_ID_PROD_NUM_6141 ||
		   product_num == PORT_SWITCH_ID_PROD_NUM_6341) {
		/* Legacy switch port device address starts from 0x10 */
		REG_PORT_BASE = REG_PORT_BASE_LEGACY;
		return id;
	} else if (product_num == PORT_SWITCH_ID_PROD_NUM_6191X ||
		   product_num == PORT_SWITCH_ID_PROD_NUM_6193X ||
		   product_num == PORT_SWITCH_ID_PROD_NUM_6361 ||
		   product_num == PORT_SWITCH_ID_PROD_NUM_6393X) {
		/* Amethyst switch port device address starts from 0 */
		REG_PORT_BASE = REG_PORT_BASE_AMETHYST;
		return id;
	} else {
		return -ENODEV;
	}

	return 0;
}

int mv88e6xxx_initialize(const void *blob)
{
	int node = 0;
	int ret;
	int port;
	int isolated_port_cnt = 0;

	soho_dev_handle = NULL;

	/* Read Device Tree */
	node = fdt_node_offset_by_compatible(blob, -1, "marvell,mv88e6xxx");

	if (node == -FDT_ERR_NOTFOUND)
		return -ENXIO;

	/* Check whether switch node is enabled */
	ret = fdtdec_get_is_enabled(blob, node);
	if (ret == 0)
		return -EACCES;

	/* Initizalize Switch Device Structure */
	soho_dev.phy_addr = fdtdec_get_uint(blob, node, "phy-addr", 0);
	soho_dev.port_mask = fdtdec_get_int(blob, node, "port-mask", 0);
	if (soho_dev.phy_addr == 0)
		soho_dev.addr_mode = 0;  /* Single Addressing mode */
	else
		soho_dev.addr_mode = 1;  /* Multi Addressing mode */

	soho_dev.id = mv88e6xxx_get_switch_id(&soho_dev);

	soho_dev.led_mode = fdtdec_get_int(blob, node, "led-mode", 0);
	soho_dev.fix_port = fdtdec_get_int(blob, node, "fix-port", -1);
	soho_dev.cpu_port = fdtdec_get_int(blob, node, "cpu-port", -1);
	if (soho_dev.cpu_port != -1) {
		u16 reg;

		/* For 88e6390X switch we need to configure C_MODE field
		 * in Port Status Register to 0xb (2500 base-x).
		 * NOTE: Port Status Register is generally RO, but it can
		 * be written for port9 and port10 (cpu ports).
		 */
		if ((soho_dev.id >> 4) == PORT_SWITCH_ID_PROD_NUM_6390X) {
			reg = mv88e6xxx_read_register(
				&soho_dev, REG_PORT(soho_dev.cpu_port),
				PORT_STATUS);
			reg &= ~PORT_STATUS_CMODE_MASK;
			reg |= PORT_STATUS_CMODE_2500BASE_X;
			ret = mv88e6xxx_write_register(
				&soho_dev, REG_PORT(soho_dev.cpu_port),
				PORT_STATUS, reg);
		} else {
			reg = mv88e6xxx_read_register(
				&soho_dev, REG_PORT(soho_dev.cpu_port),
				PORT_PCS_CTRL);
			/* CPU port is forced link-up, duplex and 1GB speed */
			reg &= ~PORT_PCS_CTRL_UNFORCED;
			reg |= PORT_PCS_CTRL_FORCE_LINK |
			       PORT_PCS_CTRL_LINK_UP |
			       PORT_PCS_CTRL_DUPLEX_FULL |
			       PORT_PCS_CTRL_FORCE_DUPLEX |
				   PORT_PCS_CTRL_RGMII_DELAY_TXCLK |
				   PORT_PCS_CTRL_RGMII_DELAY_RXCLK |
			       PORT_PCS_CTRL_1000;

			if ((soho_dev.id >> 4) ==
			    PORT_SWITCH_ID_PROD_NUM_6341) {
				/* Configure RGMII Delay on cpu port */
				reg |= PORT_PCS_CTRL_FORCE_SPEED |
				       PORT_PCS_CTRL_RGMII_DELAY_TXCLK |
				       PORT_PCS_CTRL_RGMII_DELAY_RXCLK;
			}

			ret = mv88e6xxx_write_register(
				&soho_dev, REG_PORT(soho_dev.cpu_port),
				PORT_PCS_CTRL, reg);
		}

		if (ret)
			return ret;
	}

	/* Force port setup */
	for (port = 0; port < sizeof(soho_dev.port_mask) * 8; port++) {
		if (!(soho_dev.port_mask & BIT(port)))
			continue;

		/* Set port control register */
		mv88e6xxx_write_register(&soho_dev,
					 REG_PORT(port),
					 PORT_CONTROL,
					 PORT_CONTROL_STATE_FORWARDING |
					 PORT_CONTROL_FORWARD_UNKNOWN |
					 PORT_CONTROL_FORWARD_UNKNOWN_MC |
					 PORT_CONTROL_USE_TAG |
					 PORT_CONTROL_USE_IP |
					 PORT_CONTROL_TAG_IF_BOTH);
		/* Set port based vlan table */
		if ((port >= 5) && (port <=7 )) {
			/* FBX-23340: Traffic can pass over LAN ports successfully when boot into NV5 into WG uboot */
			/* Isolate port 5, 6, and 7. Disallow to send frames to other two ports. */
			mv88e6xxx_write_register(&soho_dev,
						 REG_PORT(port),
						 PORT_BASE_VLAN,
						 soho_dev.port_mask & ~(BIT(5)|BIT(6)|BIT(7)));
			isolated_port_cnt ++;
		} else {
			mv88e6xxx_write_register(&soho_dev,
						 REG_PORT(port),
						 PORT_BASE_VLAN,
						 soho_dev.port_mask & ~BIT(port));
		}

		if (port == soho_dev.cpu_port || port == soho_dev.fix_port)
			continue;

		/* Set phy copper control for lan ports */
		mv88e6xxx_write_phy_register(&soho_dev,
					     REG_PORT(port),
					     0,
					     PHY_COPPER_CONTROL,
					     PHY_COPPER_CONTROL_SPEED_1G |
					     PHY_COPPER_CONTROL_DUPLEX |
					     PHY_COPPER_CONTROL_AUTO_NEG_EN);
		
		/* Set led for lan ports */
		mv88e6xxx_port_set_led_control(&soho_dev,REG_PORT(port),soho_dev.led_mode);
	} /* End of for (port = 0; port < sizeof(soho_dev.port_mask) * 8; port++) { */

	if (isolated_port_cnt) {
		printf("Isolate %d slave port(s). Disallow to send frames to other %d slave port(s).\n", isolated_port_cnt, (isolated_port_cnt-1));
	}

	/* configure SERDES/port 10 for BP1 */
	printf("configure SERDES/port 10 for BP1 \n");
	if (((soho_dev.id >> 4) == PORT_SWITCH_ID_PROD_NUM_6191X) ||
		((soho_dev.id >> 4) == PORT_SWITCH_ID_PROD_NUM_6193X) ||
		((soho_dev.id >> 4) == PORT_SWITCH_ID_PROD_NUM_6361)  ||
		((soho_dev.id >> 4) == PORT_SWITCH_ID_PROD_NUM_6393X)){
		/*Power Off SERDES*/
		mv88e6xxx_write_indirect45(&soho_dev,REG_PORT(10),4,0x2000,0x1940);

		/*Change Port10 C_MODE to SGMII*/	
		mv88e6xxx_write_register(&soho_dev, REG_PORT(10),PORT_STATUS, 0xa);

		/*Errata 4.8*/	
		mv88e6xxx_write_indirect45(&soho_dev,REG_PORT(10),4,0xF074,0x6000);

		/*Set SGMII PHY mode*/	
		mv88e6xxx_write_indirect45(&soho_dev,REG_PORT(10),4,0xF002,0xA00A);

		/*Force Speed Force EEE Off*/
		mv88e6xxx_write_register(&soho_dev, REG_PORT(10),PORT_PCS_CTRL, 0x2102);

		/*Power on SERDES core*/
		mv88e6xxx_write_indirect45(&soho_dev,REG_PORT(10),4,0x2000,0x9140);
	}
	soho_dev_handle = &soho_dev;
	printf("mv88e6xxx_initialize done\n");
	return 0;
}

static int sw_resolve_options(char *str)
{
	if (strcmp(str, "info") == 0)
		return SW_INFO;
	else if (strcmp(str, "read") == 0)
		return SW_READ;
	else if (strcmp(str, "write") == 0)
		return SW_WRITE;
	else if (strcmp(str, "phy_read") == 0)
		return SW_PHY_READ;
	else if (strcmp(str, "phy_write") == 0)
		return SW_PHY_WRITE;
	else if (strcmp(str, "link") == 0)
		return SW_LINK;
	else if (strcmp(str, "dev_write") == 0)
		return SW_C45_WRITE;
	else if (strcmp(str, "dev_read") == 0)
		return SW_C45_READ;
	else
		return SW_NA;
}

static int do_sw(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct mv88e6xxx_dev *dev;
	int port, reg, page, val = 0, ret = 0,dev_id = 0;

	dev = soho_dev_handle;

	if (!dev) {
		printf("Switch Device not found\n");
		return -ENODEV;
	}

	switch (sw_resolve_options(argv[1])) {
	case SW_INFO:
		mv88e6xxx_display_switch_info(dev);
		break;
	case SW_READ:
		if (argc < 4) {
			printf("Syntax Error: switch read <port> <reg>\n");
			return 1;
		}
		port = (int)simple_strtoul(argv[2], NULL, 16);
		reg  = (int)simple_strtoul(argv[3], NULL, 16);
		ret = mv88e6xxx_read_register(dev, REG_PORT(port), reg);
		if (ret < 0) {
			printf("Failed: Read  - switch port: 0x%X, ", port);
			printf("reg: 0x%X, ret: %d\n", reg, ret);
		} else {
			printf("Read - switch port: 0x%X, ", port);
			printf("reg: 0x%X, val: 0x%X\n", reg, ret);
		}
		break;

	case SW_WRITE:
		if (argc < 5) {
			printf("Syntax Error: ");
			printf("mvswitch write <port> <reg> <val>\n");
			return 1;
		}
		port = (int)simple_strtoul(argv[2], NULL, 16);
		reg  = (int)simple_strtoul(argv[3], NULL, 16);
		val  = (int)simple_strtoul(argv[4], NULL, 16);
		ret = mv88e6xxx_write_register(dev, REG_PORT(port), reg,
					       (unsigned short)val);
		if (ret < 0) {
			printf("Failed: Write - switch port: 0x%X, ", port);
			printf("reg: 0x%X, val: 0x%X, ret: %d\n",
			       reg, val, ret);
		} else {
			printf("Read  - switch port: 0x%X, ", port);
			printf("reg: 0x%X, val: 0x%X\n",
			       reg, val);
		}
		break;

	case SW_PHY_READ:
		if (argc < 5) {
			printf("Syntax Error: ");
			printf("mvswitch phy_read <port> <page> <reg>\n");
			return 1;
		}
		port = (int)simple_strtoul(argv[2], NULL, 16);
		page = (int)simple_strtoul(argv[3], NULL, 16);
		reg  = (int)simple_strtoul(argv[4], NULL, 16);
		ret = mv88e6xxx_read_phy_register(dev, REG_PORT(port),
						  page, reg);
		if (ret < 0) {
			printf("Failed: Read - switch port: 0x%X, ", port);
			printf("page: 0x%X, reg: 0x%X\n, ret: %d",
			       page, reg, ret);
		} else {
			printf("Read - switch port: 0x%X, ", port);
			printf("page: 0x%X, reg: 0x%X, val: 0x%X\n",
			       page, reg, ret);
		}
		break;

	case SW_PHY_WRITE:
		if (argc < 6) {
			printf("Syntax Error: ");
			printf("mvswitch phy_write <port> <page> <reg> <val>\n");
			return 1;
		}
		port = (int)simple_strtoul(argv[2], NULL, 16);
		page = (int)simple_strtoul(argv[3], NULL, 16);
		reg  = (int)simple_strtoul(argv[4], NULL, 16);
		val  = (int)simple_strtoul(argv[5], NULL, 16);
		ret = mv88e6xxx_write_phy_register(dev, REG_PORT(port),
						   page, reg,
						   (unsigned short)val);
		if (ret < 0) {
			printf("Failed: Write - switch port: 0x%X, ", port);
			printf("page: 0x%X, reg: 0x%X, val: 0x%X, ret: %d\n",
			       page, reg, val, ret);
		} else {
			printf("Read - switch port: 0x%X, ", port);
			printf("page: 0x%X, reg: 0x%X, val: 0x%X\n",
			       page, reg, val);
		}
		break;

	case SW_LINK:
		if (argc < 3) {
			printf("Error: Too few arguments\n");
			return 1;
		}
		port = (int)simple_strtoul(argv[2], NULL, 16);
		ret = mv88e6xxx_get_link_status(dev, port);
		break;

	case SW_C45_WRITE:
		port = (int)simple_strtoul(argv[2], NULL, 16);
		dev_id = (int)simple_strtoul(argv[3], NULL, 16);
		reg  = (int)simple_strtoul(argv[4], NULL, 16);
		val  = (int)simple_strtoul(argv[5], NULL, 16);
		ret = mv88e6xxx_write_indirect45(dev, port,dev_id,reg,val);
		if(ret == 0)
			printf("Write port:0x%X dev_id 0x%X reg 0x%X val 0x%X Done! \n",port,dev_id,reg,val);
		else
			printf("Error! \n");
		break;
	case SW_C45_READ:
		port = (int)simple_strtoul(argv[2], NULL, 16);
		dev_id  = (int)simple_strtoul(argv[3], NULL, 16);
		reg  = (int)simple_strtoul(argv[4], NULL, 16);
		val = mv88e6xxx_read_indirect45(dev,port,dev_id,reg);
		if(val < 0)
			printf("Error! \n");
		else
			printf("Read port:0x%X dev_id 0x%X reg 0x%X val 0x%X Done! \n",port,dev_id,reg,val);
		break;

	case SW_NA:
		printf("\"mvswitch %s\" - Wrong command. Try \"help switch\"\n",
		       argv[1]);

	default:
		break;
	}
	return 0;
}

/***************************************************/
U_BOOT_CMD(
	mvswitch,	6,	1,	do_sw,
	"Switch Access commands",
	"mvswitch info - Display switch information\n"
	"mvswitch read <port> <reg> - read switch register <reg> of a <port>\n"
	"mvswitch write <port> <reg> <val> - write <val> to switch register <reg> of a <port>\n"
	"mvswitch phy_read <port> <page> <reg> - read internal switch phy register <reg> at <page> of a switch <port>\n"
	"mvswitch phy_write <port> <page> <reg> <val>- write <val> to internal phy register at <page> of a <port>\n"
	"mvswitch link <port> - Display link state and speed of a <port>\n"
	"mvswitch dev_read <port> <dev_id> <reg>- read internal switch phy register over Clause 45\n"
	"mvswitch dev_read 0x15 0x04 0x2000 :read serdes register from device 4 0x2000\n"
	"mvswitch dev_write <port> <dev_id> <reg> <val> - write internal switch phy register over Clause 45\n"
);
