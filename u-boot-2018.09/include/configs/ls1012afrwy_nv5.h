/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2018 NXP
 */

#ifndef __LS1012AFRWY_H__
#define __LS1012AFRWY_H__

#include "ls1012a_common.h"

#undef CONFIG_SYS_NS16550_COM2
#define CONFIG_SYS_NS16550_COM2		0x21c0600
/* Board Rev*/
#define BOARD_REV_A_B			0x0
#define BOARD_REV_C			0x00080000
#define BOARD_REV_MASK			0x001A0000
/* DDR */
#define CONFIG_DIMM_SLOTS_PER_CTLR	1
#define CONFIG_CHIP_SELECTS_PER_CTRL	1
#define SYS_SDRAM_SIZE_512		0x20000000
#define SYS_SDRAM_SIZE_1024		0x40000000
#define CONFIG_CHIP_SELECTS_PER_CTRL	1
#define CONFIG_CMD_MEMINFO
#define CONFIG_CMD_MEMTEST
#define CONFIG_SYS_MEMTEST_START	0x80000000
#define CONFIG_SYS_MEMTEST_END		0x9fffffff

#ifndef CONFIG_SPL_BUILD
#undef BOOT_TARGET_DEVICES
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(USB, usb, 0)
#endif

#undef CONFIG_ENV_OFFSET
#define CONFIG_ENV_OFFSET              0x1D0000
#undef FSL_QSPI_FLASH_SIZE
#define FSL_QSPI_FLASH_SIZE            SZ_16M
#undef CONFIG_ENV_SECT_SIZE
#define CONFIG_ENV_SECT_SIZE		0x10000 /*64 KB*/
#undef CONFIG_ENV_SIZE
#define CONFIG_ENV_SIZE			0x10000 /*64 KB*/

/*  MMC  */
#ifdef CONFIG_MMC
#define CONFIG_FSL_ESDHC
#define CONFIG_SYS_FSL_MMC_HAS_CAPBLT_VS33
#endif

#define CONFIG_PCIE1		/* PCIE controller 1 */

#define CONFIG_PCI_SCAN_SHOW

#undef CONFIG_EXTRA_ENV_SETTINGS
#define CONFIG_EXTRA_ENV_SETTINGS		\
	"verify=no\0"				\
	"fdt_high=0xffffffffffffffff\0"		\
	"initrd_high=0xffffffffffffffff\0"	\
	"fdt_addr=0x00f00000\0"			\
	"kernel_addr=0x01000000\0"		\
	"kernel_size_sd=0x16000\0"		\
	"kernelhdr_size_sd=0x10\0"		\
	"kernel_addr_sd=0x8000\0"		\
	"kernelhdr_addr_sd=0x4000\0"		\
	"kernelheader_addr=0x1fc000\0"		\
	"kernelheader_addr=0x1fc000\0"		\
	"scriptaddr=0x80000000\0"		\
	"scripthdraddr=0x80080000\0"		\
	"fdtheader_addr_r=0x80100000\0"		\
	"kernelheader_addr_r=0x80200000\0"	\
	"kernelheader_size=0x40000\0"		\
	"kernel_addr_r=0x81000000\0"		\
	"fdt_addr_r=0x90000000\0"		\
	"load_addr=0x96000000\0"		\
	"kernel_size=0x2800000\0"		\
	"kernelheader_size=0x40000\0"		\
	"console=ttyS0,115200\0"		\
	BOOTENV					\
	"boot_scripts=ls1012afrwy_boot.scr\0"	\
	"boot_script_hdr=hdr_ls1012afrwy_bs.out\0"	\
	"consoledev=ttyS1\0"                                    \
	"loadaddr=82000000\0"                                   \
	"WGKernelfile=kernel_NV5.itb\0"                                \
	"SysARoot=mmcblk0p3\0"  \
	"SysBRoot=mmcblk0p2\0"  \
	"wgBootSysA=setenv bootargs root=/dev/$SysARoot rw rootdelay=2 " \
	"console=$consoledev,$baudrate $othbootargs "   \
	"earlycon=uart8250,mmio,0x21c0500 quiet lpj=250000; "    \
	"mmc dev 1; mmc info; " \
	"ext4load mmc 1:3 $loadaddr $WGKernelfile; " \
	"pfe stop; bootm $loadaddr;\0"             \
	"wgBootSysB=setenv bootargs root=/dev/$SysBRoot rw rootdelay=2 " \
	"console=$consoledev,$baudrate $othbootargs "   \
	"earlycon=uart8250,mmio,0x21c0500 quiet lpj=250000; "    \
	"mmc dev 1; mmc info; " \
	"ext4load mmc 1:2 $loadaddr $WGKernelfile; " \
	"pfe stop; bootm $loadaddr;\0"             \
	"wgBootRecovery=setenv bootargs wgmode=safe root=/dev/$SysARoot rw " \
	"console=$consoledev,$baudrate $othbootargs "   \
	"earlycon=uart8250,mmio,0x21c0500 quiet lpj=250000; "    \
	"mmc dev 1; mmc info; " \
	"ext4load mmc 1:3 $loadaddr $WGKernelfile; " \
	"pfe stop; bootm $loadaddr;\0"  \
	"nuke_env=sf probe 0;sf erase 0x1d0000 10000;\0"  \
	"ipaddr=10.0.1.1;\0" \
	"serveraddr=10.0.1.13;\0" \
	"netmask=255.255.255.0;\0" \
	"flash_bootloader=tftp 0x82000000 u-boot_ls1012afrwy_wg_nv5_qspi_complete.bin;" \
	"sf probe 0; sf erase 0 0x1d0000; sf write 0x82000000 0 $filesize;" \
	"sf read 0x80000000 0 $filesize; cmp.b 0x82000000 0x80000000 $filesize;\0"

#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND "run wgBootSysA"
/*
#ifdef CONFIG_TFABOOT
#undef QSPI_NOR_BOOTCOMMAND
#define QSPI_NOR_BOOTCOMMAND "pfe stop; run distro_bootcmd; run sd_bootcmd; "\
			     "env exists secureboot && esbc_halt;"
#else
#define CONFIG_BOOTCOMMAND "pfe stop; run distro_bootcmd; run sd_bootcmd; "\
			   "env exists secureboot && esbc_halt;"
#endif
*/
#define CONFIG_CMD_MEMINFO
#define CONFIG_CMD_MEMTEST
#define CONFIG_SYS_MEMTEST_START	0x80000000
#define CONFIG_SYS_MEMTEST_END		0x9fffffff

/* Ethernet*/
#define CONFIG_IPADDR		192.168.1.1
#define CONFIG_SERVERIP		192.168.1.100
#define CONFIG_NETMASK		255.255.255.0
#define CONFIG_HAS_ETH1
#define CONFIG_HAS_ETH2
#define CONFIG_ETHADDR		00:0B:6B:01:01:01
#define CONFIG_ETH1ADDR		00:0B:6B:01:01:02
#define CONFIG_ETHPRIME     "pfe_eth1"

#include <asm/fsl_secure_boot.h>

#include <asm/fsl_secure_boot.h>
#endif /* __LS1012AFRWY_H__ */
