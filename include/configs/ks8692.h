/*
 * (C) Copyright 2005
 * Greg Ungerer <greg.ungerer@opengear.com>.
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef __CONFIG_H
#define __CONFIG_H


#ifdef CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_USE_FULL_COMMANDS
#else
#if 0
#define CONFIG_USE_FULL_COMMANDS
#endif
#endif


/*
 * High Level Configuration Options
 * (easy to change)
 */
#define CONFIG_IDENT_STRING	"August 28, 2009" /* served as version info */
#define CONFIG_KS8692	1		/* it is a KS8692 CPU */
#define CONFIG_CENTAUR  1		/* it is a Centaur board */

/* It is a KS8692P chip. */
#define CONFIG_ARCH_KS8692P	1
#if 0
/* Rev. A */
#define CONFIG_KS8692P  1		/* it is KS8692P based */
#else
/* Rev. B */
#define CONFIG_KS8692P  2		/* it is KS8692P based */
#endif

#if (0)
#define CONFIG_USE_IRQ			/* we need IRQ/FIQ stuff to test ks8692 interrupt source */
#endif

#define CONFIG_CMDLINE_TAG	 1	/* enable passing of ATAGs	*/
#define CONFIG_SETUP_MEMORY_TAGS 1
#define CONFIG_INITRD_TAG	 1

#define CONFIG_DRIVER_KS8692ETH		/* use KS8692 ethernet driver	*/

#define CFG_MALLOC_LEN		(CFG_ENV_SIZE + 128*1024)

/*
 * Size of malloc() pool
 */
#define CFG_GBL_DATA_SIZE	128	/* size in bytes reserved for initial data */

#if CONFIG_KS8692P > 1
#define U_BOOT_CODE_BLOCKS      3
#else
#define U_BOOT_CODE_BLOCKS      2
#endif
#define CFG_ENV_IS_IN_FLASH     1	/*save the uboot environment variables in flash */

/*
 * Hardware drivers
 */

/*
 * select serial console configuration
 */
#define	CONFIG_SERIAL1
#define CONFIG_CONS_INDEX	1
#define CONFIG_BAUDRATE		115200
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

#if 0
#define CONFIG_NETCONSOLE		/* include NetConsole support	*/
#endif
#if 0
#define CONFIG_NET_MULTI		/* required for netconsole      */
#endif

/*
 * Include additional commands here
 */
/*-----------------------------------------------------------------------
 * PCI stuff
 *-----------------------------------------------------------------------
 */
#define PCI_HOST_ADAPTER 0              /* configure as pci adapter     */
#define PCI_HOST_FORCE  1               /* configure as pci host        */
#define PCI_HOST_AUTO   2               /* detected via arbiter enable  */

#define CONFIG_PCI			            /* include pci support	        */
#define CONFIG_PCI_HOST	PCI_HOST_AUTO   /* select pci host function     */
#undef CONFIG_PCI_PNP			        /* do pci plug-and-play         */
					                    /* resource configuration       */

#define CONFIG_PCI_SCAN_SHOW            /* print pci devices @ startup  */

#define CONFIG_PCI_CONFIG_HOST_BRIDGE 1 /* don't skip host bridge config*/

#define CONFIG_PCI_BOOTDELAY    0       /* enable pci bootdelay variable*/

#define CFG_PCI_SUBSYS_VENDORID 0x16C6  /* PCI Vendor ID: Micrel        */
#define CFG_PCI_SUBSYS_DEVICEID 0x8692  /* PCI Device ID: KS8692        */
#define CFG_PCI_CLASSCODE       0x0600  /* PCI Class Code: host bridge  */

/*-----------------------------------------------------------------------
 * USB stuff
 *-----------------------------------------------------------------------
 */

#undef	CONFIG_USB_UHCI
#define	CONFIG_USB_OHCI

#if defined( CONFIG_USB_OHCI ) || defined( CONFIG_USB_UHCI )
#define	CONFIG_USB_STORAGE
#define CONFIG_DOS_PARTITION
#define CONFIG_ISO_PARTITION
#define	LITTLEENDIAN		1	//used by USB
#define ADD_USB_CMD             CFG_CMD_USB | CFG_CMD_FAT
#else
#define ADD_USB_CMD             0
#endif /* #if defined( CONFIG_USB_OHCI ) || defined( CONFIG_USB_UHCI ) */

/* If NAND is used, add CFG_CMD_NAND to CONFIG_COMMANDS */

#ifdef CONFIG_USE_FULL_COMMANDS
#define	CONFIG_COMMANDS		(CONFIG_CMD_DFL & ~(CFG_CMD_NONSTD) | \
                             CFG_CMD_PCI |                        \
                             CFG_CMD_I2C |                        \
                             CFG_CMD_SPI |                        \
                             CFG_CMD_PING |                       \
                             CFG_CMD_DHCP |                       \
                             CFG_CMD_MII |                        \
                             CFG_CMD_MMC |                        \
                             ADD_USB_CMD)
#else
#define	CONFIG_COMMANDS		(CONFIG_CMD_DFL & ~(CFG_CMD_NONSTD) | CFG_CMD_PCI | CFG_CMD_PING | CFG_CMD_DHCP | ADD_USB_CMD)
#endif

/* this must be included AFTER the definition of CONFIG_COMMANDS (if any) */
#include <cmd_confdefs.h>

#if ((CONFIG_COMMANDS & (CFG_CMD_I2C)) == (CFG_CMD_I2C))
/*-----------------------------------------------------------------------
 * I2C stuff
 *-----------------------------------------------------------------------
 */
#define CONFIG_HARD_I2C		1	/* I2C with hardware support	*/
#undef  CONFIG_SOFT_I2C			/* I2C bit-banged		*/
#define CFG_I2C_SPEED		400000	/* I2C speed and slave address	*/
#define CFG_I2C_SLAVE		0x7F
#if (0)
#define I2C_WM8974          1       /* special code to do I2C write to WM8974 device */
#endif

/*-----------------------------------------------------------------------
 * I2S stuff
 *-----------------------------------------------------------------------
 */
#define MICREL_CMD_I2S      1       /* Micrel specific I2S commands  */

#endif

#define CONFIG_BOOTDELAY	3
#define CONFIG_BOOTARGS		"console=ttyAM0,115200"

/* uboot(0x20000) + env */
#if CONFIG_KS8692P > 1
#define CONFIG_BOOTCOMMAND	"bootm 0x1c040000"
#else
#define CONFIG_BOOTCOMMAND	"bootm 0x1c030000"
#endif

/*
 * Miscellaneous configurable options
 */
#define CFG_LONGHELP				/* undef to save memory		*/
#define CFG_PROMPT		"boot > "	/* Monitor Command Prompt	*/
#define CFG_CBSIZE		256		/* Console I/O Buffer Size	*/
#define CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Print Buffer Size */
#define CFG_MAXARGS		16		/* max number of command args	*/
#define CFG_BARGSIZE		CFG_CBSIZE	/* Boot Argument Buffer Size	*/

#define CFG_MEMTEST_START	0x00010000	/* memtest works on	*/
#define CFG_MEMTEST_END		0x06800000	/* xxx MB in DRAM	*/

#undef	CFG_CLKS_IN_HZ		/* everything, incl board info, in Hz */

#define CFG_LOAD_ADDR		0x00008000	/* default load address */

#define CFG_HZ			(1000)		/* 1ms resolution ticks */

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(128*1024)	/* regular stack */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ	(4*1024)	/* IRQ stack */
#define CONFIG_STACKSIZE_FIQ	(4*1024)	/* FIQ stack */
#endif

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CFG_FLASH_CFI
#define CFG_FLASH_CFI_DRIVER
#define CFG_FLASH_CFI_WIDTH	FLASH_CFI_16BIT
#undef CFG_FLASH_CHECKSUM

/*
 * DDR CONFIGURATION
 */
#define DDR_1GBIT_BY_16		0x000a0101
#define DDR_512MBIT_BY_8	0x000a0101
#define DDR_512MBIT_BY_16	0x000a0201
#define DDR_256MBIT_BY_8	0x000a0201
#define DDR_256MBIT_BY_16	0x000a0301

#define DDR_BUS_32BIT		0
#define DDR_BUS_16BIT		0x00000100

#define KS8692_DDR_ROW_COLON_CONF	DDR_512MBIT_BY_16
#define KS8692_DDR_BUS_SIZE_CONF	DDR_BUS_16BIT

#define CONFIG_NR_DRAM_BANKS	1
#define PHYS_SDRAM_1_SIZE	0x04000000 /* 64 MB */

#define PHYS_FLASH_1		0x1c000000 /* Flash Bank #1 */

#define PHYS_FLASH_SECT_SIZE    0x00010000 /* 64 KB sectors (x1) */

#define CFG_FLASH_BASE		PHYS_FLASH_1
#if CONFIG_KS8692P > 1
#define	CFG_FLASH_SIZE		0x800000
#else
#define	CFG_FLASH_SIZE		0x400000
#endif

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#define CFG_MAX_FLASH_BANKS	1	/* max number of flash banks */
#define CFG_MONITOR_BASE	PHYS_FLASH_1
#define	U_BOOT_CODE_SIZE	(PHYS_FLASH_SECT_SIZE * U_BOOT_CODE_BLOCKS)

#define CFG_ENV_ADDR		(PHYS_FLASH_1 + U_BOOT_CODE_SIZE)
#define CFG_ENV_SECT_SIZE	PHYS_FLASH_SECT_SIZE	/* 64 KB sectors (x1) */
#define CFG_ENV_SIZE		PHYS_FLASH_SECT_SIZE	/* Total Size of Environment */

#define CFG_MAX_FLASH_SECT	(160)	/* max number of sectors on one chip */

/* timeout values are in ticks */
#define CFG_FLASH_WRITE_TOUT	(CFG_HZ/2)	/* Timeout for Flash Write */
#define CFG_FLASH_ERASE_TOUT	(CFG_HZ/2)	/* Timeout for Flash Erase */

/*
 * NAND flash support
 */
#define NAND_MAX_CHIPS		2
#define CFG_MAX_NAND_DEVICE	1
#define CFG_NAND_BASE		0x1ffe0000
#define CONFIG_MTD_NAND_VERIFY_WRITE

/* board depenend parameters */
#define CFG_KS8692_GIGA_PHY
#undef CFG_KS8692_GIGA_PHY_MII	/* define for using VS8201 in MII mode */

#ifdef CFG_KS8692_GIGA_PHY
#define CFG_PHY0_ADDR		0x07
#define CFG_PHY1_ADDR		0x06
#else
#define CFG_PHY0_ADDR		0x1
#define CFG_PHY1_ADDR		0x3
#endif

/*==========================================================================
 * Added by David J. Choi for Micrel KS8692
 */

#define	CONFIG_IPADDR		192.168.1.200
#define	CONFIG_NETMASK		255.255.255.0
#define	CONFIG_SERVERIP 	192.168.1.11
#ifdef CFG_KS8692_GIGA_PHY
#define	CONFIG_ETHADDR		00:10:A1:96:92:01
#define	CONFIG_ETH1ADDR		00:10:A1:96:92:11
#else
#define	CONFIG_ETHADDR		00:10:A1:86:92:01
#define	CONFIG_ETH1ADDR		00:10:A1:86:92:11
#endif
#define CONFIG_HAS_ETH1

#define	CONFIG_AUTOSCRIPT	1
#define	CONFIG_ENV_OVERWRITE

#define CFG_AUTOLOAD		"n"

#define	UBOOT_LOAD_RAMDISK	(TEXT_BASE - CFG_FLASH_SIZE)

#endif	/* __CONFIG_H */
