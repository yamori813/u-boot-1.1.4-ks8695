/*
 * ks8692eth.c -- KS8692 ethernet driver
 *
 * (C) Copyright 2006 ---  Jianjun Zhai  <ken.zhai@micrel.com>
 *
 * This code is redesigned by Ken Zhai for ks8692 ethernet driver
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
/****************************************************************************/
#include <common.h>

#ifdef	CONFIG_DRIVER_KS8692ETH
#include <malloc.h>
#include <net.h>
#include <asm/io.h>

#include <miiphy.h>
#include <asm/arch/platform.h>
#include "../common/cmd_ks8692eth.h"

static void ks8692_dump_packet(int port, int dir, unsigned char *data, int len);

/****************************************************************************/
/*
 * Hardware register access to the KS8692 LAN ethernet port
 * (well, it is the 4 port switch really).
 */
#define	ks8692_read(a)    *((volatile unsigned long *) (KS8692_IO_BASE + (a)))
#define	ks8692_write(a,v) *((volatile unsigned long *) (KS8692_IO_BASE + (a))) = (v)

/****************************************************************************/
/*
 * Define the descriptor in-memory data structures.
 */
struct txdesc_for_all {
	uint32_t	owner;
	uint32_t	ctrl;
	uint32_t	addr;
	uint32_t	next;
};
struct rxdesc_for_all {
	uint32_t	status;
	uint32_t	ctrl;
	uint32_t	addr;
	uint32_t	next;
};

/************************************************************************************/
/*
 * Allocate local data structures to use for receiving and sending
 * packets. Just to keep it all nice and simple.
 */
volatile struct txdesc_for_all ks8692_wan_tx[TXDESCS] __attribute__((aligned(256)));
volatile struct rxdesc_for_all ks8692_wan_rx[RXDESCS] __attribute__((aligned(256)));
volatile uint8_t ks8692_wan_bufs[BUFSIZE*(TXDESCS+RXDESCS)] __attribute__((aligned(2048)));;
static int wanrx_index, wantx_index;

volatile struct txdesc_for_all ks8692_lan_tx[TXDESCS] __attribute__((aligned(256)));
volatile struct rxdesc_for_all ks8692_lan_rx[RXDESCS] __attribute__((aligned(256)));
volatile uint8_t ks8692_lan_bufs[BUFSIZE*(TXDESCS+RXDESCS)] __attribute__((aligned(2048)));;
static int lanrx_index, lantx_index;

static int use_wan=1,save_use_wan, cmd_forced_use=0;
struct net_device_stats ether_net_stats[2];
static uint pkt_dump_flag=0, pkt_dump_port=NONE_PORT;

/*****************************************************************************************/
/*
 *	Ideally we want to use the MAC address stored in flash.
 *	But we do some sanity checks in case they are not present
 *	first.
 */
unsigned char default_eth_mac[] = {
	0x00, 0x10, 0xA1, 0x96, 0x92, 0x01
};
static unsigned char eth_mac[] = { 0, 0, 0, 0, 0, 0 };


/*
 * read PHY register
 */
static int mdio_read(int phy, int reg)
{
	volatile int i;

	if (phy == 0)
		ks8692_write(KS8692_STA_COMM, reg<<6|CFG_PHY0_ADDR<<1|1);	/* prepare to read PHY0 status */
	else
		ks8692_write(KS8692_STA_COMM, reg<<6|CFG_PHY1_ADDR<<1|1);	/* prepare to read PHY1 status */
	ks8692_write(KS8692_STA_CTRL, STA_START);	/* start to read */
	udelay(10000);
	i = ks8692_read(KS8692_STA_STATUS);

   	i = ks8692_read(KS8692_STA_DATA0);
#if 0
	printf("mdio port%d reg%d = %x\n",phy,reg,i);
#endif
   	return i;
}

#if 0
static int atoi(int i)
{
	if( i >= '0' && i <= '9' )
		return i-'0';
	else if( i >= 'a' && i <= 'f' )
		return i - 'a' + 10;
	else if( i >= 'A' && i <= 'F' )
		return i - 'A' + 10;
	else {
		printf("Input for ASCII conversion is wrong[0x%x]\n",i);
		return i;
	}
}

void ks8692_getmac(void)
{
	unsigned char *fp;
	unsigned char	temp[32];

	memset(temp,0x00,sizeof(temp));
	fp = (unsigned char*) getenv("ethaddr");
	if (fp != NULL ) {
		eth_mac[0] = ( atoi( *(fp+0) ) << 4 | atoi( *(fp+1)) );
		eth_mac[1] = ( atoi( *(fp+3) ) << 4 | atoi( *(fp+4)) );
		eth_mac[2] = ( atoi( *(fp+6) ) << 4 | atoi( *(fp+7)) );
		eth_mac[3] = ( atoi( *(fp+9) ) << 4 | atoi( *(fp+10)) );
		eth_mac[4] = ( atoi( *(fp+12) ) << 4 | atoi( *(fp+13)) );
		eth_mac[5] = ( atoi( *(fp+15) ) << 4 | atoi( *(fp+16)) );
	}
}
#endif

/***************************************************************************************/

static int eth_initted = 0;
static int wan_initted = 0;
static int lan_initted = 0;

static void eth_wan_reset(bd_t *bd)
{
	uint i;
	int nTimeOut = 1000;
	unsigned int uReg;

	do
	{
		uReg = ks8692_read(KS8692_WAN_DMA_TX);
		if (!(uReg & 0x80000000))
			break;
	} while (--nTimeOut);

	if (nTimeOut < 1) 
	    debug ("%s(%d): eth_wan_reset() ERROR!\n", __FILE__, __LINE__);

	/* Set WAN MAC address */
	ks8692_write(KS8692_WAN_MAC_LOW, (eth_mac[5] | (eth_mac[4] << 8) |
		(eth_mac[3] << 16) | (eth_mac[2] << 24)));
	ks8692_write(KS8692_WAN_MAC_HIGH, (eth_mac[1] | (eth_mac[0] << 8)));

	if ( wan_initted )
		goto start_wan;

	wanrx_index = 0;
	wantx_index = 0;

	/* Initialize descriptor rings */
	for (i = 0; (i < TXDESCS); i++) {
		ks8692_wan_tx[i].owner = 0;
		ks8692_wan_tx[i].ctrl = 0;
		ks8692_wan_tx[i].addr = (uint32_t) &ks8692_wan_bufs[i*BUFSIZE];
		ks8692_wan_tx[i].next = (uint32_t) &ks8692_wan_tx[i+1];
	}
	ks8692_wan_tx[TXDESCS-1].ctrl = 0x02000000;
	ks8692_wan_tx[TXDESCS-1].next = (uint32_t) &ks8692_wan_tx[0];

	for (i = 0; (i < RXDESCS); i++) {
		ks8692_wan_rx[i].status = 0x80000000;
		ks8692_wan_rx[i].ctrl = BUFSIZE - 4;
		ks8692_wan_rx[i].addr = (uint32_t) &ks8692_wan_bufs[(i+TXDESCS)*BUFSIZE];
		ks8692_wan_rx[i].next = (uint32_t) &ks8692_wan_rx[i+1];
	}
	ks8692_wan_rx[RXDESCS-1].ctrl |= 0x00080000;
	ks8692_wan_rx[RXDESCS-1].next = (uint32_t) &ks8692_wan_rx[0];

	/* Enable the ethernet engine */
	ks8692_write(KS8692_WAN_TX_LIST, (uint32_t) &ks8692_wan_tx[0]);
	ks8692_write(KS8692_WAN_RX_LIST, (uint32_t) &ks8692_wan_rx[0]);

start_wan:
	ks8692_write(KS8692_WAN_DMA_TX, 0x08000203 );
	ks8692_write(KS8692_WAN_DMA_RX, 0x08000271 );

	ks8692_write(KS8692_WAN_DMA_RX_START, 0x1);
	wan_initted = 1;
}

/*********************************************************************************/
static void eth_lan_reset(bd_t *bd)
{
	uint i;
	int nTimeOut = 1000;
	unsigned int uReg;

	do
	{
	   uReg = ks8692_read(KS8692_LAN_DMA_TX);
	   if (!(uReg & 0x80000000))
			break;
	} while (--nTimeOut);

	if (nTimeOut < 1) 
	    debug ("%s(%d): eth_lan_reset() ERROR!\n", __FILE__, __LINE__);

	/* Set MAC address */
	ks8692_write(KS8692_LAN_MAC_LOW, (eth_mac[5] | (eth_mac[4] << 8) |
		(eth_mac[3] << 16) | (eth_mac[2] << 24)));
	ks8692_write(KS8692_LAN_MAC_HIGH, (eth_mac[1] | (eth_mac[0] << 8)));

	if ( lan_initted )
		goto start_lan;

	lanrx_index = 0;
	lantx_index = 0;

	/* Initialize descriptor rings */
	for (i = 0; (i < TXDESCS); i++) {
		ks8692_lan_tx[i].owner = 0;
		ks8692_lan_tx[i].ctrl = 0;
		ks8692_lan_tx[i].addr = (uint32_t) &ks8692_lan_bufs[i*BUFSIZE];
		ks8692_lan_tx[i].next = (uint32_t) &ks8692_lan_tx[i+1];
	}
	ks8692_lan_tx[TXDESCS-1].ctrl = 0x02000000;
	ks8692_lan_tx[TXDESCS-1].next = (uint32_t) &ks8692_lan_tx[0];

	for (i = 0; (i < RXDESCS); i++) {
		ks8692_lan_rx[i].status = 0x80000000;
		ks8692_lan_rx[i].ctrl = BUFSIZE - 4;
		ks8692_lan_rx[i].addr = (uint32_t) &ks8692_lan_bufs[(i+TXDESCS)*BUFSIZE];
		ks8692_lan_rx[i].next = (uint32_t) &ks8692_lan_rx[i+1];
	}
	ks8692_lan_rx[RXDESCS-1].ctrl |= 0x00080000;
	ks8692_lan_rx[RXDESCS-1].next = (uint32_t) &ks8692_lan_rx[0];

	/* Enable the ethernet engine */
	ks8692_write(KS8692_LAN_TX_LIST, (uint32_t) &ks8692_lan_tx[0]);
	ks8692_write(KS8692_LAN_RX_LIST, (uint32_t) &ks8692_lan_rx[0]);

start_lan:
	ks8692_write(KS8692_LAN_DMA_TX, 0x08000203 );
	ks8692_write(KS8692_LAN_DMA_RX, 0x08000275 );

	ks8692_write(KS8692_LAN_DMA_RX_START, 0x1);
	lan_initted = 1;
}

/****************************************************************************/
#ifdef CONFIG_NET_MULTI
static int  ksz_init (
	struct eth_device* dev,
#else
int eth_init (
#endif
	bd_t *bd)
{
#ifndef CFG_KS8692_GIGA_PHY_MII
	int ureg10;
#endif
	int ureg5;
	int misc = 0;
	unsigned long i;

	if ( eth_initted )
		goto skip_reset;

#ifdef CONFIG_NET_MULTI
	if ( *(( int* ) dev->enetaddr ) == 0 ) {
		memcpy( eth_mac, default_eth_mac, 6 );
		if ( KS8692_WAN_DMA_TX == dev->iobase )
			eth_mac[ 5 ] ^= 0x10;
		memcpy( dev->enetaddr, eth_mac, 6 );
	}
#endif
	if ( *(( int* ) bd->bi_enetaddr ) == 0 ) {
		memcpy( eth_mac, default_eth_mac, 6 );
		eth_mac[ 5 ] ^= 0x10;
		memcpy( bd->bi_enetaddr, eth_mac, 6 );
	}
#ifdef CONFIG_HAS_ETH1
	if ( *(( int* ) bd->bi_enet1addr ) == 0 ) {
		memcpy( eth_mac, default_eth_mac, 6 );
		memcpy( bd->bi_enet1addr, eth_mac, 6 );
	}
#endif

	/* Reset the ethernet engines first */
	i = ks8692_read( KS8692_SYSTEM_BUS_CLOCK );
	ks8692_write( KS8692_SYSTEM_BUS_CLOCK, ( i & ~SYSTEM_BUS_CLOCK_MASK ) |
		SYSTEM_BUS_CLOCK_125 );
	ks8692_write(KS8692_WAN_DMA_TX, 0x80000000);
	ks8692_write(KS8692_LAN_DMA_TX, 0x80000000);
	ks8692_write( KS8692_SYSTEM_BUS_CLOCK, i );

skip_reset:
	i = mdio_read(0,1);
   	if (((i & 0xffff) != 0xffff) && (i & 0x4)) {
		use_wan = 1;
		ureg5 = mdio_read(0,5);		/* 10/100Base T status */
#ifndef CFG_KS8692_GIGA_PHY_MII
		ureg10 = mdio_read(0,10);	/* 1000Base T status */
		if (ureg10 & 0x800)
		/* 1000 FD */
        		misc = 0x180 | MISC_PORT_1000M | MISC_PORT_FD;
		else if (ureg10 & 0x400)
		/* 1000 HD */
        		misc = 0x180 | MISC_PORT_1000M | MISC_PORT_HD;
		else
#endif
		if (ureg5 & 0x100)
		/* 100 FD */
        		misc = 0x180 | MISC_PORT_100M | MISC_PORT_FD;
		else if (ureg5 & 0x80)
		/* 100 HD */
        		misc = 0x180 | MISC_PORT_100M | MISC_PORT_HD;
		else if (ureg5 & 0x40)
		/* 10 FD */
        		misc = 0x180 | MISC_PORT_10M | MISC_PORT_FD;
		else if (ureg5 & 0x20)
		/* 10 HD */
        		misc = 0x180 | MISC_PORT_10M | MISC_PORT_HD;
		else
			printf("eth_init ERROR - no valid mode found\n");
#ifdef CFG_KS8692_GIGA_PHY
#ifndef CFG_KS8692_GIGA_PHY_MII
		misc |= 0x800;		/* select RGMII */
#endif
#if CONFIG_KS8692P > 1
		misc |= MISC_RGMII_TX_DELAY | MISC_RGMII_RX_DELAY;
#endif
#endif
		/* configure WAN port */
		ks8692_write(KS8692_WAN_MISC_CFG,misc);
	}

	i = mdio_read(1,1);
   	if (((i & 0xffff) != 0xffff) && (i & 0x4)) {
		use_wan = 0;
		ureg5 = mdio_read(1,5);		/* 10/100Base T status */
#ifndef CFG_KS8692_GIGA_PHY_MII
		ureg10 = mdio_read(1,10);	/* 1000Base T status */
		if (ureg10 & 0x800)
		/* 1000 FD */
        		misc = 0x180 | MISC_PORT_1000M | MISC_PORT_FD;
		else if (ureg10 & 0x400)
		/* 1000 HD */
        		misc = 0x180 | MISC_PORT_1000M | MISC_PORT_HD;
		else
#endif
		if (ureg5 & 0x100)
		/* 100 FD */
        		misc = 0x180 | MISC_PORT_100M | MISC_PORT_FD;
		else if (ureg5 & 0x80)
		/* 100 HD */
        		misc = 0x180 | MISC_PORT_100M | MISC_PORT_HD;
		else if (ureg5 & 0x40)
		/* 10 FD */
        		misc = 0x180 | MISC_PORT_10M | MISC_PORT_FD;
		else if (ureg5 & 0x20)
		/* 10 HD */
        		misc = 0x180 | MISC_PORT_10M | MISC_PORT_HD;
		else
			printf("eth_init ERROR - no valid mode found\n");
#if CONFIG_KS8692P > 1
#ifdef CFG_KS8692_GIGA_PHY
#ifndef CFG_KS8692_GIGA_PHY_MII
		misc |= 0x800;		/* select RGMII */
#endif
		misc |= MISC_RGMII_TX_DELAY | MISC_RGMII_RX_DELAY;
#endif
#endif
		/* configure LAN port */
		ks8692_write(KS8692_LAN_MISC_CFG,misc);
	}
	eth_initted = 1;

#ifdef CONFIG_NET_MULTI
	if ( ( KS8692_WAN_DMA_TX == dev->iobase  &&  !use_wan )  ||
			( KS8692_LAN_DMA_TX == dev->iobase  &&  use_wan ) )
		return 0;
	memcpy( eth_mac, dev->enetaddr, 6 );
	if ( KS8692_WAN_DMA_TX == dev->iobase )
#else
	printf("use wan = %d\n",use_wan);
	memcpy( eth_mac, bd->bi_enetaddr, 6 );
	if (use_wan)
#endif
		eth_wan_reset(bd);
	else
		eth_lan_reset(bd);

	return 1;
}

/****************************************************************************/
void eth_wan_halt(void)
{
	unsigned int uReg;

	/* Disable WAN MAC DMA transmit */
	uReg = ks8692_read(KS8692_WAN_DMA_TX);
	ks8692_write(KS8692_WAN_DMA_TX, (uReg & ~0x1));

	/* Disable WAN MAC DMA receive */
	uReg = ks8692_read(KS8692_WAN_DMA_RX);
	ks8692_write(KS8692_WAN_DMA_RX, (uReg & ~0x1));
}

void eth_lan_halt(void)
{
	unsigned int uReg;

	/* Disable LAN MAC DMA transmit */
	uReg = ks8692_read(KS8692_LAN_DMA_TX);
	ks8692_write(KS8692_LAN_DMA_TX, (uReg & ~0x1));

	/* Disable LAN MAC DMA receive */
	uReg = ks8692_read(KS8692_LAN_DMA_RX);
	ks8692_write(KS8692_LAN_DMA_RX, (uReg & ~0x1));
}

#ifdef CONFIG_NET_MULTI
static void ksz_halt (struct eth_device* dev)
#else
void eth_halt (void)
#endif
{
#ifdef CONFIG_NET_MULTI
	if ( KS8692_WAN_DMA_TX == dev->iobase )
#else
	if (use_wan)
#endif
		eth_wan_halt();
	else
		eth_lan_halt();
}

/****************************************************************************/
#if 0
int eth_wan_rx(void)
{
	volatile struct rxdesc_for_all *dp;
	int len = 0;

	int portid =  WAN_PORT;

	dp = &ks8692_wan_rx[wanrx_index];

	while ((dp->status & 0x80000000) == 0) {
		len = (dp->status & 0x7ff) - 4;
                ks8692_dump_packet(portid,INBOUND,dp->addr,len);
		NetReceive((void *) dp->addr, len);
		dp->status = 0x80000000;
                STAT_NET(portid,rx_packets++);
                STAT_NET(portid,rx_bytes) += len;
		if (++wanrx_index >= RXDESCS)
			wanrx_index = 0;
		dp = &ks8692_wan_rx[wanrx_index];
	}

	if ( len ) {
		ks8692_write(KS8692_WAN_DMA_RX_START, 0x1);
	}

	return len;
}
int eth_lan_rx(void)
{
	volatile struct rxdesc_for_all *dp;
	int len = 0;

	int portid = LAN_PORT;

	dp = &ks8692_lan_rx[lanrx_index];

	while ((dp->status & 0x80000000) == 0) {
		len = (dp->status & 0x7ff) - 4;
                ks8692_dump_packet(portid,INBOUND,dp->addr,len);
		NetReceive((void *) dp->addr, len);
		dp->status = 0x80000000;
                STAT_NET(portid,rx_packets++);
                STAT_NET(portid,rx_bytes) += len;
		if (++lanrx_index >= RXDESCS)
			lanrx_index = 0;
		dp = &ks8692_lan_rx[lanrx_index];
	}

	if ( len ) {
		ks8692_write(KS8692_LAN_DMA_RX_START, 0x1);
	}

	return len;
}
#else
int eth_wan_rx(void)
{
	volatile struct rxdesc_for_all *rxdp;
	volatile struct txdesc_for_all *txdp;
	int len = 0;
	int portid = WAN_PORT;

	rxdp = &ks8692_wan_rx[wanrx_index];
	while ((rxdp->status & 0x80000000) == 0) {
 	 len = (rxdp->status & 0x7ff) - 4;
	 rxdp->status = 0x80000000;
#if 1
          ks8692_dump_packet(portid,INBOUND,rxdp->addr,len);
	 NetReceive((void *) rxdp->addr, len);
         STAT_NET(portid,rx_packets++);
         STAT_NET(portid,rx_bytes) += len;
#else
	txdp = &ks8692_wan_tx[wantx_index];

	while( txdp->owner & 0x80000000 ) ;

	txdp->addr = rxdp->addr;

	txdp->ctrl = len | 0xe0000000;
	txdp->owner = 0x80000000;

#if 0
       	ks8692_dump_packet(portid,OUTBOUND,txdp->addr,len);
#endif
	ks8692_write(KS8692_WAN_DMA_TX_START, 0x1);

	if (++wantx_index >= TXDESCS)
		wantx_index = 0;
#endif
	 if (++wanrx_index >= RXDESCS)
	   wanrx_index = 0;
	 rxdp = &ks8692_wan_rx[wanrx_index];
	}
	if ( len ) {
	  ks8692_write(KS8692_WAN_DMA_RX_START, 0x1);
	}
	return len;
}

int eth_lan_rx(void)
{
	volatile struct rxdesc_for_all *rxdp;
	volatile struct txdesc_for_all *txdp;
	int len = 0;
	int portid = LAN_PORT;

	rxdp = &ks8692_lan_rx[lanrx_index];

	while ((rxdp->status & 0x80000000) == 0) {
	  len = (rxdp->status & 0x7ff) - 4;
#if 1
         ks8692_dump_packet(portid,INBOUND,rxdp->addr,len);
	 NetReceive((void *) rxdp->addr, len);
         STAT_NET(portid,rx_packets++);
         STAT_NET(portid,rx_bytes) += len;
#else
	txdp = &ks8692_lan_tx[lantx_index];

	while( txdp->owner & 0x80000000 ) ;

	txdp->addr = rxdp->addr;

	txdp->ctrl = len | 0xe0000000;
	txdp->owner = 0x80000000;

#if 0
       	ks8692_dump_packet(portid,OUTBOUND,txdp->addr,len);
#endif
	ks8692_write(KS8692_LAN_DMA_TX_START, 0x1);

	if (++lantx_index >= TXDESCS)
		lantx_index = 0;
#endif
	 rxdp->status = 0x80000000;
	 if (++lanrx_index >= RXDESCS)
	   lanrx_index = 0;
	 
         rxdp = &ks8692_lan_rx[lanrx_index];
	}

	if ( len ) {
	  ks8692_write(KS8692_LAN_DMA_RX_START, 0x1);
	}

	return len;
}
#endif

#ifdef CONFIG_NET_MULTI
static int  ksz_recv (struct eth_device* dev)
#else
int eth_rx (void)
#endif
{
#ifdef CONFIG_NET_MULTI
	if ( KS8692_WAN_DMA_TX == dev->iobase )
#else
	if (use_wan)
#endif
		return eth_wan_rx();
	else
		return eth_lan_rx();
}

/***************************************************************************/
int eth_wan_send(volatile void *packet, int len)
{
	volatile struct txdesc_for_all *dp;

	dp = &ks8692_wan_tx[wantx_index];

	while( dp->owner & 0x80000000 ) ;

        memcpy((void *) dp->addr, (void *) packet, len);
	if (len < 64) {
		memset((void *) dp->addr+len, 0, 64-len);
		len = 64;
	}

	dp->ctrl = len | 0xe0000000;
	dp->owner = 0x80000000;

	ks8692_write(KS8692_WAN_DMA_TX_START, 0x1);
/* Staticstic & packet dump */
       	STAT_NET(WAN_PORT,tx_packets++);
       	STAT_NET(WAN_PORT,tx_bytes) += len;
       	ks8692_dump_packet(WAN_PORT,OUTBOUND,packet,len);

	if (++wantx_index >= TXDESCS)
		wantx_index = 0;

	return len;
}

int eth_lan_send(volatile void *packet, int len)
{
	volatile struct txdesc_for_all *dp;

	dp = &ks8692_lan_tx[lantx_index];

	while( dp->owner & 0x80000000 ) ;

        memcpy((void *) dp->addr, (void *) packet, len);
	if (len < 64) {
		memset((void *) dp->addr+len, 0, 64-len);
		len = 64;
	}

	dp->ctrl = len | 0xe0000000;
	dp->owner = 0x80000000;

	ks8692_write(KS8692_LAN_DMA_TX_START, 0x1);
/* Staticstic & packet dump */
       	STAT_NET(LAN_PORT,tx_packets++);
       	STAT_NET(LAN_PORT,tx_bytes) += len;
       	ks8692_dump_packet(LAN_PORT,OUTBOUND,packet,len);

	if (++lantx_index >= TXDESCS)
		lantx_index = 0;

	return len;
}

#ifdef CONFIG_NET_MULTI
static int  ksz_send (
	struct eth_device* dev,
#else
int eth_send (
#endif
	volatile void *packet, int len)
{
#ifdef CONFIG_NET_MULTI
	if ( KS8692_WAN_DMA_TX == dev->iobase )
#else
	if (use_wan)
#endif
		return eth_wan_send(packet, len);
	else
		return eth_lan_send(packet, len);
}

void eth_link_status(struct eth_linksts *ptr )
{
	memset(ptr,0x00,sizeof(struct eth_linksts));

	ks8692_write(KS8692_STA_COMM, 0x43);	/* prepare to read PHY0 status - reg1 */
	ks8692_write(KS8692_STA_CTRL, STA_START);	/* start to read */
	udelay(10000);			/* wait */
        ptr->wanport =  (ks8692_read(KS8692_STA_DATA0) >> 2) & 0x1;

	ks8692_write(KS8692_STA_COMM, 0x47);	/* prepare to read PHY1 status - reg1 */
	ks8692_write(KS8692_STA_CTRL, STA_START);	/* start to read */
	udelay(10000);			/* wait */
        ptr->lanport =  (ks8692_read(KS8692_STA_DATA0) >> 2) & 0x1;
}

int eth_init_cmd(unsigned int portid)
{
        DECLARE_GLOBAL_DATA_PTR;

        bd_t *bd = gd->bd;
	cmd_forced_use = 1;
	save_use_wan = use_wan;
	/* overwrite use_wan */
	use_wan = (portid == WAN_PORT ) ? 1 : 0;
	eth_initted = wan_initted = lan_initted = 0;
	return eth_init(bd);
}

void get_mib_reset(void);

void eth_halt_cmd(void)
{
	cmd_forced_use = 0;
	get_mib_reset();
	use_wan = save_use_wan;
}

void eth_pktdump_update(uint port, uint flag)
{
        pkt_dump_port = port;
	pkt_dump_flag = flag;
}

void eth_buffer_link(uint BufNum,uint BufLen,uint SameBuf)
{
	char *pTxBuf = (char*)( TEST_BUFFER_START );
	volatile struct txdesc_for_all *dp;

	int	descinx=lantx_index;
	int	bufinx=0;
	int	lpcnt=0;

	for( lpcnt=0; lpcnt < TXDESCS; lpcnt++ )
	{	
		char *srcptr, *destptr;
		dp = &ks8692_lan_tx[descinx];
		destptr = (char *)dp->addr;
		srcptr = (char *)(pTxBuf + (BufNum + bufinx)%TEST_BUFFER_COUNT * TEST_BUFSIZ);

		memcpy(destptr,srcptr,BufLen);

		if(!SameBuf) bufinx++;
		descinx = (descinx+1)%TXDESCS;
	}
}

int eth_send_fast(uint BufLen, uint cnt)
{
        volatile struct txdesc_for_all *dp;
        uint    icnt=0;

	/* check only the last packet being sent. */
        while ( ks8692_lan_tx[ (lantx_index+cnt-1)%TXDESCS ].owner & 0x80000000 ) ;

        for( icnt=0; icnt < cnt; ++icnt)
        {
                dp = &ks8692_lan_tx[lantx_index];
                dp->ctrl = BufLen | 0xe0000000;
                dp->owner = 0x80000000;
                lantx_index = (lantx_index+1)%TXDESCS;
        }

	if ( use_wan ) {
		ks8692_write(KS8692_WAN_DMA_TX_START, 0x1);
	}

	else {
		ks8692_write(KS8692_LAN_DMA_TX_START, 0x1);
	}

	return BufLen;
}

int eth_rx_fast(void)
{
	volatile struct rxdesc_for_all *dp;
	int len = 0;

	int portid = use_wan ? WAN_PORT : LAN_PORT;

	dp = &ks8692_lan_rx[lanrx_index];

	while ((dp->status & 0x80000000) == 0) {
		len = (dp->status & 0x7ff) - 4;
                ks8692_dump_packet(portid,INBOUND,dp->addr,len);
		dp->status = 0x80000000;
                STAT_NET(portid,rx_packets++);
                STAT_NET(portid,rx_bytes) += len;
		if (++lanrx_index >= RXDESCS)
			lanrx_index = 0;
		dp = &ks8692_lan_rx[lanrx_index];
	}

	if ( len ) {
		if ( use_wan )
			ks8692_write(KS8692_WAN_DMA_RX_START, 0x1);
		else
			ks8692_write(KS8692_LAN_DMA_RX_START, 0x1);
	}

	return len;
}

void get_ks8692_geninfo( struct genInfo *buf)
{
        strcpy(buf->portName, use_wan==1 ? "WAN" : "LAN");
        buf->txdesc = TXDESCS;
        buf->rxdesc = RXDESCS;
        buf->txdesc_addr = (uint) ks8692_lan_tx;
        buf->rxdesc_addr = (uint) ks8692_lan_rx;
        buf->bufsize = BUFSIZE;
        buf->bufstart = (uint) ks8692_lan_bufs;
}

static void ks8692_dump_packet(int port, int dir, unsigned char *data, int len)
{
        /* we may need to have locking mechamism to use this function, since Rx call it within INT context
           and Tx call it in normal context */
        if ( pkt_dump_port >= NONE_PORT )
                return;

        if( pkt_dump_port != ALL_PORT &&  pkt_dump_port != port )
                return;

        if (pkt_dump_flag && len >= 18) {
		if( use_wan )
                	printf("\nWAN %s ", dir==INBOUND ? "Rx" : "Tx");
		else
                	printf("\nLAN Port=%d %s ", port, dir==INBOUND ? "Rx" : "Tx");
                if (pkt_dump_flag & DEBUG_PACKET_LEN) {
                        printf("Pkt-Len=0x%x\n", len);
                }
		else
                        printf("\n");

                if (pkt_dump_flag & DEBUG_PACKET_CONTENT) {
                        int     j = 0, k;

                        do {
                                printf(" %08x   ", (int)(data+j));
                                for (k = 0; (k < 16 && len); k++, data++, len--) {
                                        printf("%02x  ", *data);
                                }
                        	printf("\n");
                                j += 16;
                        } while (len > 0);
                        printf("\n");
                }
        }
}

#define	KS8692_SEIAC	0xE850
#define	KS8692_SEIADL	0xE85C
void get_mib_reset( )
{
	unsigned long reg,value=0;

	reg = 0x0c60;
	for( reg = 0x0c60;reg <= 0x0d09; reg++)
	for( reg = 0x0c60;reg <= 0x0c7f; reg++)
	{
		ks8692_write(KS8692_SEIAC, reg);
		ks8692_write(KS8692_SEIADL, value);
	}
}

static uint ks8692_read_rep( uint reg )
{
	unsigned int value = 0;
	unsigned int cnt=0;
	while( ++cnt < 10000 )
	{
		value = ks8692_read(reg) ;
		if( value & 0x40000000 ) break;
		udelay(1000);
	}
	if( cnt >= 10000 )
		printf("Fail to read reg=0x%lx value=0x%lx\n", reg,value);
	return (value & 0x3fffffff);
}


void get_mib_count( struct mib_table *ptr)
{
	unsigned long reg;
	uint	*uptr = (uint *)ptr;
	
	for (reg = 0x1c60; reg < 0x1c80; reg++)
	{
		ks8692_write(KS8692_SEIAC, reg);
		*uptr = (uint) ks8692_read_rep(KS8692_SEIADL) ;
		uptr++;
		if( reg == 0x1c7f ) reg = 0x1d00-1;
	}
	for (reg = 0x1d00; reg < 0x1d0a; reg++)
	{
		ks8692_write(KS8692_SEIAC, reg);
		*uptr = (uint)( ks8692_read(KS8692_SEIADL) & 0xffff );
		uptr++;
	}
}

int nettest(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	bd_t *bd = gd->bd;
	int count;
	int i;
	struct txdesc_for_all *tdp;
	volatile struct txdesc_for_all *wantdp = &ks8692_wan_tx[0];
	volatile struct txdesc_for_all *lantdp = &ks8692_lan_tx[0];
	struct rxdesc_for_all *rdp;
	int rxindex = 0;
	int txindex = 0;
	int wantxindex = 0;
	int lantxindex = 0;
	int *p;
	int led = 0;

	eth_initted = wan_initted = lan_initted = 0;
	eth_init(bd);
	eth_pktdump_update(ALL_PORT,DEBUG_PACKET_LEN | DEBUG_PACKET_CONTENT);
#if 0	/* echo back the received packets on one port */
	if (use_wan) {
	rdp = &ks8692_wan_rx[rxindex];
	tdp = &ks8692_wan_tx[txindex];
	while ( !ctrlc())
  	{
		while ((rdp->status & 0x80000000) == 0) {
			if (rdp->status & 0x10000000) { 
				/* error packets */
				printf("error %x\n",rdp->status);
				int len = rdp->status & 0x7ff;
				if (len == 0) len = 16;
                		ks8692_dump_packet(0,INBOUND,rdp->addr,len);
			}
			led += 0x100;
			ks8692_write(KS8692_GPIO_DATA,led);
			while (tdp->owner & 0x80000000) ;
			tdp->addr = rdp->addr;
			p = (int *)tdp->addr;
			*p = 0x00123400;
			tdp-> ctrl = ((rdp->status & 0x7ff) - 4) | 0xe0000000;
			tdp-> owner = 0x80000000;
			ks8692_write(KS8692_WAN_DMA_TX_START, 0x01);
			if (++txindex >= TXDESCS)
				txindex = 0;
			tdp = &ks8692_wan_tx[txindex];
			rdp->status = 0x80000000;
			if (++rxindex >= RXDESCS)
				rxindex = 0;
			rdp = &ks8692_wan_rx[rxindex];
			ks8692_write(KS8692_WAN_DMA_RX_START, 0x01);
		}
	}
	/* print MIB counter */
	count = ks8692_read(0xed28);
	count += ks8692_read(0xed2c);
	count += ks8692_read(0xed30);
	printf(" receive packets count: %d\n", count);
	count = ks8692_read(0xed5c);
	count += ks8692_read(0xed60);
	count += ks8692_read(0xed64);
	count += ks8692_read(0xed68);
	printf(" transmit packets count: %d\n",count);
	eth_wan_halt();
	} else {
	rdp = &ks8692_lan_rx[rxindex];
	tdp = &ks8692_lan_tx[txindex];
	while ( !ctrlc())
  	{
		while ((rdp->status & 0x80000000) == 0) {
			if (rdp->status & 0x10000000) { 
				/* error packets */
				printf("error %x\n",rdp->status);
				int len = rdp->status & 0x7ff;
				if (len == 0) len = 16;
                		ks8692_dump_packet(0,INBOUND,rdp->addr,len);
			}
			led += 0x100;
			ks8692_write(KS8692_GPIO_DATA,led);
			while (tdp->owner & 0x80000000) ;
			tdp->addr = rdp->addr;
			p = (int *)tdp->addr;
			*p = 0x00123400;
			tdp-> ctrl = ((rdp->status & 0x7ff) - 4) | 0xe0000000;
			tdp-> owner = 0x80000000;
			ks8692_write(KS8692_LAN_DMA_TX_START, 0x01);
			if (++txindex >= TXDESCS)
				txindex = 0;
			tdp = &ks8692_lan_tx[txindex];
			rdp->status = 0x80000000;
			if (++rxindex >= RXDESCS)
				rxindex = 0;
			rdp = &ks8692_lan_rx[rxindex];
			ks8692_write(KS8692_LAN_DMA_RX_START, 0x01);
		}
	}
	/* print MIB counter */
	count = ks8692_read(0xede8);
	count += ks8692_read(0xedec);
	count += ks8692_read(0xedf0);
	printf(" receive packets count: %d\n", count);
	count = ks8692_read(0xee20);
	count += ks8692_read(0xee24);
	count += ks8692_read(0xee28);
	count += ks8692_read(0xee2c);
	printf(" transmit packets count: %d\n",count);
	eth_lan_halt();
	}
#else	/* echo back packets on both ports */
	ks8692_write(KS8692_WAN_DMA_TX, 0x08000201 );
	ks8692_write(KS8692_LAN_DMA_TX, 0x08000201 );
	wantdp = &ks8692_wan_tx[0];
	lantdp = &ks8692_lan_tx[0];
	/* setup all tx desc to send initially */
	for (i=0; i<TXDESCS; i++) {
		unsigned short *p;
		int j;
		p = (unsigned short *)wantdp->addr;
		for (j=0; j < 1514/2 ; j++)
			*p++ = 0x5555;	/* data pattern */
		*p ++ = 0x267e;
		*p ++ = 0xc252;
		wantdp-> ctrl = 0xe00005ee;
		wantdp++->owner = 0x80000000;

		p = (unsigned short *)lantdp->addr;
		for (j=0; j < 1514/2 ; j++)
			*p++ = 0x5555;	/* data pattern */
		*p ++ = 0x267e;
		*p ++ = 0xc252;
		lantdp-> ctrl = 0xe00005ee;
		lantdp++->owner = 0x80000000;
	}
	ks8692_write(KS8692_WAN_DMA_TX_START, 0x01);
	ks8692_write(KS8692_LAN_DMA_TX_START, 0x01);
	wantdp = &ks8692_wan_tx[0];
	lantdp = &ks8692_lan_tx[0];
	while ( !ctrlc()) {
	/* check if any tx done, replenish it */
		if (!(wantdp->owner & 0x80000000)) {
			wantdp->owner = 0x80000000;
			ks8692_write(KS8692_LAN_DMA_TX_START, 0x01);
			if (++wantxindex >= TXDESCS)
				wantxindex = 0;
			wantdp = &ks8692_wan_tx[wantxindex];
		}
		if (!(lantdp->owner & 0x80000000)) {
			lantdp->owner = 0x80000000;
			ks8692_write(KS8692_LAN_DMA_TX_START, 0x01);
			if (++lantxindex >= TXDESCS)
				lantxindex = 0;
			lantdp = &ks8692_lan_tx[lantxindex];
		}
	}
#endif
	return 0;
}


#if defined(CONFIG_MII) || (CONFIG_COMMANDS & CFG_CMD_MII)

extern int  hwReadSTA (u32, u32, u16 *);
extern int  hwWriteSTA (u32, u32, u16);

int  ks8692_miiphy_read
(
     char *devname, 
     unsigned char addr,
     unsigned char reg, 
     unsigned short * value
)
{
     return( hwReadSTA (reg, addr, value) );
}

int  ks8692_miiphy_write
(
     char *devname, 
     unsigned char addr,
     unsigned char reg, 
     unsigned short value
)
{
     return( hwWriteSTA (reg, addr, value) );
}

int ks8692_miiphy_initialize(bd_t *bis)
{
     mii_init();
     miiphy_register("ks8692phy", ks8692_miiphy_read, ks8692_miiphy_write);
     return 0;
}

#endif	/* defined(CONFIG_MII) || (CONFIG_COMMANDS & CFG_CMD_MII) */

#ifdef CONFIG_NET_MULTI
int ksz8692_initialize (bd_t *bis)
{
	struct eth_device* dev;
	int dev_nr = 0;

	for ( dev_nr = 0; dev_nr < 2; dev_nr++ ) {
		dev = (struct eth_device*) malloc(sizeof *dev);
		sprintf(dev->name, "ksznet#%d", dev_nr);

		if ( !dev_nr )
			dev->iobase = KS8692_WAN_DMA_TX;
		else
			dev->iobase = KS8692_LAN_DMA_TX;

		/*
		 * Setup device structure and register the driver.
		 */
		dev->init = ksz_init;
		dev->halt = ksz_halt;
		dev->send = ksz_send;
		dev->recv = ksz_recv;

		eth_register(dev);
	}

	return dev_nr;
}
#endif

#endif	/* CONFIG_DRIVER_KS8692ETH */
