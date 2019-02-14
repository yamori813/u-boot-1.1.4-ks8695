/*
 * ks8692_ictrol.h   * PING_READY *
 *      This header file defines the ioctrols supported by the driver.
 *
 *  Copyright (c) 2006-2007, Micrel Semiconductor. All rights reserved.
 *
 *  Modification History
 *
 *  Name        Date        Ver     Brief
 *  ----------- ----------- ------- --------------------------------------------
 */
#ifndef	__KS8692_IOCTRL_H
#define	__KS8692_IOCTRL_H

#ifndef	__KERNEL__
#include <stdio.h>
#endif

/*
 * local defines
 */
enum {
	REG_DMA_DUMP,			/* dump all base DMA registers (based on current driver is for) */
	REG_DMA_WOL_DUMP,		/* dump all DMA WOL registers */
	REG_UART_DUMP,			/* dump all UART related registers */
	REG_INT_DUMP,			/* dump all Interrupt related registers */
	REG_TIMER_DUMP,			/* dump all Timer related registers */
	REG_GPIO_DUMP,			/* dump all GPIO related registers */
	REG_I2C_DUMP,		    /* dump all I2C related registers */
	REG_MISC_DUMP,			/* dump all misc registers */
	REG_SNMP_DUMP,			/* dump all SNMP registers */

	DRV_VERSION,			/* get driver version, (we need this since proc is removed from driver) */

	DUMP_PCI_SPACE,			/* dump PCI configuration space for KS8695P */
	DUMP_BRIDGE_REG,		/* dump bridge related register for KS8695P */

	MEMORY_DUMP,			/* to dump given memory */
	MEMORY_SEARCH,			/* to search for given data pattern */

	REG_WRITE,			/* to write IO register */

	DEBUG_DUMP_TX_PACKET,		/* to debug ethernet packet to transmit */
	DEBUG_DUMP_RX_PACKET,		/* to debug ethernet packet received */

	DEBUG_RESET_DESC,		/* to reset Rx descriptors */
	DEBUG_STATISTICS,		/* debug statistics */
	DEBUG_DESCRIPTORS,		/* debug descriptors */

	DEBUG_LINK_STATUS,		/* debug link status */

	CONFIG_LINK_TYPE,		/* configure link media type */
	CONFIG_MULTICAST_TABLE,	/* configure multicast table */

	/* switch configuration for web page */
	CONFIG_SWITCH_GET,		/* get switch configuration settings */
	CONFIG_SWITCH_SET,		/* set switch configuration settings */
};

/* defined configured SWITCH SUBID */
enum CONFIG_SWITCH_SUBID {
	/* configuration related to basic switch web page */
	CONFIG_SW_SUBID_ON,				/* turn on/off switch for LAN */
	CONFIG_SW_SUBID_PORT_VLAN,			/* configure port VLAN ID, and Engress mode */
	CONFIG_SW_SUBID_PRIORITY,			/* configure port priority */

	/* configuration related to advanced switch web page */
	CONFIG_SW_SUBID_ADV_LINK_SELECTION,		/* configure port link selection */
	CONFIG_SW_SUBID_ADV_CTRL,			/* configure switch control register */
	CONFIG_SW_SUBID_ADV_MIRRORING,			/* configure switch port mirroring */
	CONFIG_SW_SUBID_ADV_THRESHOLD,			/* configure threshold for both 802.1p and broadcast storm protection */
	CONFIG_SW_SUBID_ADV_DSCP,			/* configure switch DSCP priority */

	/* configuration related to Switch internal web page */
	CONFIG_SW_SUBID_INTERNAL_LED,			/* configure LED for all */
	CONFIG_SW_SUBID_INTERNAL_MISC,			/* configure misc. */
	CONFIG_SW_SUBID_INTERNAL_SPANNINGTREE,		/* configure spanning tree */

	/* configuration phy related features for KS8695P */
	CONFIG_SW_SUBID_PHY_IF,				/* configure PHY interface, for KS8695P only */
	CONFIG_SW_SUBID_SEC1,				/* configure Switch Engine Control 1 register 0xE804 */

	/* for KS8695P only */
	CONFIG_SW_SUBID_GENERIC_DUMP,			/* generic dump for KS8695), e.g. Dynamic Mac Table, or switch registers */
	CONFIG_SW_SUBID_RATE_CTRL,			/* high/low priority rate control */
};

enum  GENERIC_DUMP {
	GENERIC_DUMP_STATIC,		/* dump static Mac table */
	GENERIC_DUMP_DYNAMIC,		/* dump dynamic Mac table */
	GENERIC_DUMP_VLAN,		/* dump VLAN table */
	GENERIC_DUMP_SWITCH_REGS,	/* dump switch registers for KS8695P */
};

/* defined configured SWITCH SUBID */
enum _DEBUG_PACKET {		/* debug packet bit definition */
	DEBUG_PACKET_LEN	= 0x00000001,		/* debug packet length */
	DEBUG_PACKET_HEADER	= 0x00000002,		/* debug packet header */
	DEBUG_PACKET_CONTENT	= 0x00000004,		/* debug packet content */
	DEBUG_PACKET_OVSIZE	= 0x00000008,		/* dump rx over sized packet content */
	DEBUG_PACKET_UNDERSIZE	= 0x00000010,		/* prompt rx under sized packet */
};

#define	REG_DMA_MAX		    (((REG_MULTICAST_HI - REG_TXCTRL) >> 2) + 1)
#define	REG_DMA_WOL_MAX     (((REG_FRAME3_MASK1 - REG_WOL_CTRL) >> 2) + 1)
#define	REG_UART1_MAX		(((KS8692_UART1_STATUS - KS8692_UART1_RX_BUFFER) >> 2) + 1)
#define	REG_UART_MAX		(REG_UART1_MAX * 4)
#define	REG_INT_MAX		    (((KS8692_IRQ2_PEND_PRIORITY - KS8692_INT_CONTL1) >> 2) + 1)
#define	REG_TIMER_MAX		(((KS8692_TIMER0_PCOUNT - KS8692_TIMER_CTRL) >> 2) + 1)
#define	REG_GPIO_MAX		(((KS8692_GPIO_DATA - KS8692_GPIO_MODE) >> 2) + 1)
#define	REG_I2C_MAX		    (((KS8692_I2C_DEBUG - KS8692_I2C_CTRL) >> 2) + 1)
#define	REG_MISC_MAX		(((KS8692_USB_TEST_STATUS - KS8692_DEVICE_ID) >> 2) + 1)
#define MIB_COUNT_MAX       32
#define HW_MULTICAST_SIZE   8
#define SET_LINK_MAX        1

#define	DUMP_BUFFER_MAX		1024

#define SW_PHY_AUTO		0   /* autosense */
#define SW_PHY_10BASE_T		1   /* 10Base-T */
#define SW_PHY_10BASE_T_FD	2   /* 10Base-T Full Duplex */
#define SW_PHY_100BASE_TX	3   /* 100Base-TX */
#define SW_PHY_100BASE_TX_FD	4   /* 100Base-TX Full Duplex */

/* use __packed in armcc later */
typedef struct {
	uint8_t		byId;
	uint16_t	usLen;
	union {
		uint32_t	uData[0];
		uint16_t	usData[0];
		uint8_t		byData[0];
	} u;
} IOCTRL, *PIOCTRL;

typedef struct {
	uint8_t		byId;
	uint16_t	usLen;
	uint8_t		bySubId;
	union {
		uint32_t	uData[0];
		uint16_t	usData[0];
		uint8_t		byData[0];
	} u;
} IOCTRL_SWITCH, *PIOCTRL_SWITCH;

enum _LINK_SELECTION {
	LINK_SELECTION_FULL_AUTO = 0,		/* fully auto nego */
	LINK_SELECTION_FORCED,				/* forced mode, no auto nego */
	LINK_SELECTION_PARTIAL_AUTO,		/* partial auto nego */
};

/* The proprietary IOCTL code for PHY IO access */
#define SIOC_KS8695_IOCTRL	(SIOCDEVPRIVATE + 15)

#endif	/*__KS8692_IOCTRL_H*/