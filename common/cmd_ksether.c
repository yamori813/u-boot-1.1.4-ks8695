/*
 * (C) Copyright 2006 Micrel Semiconductor Inc., San Jose
 * David J. Choi <david.choi@micrel.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Ether routines
 */

#include <common.h>

#ifdef CONFIG_KSETHER

#include <command.h>
#include <asm/processor.h>
#include <asm/io.h>
#include <asm/u-boot.h>
#include <net.h>
#include "cmd_ksether.h"


#if (CONFIG_COMMANDS & CFG_CMD_KSETHER)

#if (0)
#define TEST_KS7695
#endif

/* Ether Configuration Space access commands
 *
 * Syntax:
 *	ether info
 *	ether send port buf_no buf_len count same_buf delay
 *	ether reset port
 *	ether traf port
 *	ether dump port
 */


static char *pTxBuf = ( char* )( TEST_BUFFER_START );

extern void eth_buffer_link(uint BufNum,uint BufLen,uint SameBuf);
extern int eth_send_fast(uint BufLen, uint cnt);
extern int eth_rx_fast(void);
extern struct net_device_stats ether_net_stats[2];
extern void get_ks8695_geninfo( struct genInfo *buf);
extern void eth_halt_cmd(void);
extern int eth_init_cmd(uint);
extern void eth_link_status(struct eth_linksts *);
extern void eth_pktdump_update(uint port,uint flag);
extern void get_mib_reset(void);
extern void get_mib_count( struct mib_table *ptr);

static int etherProcReceive(uint Port);
static int etherProcPing(uint Port);
static int etherProcReset(uint port);
static int etherProcTraffic(uint port);



static int etherProcFastTx(uint Port,uint BufNum,uint BufLen,uint RepeatCount,uint SameBuf)
{
   	int 	status;
	int	retv=CMD_FAIL;
	uint	cnt=0, txcnt=0;
	struct 	eth_linksts	ethbuf;

	memset(&ethbuf,0x00,sizeof(ethbuf));
/*
printf("%s: Port=%d, BufNum=%d BufLen=0x%x RepeatCount=%d SameBuf=%d\n",
	__FUNCTION__, Port, BufNum, BufLen, RepeatCount, SameBuf);
*/
	if( Port !=  LAN_PORT && Port != WAN_PORT )
	{
		printf("Port type is wrong\n");
		return retv;
	}

	eth_link_status(&ethbuf);

	do {
		if ( ethbuf.wanport && Port == WAN_PORT ) break;
		if ( (ethbuf.lanport[0] || ethbuf.lanport[1] || ethbuf.lanport[2] || ethbuf.lanport[3])
			&& (Port == LAN_PORT )) break;

		printf("No physical connection\n");
		return retv;
	
	} while(0);

	if( BufLen > MAX_ETHER_PACKET )
	{
		printf("Buffer length is too long. Max Buffer length is 0x%x\n",MAX_ETHER_PACKET);
		return retv;
	}

	if( BufLen < DEF_BUF_LEN )
		BufLen = DEF_BUF_LEN;

	if( BufNum >= TEST_BUFFER_COUNT )
	{
		printf("Max BufferNumber  is 0x%x\n",TEST_BUFFER_COUNT-1);
		return retv;
	}

	if( RepeatCount > MAX_REPEAT_COUNT )
	{
		printf("Max RepeatCount is 0x%x[%d]\n",MAX_REPEAT_COUNT,MAX_REPEAT_COUNT);
		return retv;
	}

	if( !RepeatCount ) RepeatCount=1;

	eth_init_cmd(Port);

	//printf("%s:Passed eth_init_send\n",__FUNCTION__);
	udelay(100000);

	eth_buffer_link(BufNum,BufLen,SameBuf);

	retv=CMD_SUCCESS;

	do {
		cnt = (RepeatCount >= TXDESCS) ? TXDESCS : RepeatCount ;
		status = eth_send_fast(BufLen, cnt );
		
		txcnt += cnt;
		RepeatCount -= cnt;
//printf("Remaining count=0x%x sent-count=0x%x curr-cnt=0x%x\n", RepeatCount, txcnt, cnt);

		if( ctrlc() )
		{
			retv=CMD_FAIL;
			break;
		}

   	} while ( RepeatCount > 0 );

	udelay(100000);		//100ms
	eth_halt_cmd();

	if(retv == CMD_SUCCESS)
		printf("Total sending Packet%s = 0x%x\n",(txcnt > 1) ? "s" :"", txcnt);
	else 
		printf("Abort the command\n");

	return retv;
}

static int etherProcSend(uint Port,uint BufNum,uint BufLen,uint RepeatCount,uint SameBuf,uint Delay)
{
   	//int 	i;
   	int 	status;
	int	retv=CMD_FAIL;
	uint	txcnt=0;
	struct 	eth_linksts	ethbuf;
    int     dstport = 0x0;


	//struct genInfo	buf;
	memset(&ethbuf,0x00,sizeof(ethbuf));

	if( Port !=  LAN_PORT && Port != WAN_PORT )
	{
		printf("Port type is wrong\n");
		return retv;
	}

	eth_link_status(&ethbuf);

	do {
		if ( ethbuf.wanport && Port == WAN_PORT ) break;
		if ( (ethbuf.lanport[0] || ethbuf.lanport[1] || ethbuf.lanport[2] || ethbuf.lanport[3])
			&& (Port == LAN_PORT )) break;

		printf("No physical connection\n");
		return retv;
	
	} while(0);

	if( BufLen > TEST_BUFSIZ )
	{
		printf("Buffer length is too long. Max Buffer length is 0x%x\n",TEST_BUFSIZ);
		return retv;
	}
#ifndef TEST_KS7695
	if( BufLen < DEF_BUF_LEN )
		BufLen = DEF_BUF_LEN;
#endif

	if( BufNum >= TEST_BUFFER_COUNT )
	{
		printf("Max BufferNumber  is 0x%x\n",TEST_BUFFER_COUNT-1);
		return retv;
	}

	if( RepeatCount > MAX_REPEAT_COUNT )
	{
		printf("Max RepeatCount is 0x%x[%d]\n",MAX_REPEAT_COUNT,MAX_REPEAT_COUNT);
		return retv;
	}

	if( Delay > MAX_DELAY_COUNT )
	{
		printf("Max DelayTime is 0x%x[%d] usec\n",MAX_DELAY_COUNT,MAX_DELAY_COUNT);
		return retv;
	}

	if( !RepeatCount ) RepeatCount=1;

	eth_init_cmd(Port);

	udelay(10000);

	etherProcReset(Port);
	retv=CMD_SUCCESS;

	do {
#ifndef TEST_KS7695
		status = eth_send ( pTxBuf + (BufNum * TEST_BUFSIZ), BufLen);
#else
		status = eth_send_port ( pTxBuf + (BufNum * TEST_BUFSIZ), BufLen, dstport);
#endif

       		/* Find next buffer to transmit */
       		if ( !SameBuf )
           		BufNum = (BufNum+1) % TEST_BUFFER_COUNT;

		if( ctrlc() )
		{
			retv=CMD_FAIL;
			break;
		}

       		/* Delay in ms if requested */
       		if ( (Delay > 0 ) && (RepeatCount > 1)  ) udelay ( Delay );
   	} while ( --RepeatCount > 0 );

	udelay(100000);		//100ms
	eth_halt_cmd();

#if (0)
	if(retv == CMD_SUCCESS)
		retv = etherProcTraffic(Port);
	else 
		printf("Abort the command\n");
#endif

	return retv;
}

static int etherProcReceive(uint Port)
{
	int	retv=CMD_FAIL;

	struct eth_linksts	ethbuf;

	//struct genInfo	buf;
	memset(&ethbuf,0x00,sizeof(ethbuf));

	if( Port !=  LAN_PORT && Port != WAN_PORT )
	{
		printf("Port type is wrong\n");
		return retv;
	}

	eth_link_status(&ethbuf);
	do {
		if ( ethbuf.wanport && Port == WAN_PORT ) break;
		if ( (ethbuf.lanport[0] || ethbuf.lanport[1] || ethbuf.lanport[2] || ethbuf.lanport[3])
			&& (Port == LAN_PORT )) break;

		printf("No physical connection\n");
		return retv;
	
	} while(0);

	eth_init_cmd(Port);
	udelay(1000);

	if (NetOurIP == 0) {
		extern unsigned char default_eth_mac[];
		NetOurIP = getenv_IPaddr ("ipaddr");
		memcpy(NetOurEther,default_eth_mac,6);
		printf("Use default IP=0x%08x\n",NetOurIP);
	}
	etherProcReset(Port);
	while( 1 )	//wait until ctrl-C
	{
		eth_rx_fast();
		if( ctrlc() ) break;
   	} 

	eth_halt_cmd();
	retv = etherProcTraffic(Port);
	return retv;
}

static int etherProcPing(uint Port)
{
	int	retv=CMD_FAIL;
	struct eth_linksts	ethbuf;

	memset(&ethbuf,0x00,sizeof(ethbuf));

	if( Port !=  LAN_PORT && Port != WAN_PORT )
	{
		printf("Port type is wrong\n");
		return retv;
	}

	eth_link_status(&ethbuf);
	do {
		if ( ethbuf.wanport && Port == WAN_PORT ) break;
		if ( (ethbuf.lanport[0] || ethbuf.lanport[1] || ethbuf.lanport[2] || ethbuf.lanport[3])
			&& (Port == LAN_PORT )) break;

		printf("No physical connection\n");
		return retv;
	
	} while(0);

	eth_init_cmd(Port);
	udelay(1000);

	if (NetOurIP == 0) {
		extern unsigned char default_eth_mac[];
		NetOurIP = getenv_IPaddr ("ipaddr");
		memcpy(NetOurEther,default_eth_mac,6);
		printf("Use default IP=0x%08x\n",NetOurIP);
	}
	etherProcReset(Port);
	printf("Ready to respond to ping command...");
	while( 1 )	//wait until ctrl-C
	{
		eth_rx();
		if( ctrlc() ) break;
	}
	printf( "\n" );

	eth_halt_cmd();
	retv = etherProcTraffic(Port);
	return retv;
}

//Reset software DB containing statistical information
static int etherProcReset(uint port)
{
	int	retv=CMD_FAIL;

	if( port > ALL_PORT )
	{
		printf("Port number is wrong\n");
		return retv;
	}

	if( port == ALL_PORT )
	{
		memset((char *)&ether_net_stats[0],0x00,sizeof(struct net_device_stats)*2);
		port = 0;
	}
	else
	{
		memset((char *)&ether_net_stats[port],0x00,sizeof(struct net_device_stats));
	}
	get_mib_reset();
	printf("Success to clear Ethernet Statistics\n");
	return CMD_SUCCESS;
}

static int etherProcTraffic(uint port)
{

	int	portcnt=1;
	int	retv=CMD_FAIL;
	int	index=0, found=0;
	struct mib_table mibtable;
	uint	*uptr=(uint *)&mibtable;

	static char *MIBname[] = {
		"RxLoPriorityByte  ",	//00
		"RxHiPriorityByte  ",	//1
		"RxUndersizePkt    ",	//2
		"RxFragments       ",	//3
		"RxOversize        ",	//4
		"RxJabbers         ",	//5
		"RxSymbolError     ",	//6
		"RxCRCerror        ",	//7
		"RxAlignmentError  ",	//8
		"RxControl8808Pkts ",	//9
		"RxPausePkts       ",	//a
		"RxBroadcast       ",	//b
		"RxMulticast       ",	//c
		"RxUnicast         ",	//d
		"Rx64Octets        ",	//e
		"Rx65to127Octets   ",	//f
		"Rx128to255Octets  ",	//0x10
		"Rx256to511Octets  ",	//0x11
		"Rx512to1023Octets ",	//0x12
		"Rx1024to1522Octets",	//0x13

		"TxLoPriorityByte  ",	//0x14
		"TxHiPriorityByte  ",	//0x15
		"TxLateCollision   ",	//0x16
		"TxPausePkts       ",	//0x17
		"TxBroadcastPkts   ",	//0x18
		"TxMulticastPkts   ",	//0x19
		"TxUnicastPkts     ",	//0x1a
		"TxDeferred        ",	//0x1b
		"TxTotalCollision  ",	//0x1c
		"TxExcessiveCollision",	//0x1d
		"TxSingleCollision  ",	//0x1e
		"TxMultipleCollision",	//0x1f

		"LanTxDrop0         ",
		"LanTxDrop1         ",
		"LanTxDrop2         ",
		"LanTxDrop3         ",
		"LanTxDropALL       ",
		"LanRxDrop0         ",
		"LanRxDrop1         ",
		"LanRxDrop2         ",
		"LanRxDrop3         ",
		"LanRxDropALL       ",
		'\0'
	};

	if( (port > ALL_PORT) || port == NONE_PORT )
	{
		printf("Port type is wrong\n");
		return retv;
	}

	if( port == ALL_PORT )
	{
		portcnt=2;
		port = 0;
	}

	while( portcnt )
	{
		printf("<<< %s Port Statistics >>>\n", (port == WAN_PORT) ? "WAN" : "LAN");
		printf("TX-------\n");
        	printf("\ttx_packets      = 0x%x[%d]\n", 
				(uint)STAT_NET(port,tx_packets), (uint)STAT_NET(port,tx_packets));
        	printf("\ttx_bytes        = 0x%x\n", (uint)STAT_NET(port,tx_bytes));

        	printf("RX-------\n");
        	printf("\trx_packets      = 0x%x[%d]\n", 
				(uint)STAT_NET(port,rx_packets), (uint)STAT_NET(port,rx_packets));
        	printf("\trx_bytes        = 0x%x\n", (uint)STAT_NET(port,rx_bytes));
/*
        	printf("\trx_dropped      = 0x%x\n", (uint)STAT_NET(port,rx_dropped));
        	printf("\trx_errors       = 0x%12x\n", (uint)STAT_NET(port,rx_errors));
        	printf("\trx_length_errors= 0x%12x\n", (uint)STAT_NET(port,rx_length_errors));
        	printf("\trx_crc_errors   = 0x%12x\n", (uint)STAT_NET(port,rx_crc_errors));
        	printf("\tcollisions      = 0x%12x\n", (uint)STAT_NET(port,collisions));
        	printf("\tmulticast       = 0x%12x\n", (uint)STAT_NET(port,multicast));
        	printf("\trx_missed_errors= 0x%12x\n", (uint)STAT_NET(port,rx_missed_errors));
        	printf("\trx_length_errors= 0x%12x\n", (uint)STAT_NET(port,rx_length_errors));

*/
		printf("\n");
		portcnt--;port++;
	}

	get_mib_count(&mibtable);
	printf("MIB information last port\n");

	for(index=0; MIBname[index]; index++, uptr++)
	{
		//if ( *uptr ==0 ) continue;
		printf("%s:%lu\n",MIBname[index], *uptr);
		found=1;
	}
	//if( !found ) printf("All MIB counters are zero\n");
	return retv=CMD_SUCCESS;
}

static int etherProcDump(uint port)
{
	if( port == NONE_PORT )
	{
		eth_pktdump_update(port,0);
		printf("Stop Packet dump\n");
	}

	else if( port == ALL_PORT )
	{
		eth_pktdump_update(port,DEBUG_PACKET_LEN | DEBUG_PACKET_CONTENT);
		printf("Start Packet dump\n");
	}

	else
	{
		printf("Invalid port type\n");
		return CMD_FAIL;
	}

	return CMD_SUCCESS;
}

static int etherProcInfo(void)
{
	int	i;
	char	strbuf[20];
	struct genInfo	buf;
	struct eth_linksts	ethbuf;

	memset(&ethbuf,0x00,sizeof(ethbuf));
	memset(&buf,0x00,sizeof(buf));

	get_ks8695_geninfo( &buf );
	eth_link_status(&ethbuf);

	printf("<<< Ethernet General Information >>>\n");
	printf("  Device ID\t= 0x%x\n",ethbuf.deviceid);
	strbuf[0]='\0';
	if( ethbuf.wanport ) strcat(strbuf,"WAN ");
	if( ethbuf.lanport[0] || ethbuf.lanport[1] || ethbuf.lanport[2] || ethbuf.lanport[3] )
		strcat(strbuf,"LAN");
	printf("  Connection\t= %s\n",strbuf[0] != '\0' ? strbuf : "None");
	if( strbuf[0] != '\0')
	{
		printf("  Link Status\t= ");
		if (ethbuf.wanport) printf("WAN:On ");
		for(i=0;i<4;i++)
		{
			if( ethbuf.lanport[i] ) printf("LAN%d:On ",i);
		}
		printf("\n");
	}

	printf("  TxDescriptors\t= 0x%x\n",buf.txdesc);
	printf("  RxDescriptors\t= 0x%x\n",buf.rxdesc);
	printf("  TxDesc start\t= 0x%x\n",buf.txdesc_addr);
	printf("  RxDesc start\t= 0x%x\n",buf.rxdesc_addr);
	printf("  Desc size\t= 0x%x\n",buf.bufsize);

	for(i=0;i<TEST_BUFFER_COUNT;i++)
	{
		printf("  Buffer[%d]\t= 0x%x\n", i, pTxBuf+TEST_BUFSIZ*i );
	}
	return CMD_SUCCESS;
}

int do_ether (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char	op=0;
	uint	port=0;
	uint	buf_no=DEF_BUF_NO;
	uint	buf_len=DEF_BUF_LEN;
	uint	count=DEF_COUNT;
	uint	same_buf=DEF_SAME_BUF;
	uint	dly=DEF_DELAY;	
	int	retvalue=CMD_SUCCESS;

        /*
         * We use the last specified parameters, unless new ones are
         * entered.
         */

	if (argc > 1)
	{
		struct eth_linksts	ethbuf;
		memset(&ethbuf,0x00,sizeof(ethbuf));

		op = argv[1][0];

		eth_link_status(&ethbuf);

		//Fill the default value for port.
		if (ethbuf.wanport)
			port  = WAN_PORT;
		else
			port = LAN_PORT;

		//start to overwrite parameters value, only if there is input.
		if( argc > 2 ) 
		{
			char	pid=argv[2][0];

			if( pid == 'a' ) 
				port  = ALL_PORT;
			else if( pid == 'n' ) 
				port  = NONE_PORT;
			else if( pid == 'l' ) 
				port  = LAN_PORT;
			else if( pid == 'w' ) 
				port  = WAN_PORT;
			else
			{
				printf("Invalid port type\n");
				printf ("Usage:\n%s\n", cmdtp->usage);
				return retvalue=CMD_FAIL;
			}
		}

		if( argc > 3  && (op == 's' || op == 'f') )
			buf_no  = simple_strtoul(argv[3],NULL,16);
		if( argc > 4  && (op == 's' || op == 'f') )
			buf_len  = simple_strtoul(argv[4],NULL,16);
		if( argc > 5  && (op == 's' || op == 'f') )
			count  = simple_strtoul(argv[5],NULL,16);
		if( argc > 6  && (op == 's' || op == 'f') )
			same_buf  = simple_strtoul(argv[6],NULL,16);
		if( argc > 7  && (op == 's' || op == 'f') )
			dly  = simple_strtoul(argv[7],NULL,16);
		//end of overwrite.
	}

	switch (op) {
		case 's':		/* send packets */
			retvalue = etherProcSend(port,buf_no,buf_len,count,same_buf,dly);
			break;
		case 'r':		/* receive packets without reply for highspeed reception */
			retvalue = etherProcReceive(port);
			break;
		case 'p':		/* reply to ping request */
			retvalue = etherProcPing(port);
			break;
		case 'c':		/* clear */
			retvalue = etherProcReset(port);
			break;
		case 't':		/* traffic(statistics) display */
			retvalue = etherProcTraffic(port);
			break;
		case 'd':		/* packet dump */
			retvalue = etherProcDump(port);
			break;
		case 'i':		/* info */
			retvalue = etherProcInfo();
			break;
		case 'f':		/* fast packet transmission */
			retvalue = etherProcFastTx(port,buf_no,buf_len,count,same_buf);
			break;
		default:
			retvalue = CMD_FAIL;
			break;
	}

	if( retvalue == CMD_FAIL )
	{
		printf ("Usage:\n%s\n", cmdtp->usage);
	}

	return retvalue;
}

/***************************************************/

U_BOOT_CMD (
	ether, 8, 1, do_ether,
	"ether   - Ethernet port info and send/receive control\n",
	"\n"
	"    - show ethernet port information and buffer address\n"
	"ether clear port[wan|lan|all]\n"
	"    - clear statistics\n"
	"ether dump port[all|none]\n"
	"    - dump tx/rx packets\n"
	"ether fptx port[wan|lan] buf_no buf_len count same_buf\n"
	"    - fast packet transmission, default is ether fptx connected-port 0 3c 1 0\n"
	"ether info\n"
	"    - show general information\n"
	"ether pong port[wan|lan]\n"
	"    - send pong packets responding to ping packets from lan or wan port\n"
	"ether recv port[wan|lan]\n"
	"    - receive packets from lan or wan port\n"
	"ether send port[wan|lan] buf_no buf_len count same_buf delay\n"
	"    - send packets, default is ether send connected-port 0 3c 1 0 0\n"
	"ether traf port[wan|lan|all]\n"
	"    - show ethernet statistics\n"
);


#endif /* (CONFIG_COMMANDS & CFG_CMD_KSETHER) */

#endif /* CONFIG_KSETHER */
