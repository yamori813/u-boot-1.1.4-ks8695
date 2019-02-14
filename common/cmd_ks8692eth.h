struct mib_table
{
	uint	RxLoPriorityByte;	//00
	uint	RxHiPriorityByte;	//1
	uint	RxUndersizePkt;		//2
	uint	RxFragments;		//3
	uint	RxOversize;		//4
	uint	RxJabbers;		//5
	uint	RxSymbolError;		//6
	uint	RxCRCerror;		//7
	uint	RxAlignmentError;	//8
	uint	RxControl8808Pkts;	//9
	uint	RxPausePkts;		//a
	uint	RxBroadcast;		//b
	uint	RxMulticast;		//c
	uint	RxUnicast;		//d
	uint	Rx64Octets;		//e
	uint	Rx65to127Octets;	//f
	uint	Rx128to255Octets;	//0x10
	uint	Rx256to511Octets;	//0x11
	uint	Rx512to1023Octets;	//0x12
	uint	Rx1024to1522Octets;	//0x13

	uint	TxLoPriorityByte;	//0x14
	uint	TxHiPriorityByte;	//0x15
	uint	TxLateCollision;	//0x16
	uint	TxPausePkts;		//0x17
	uint	TxBroadcastPkts;	//0x18
	uint	TxMulticastPkts;	//0x19
	uint	TxUnicastPkts;		//0x1a
	uint	TxDeferred;		//0x1b
	uint	TxTotalCollision;	//0x1c
	uint	TxExcessiveCollision;	//0x1d
	uint	TxSingleCollision;	//0x1e
	uint	TxMultipleCollision;	//0x1f

	uint	LanTxDrop0;
	uint	LanTxDrop1;
	uint	LanTxDrop2;
	uint	LanTxDrop3;
	uint	LanTxDrop;
	uint	LanRxDrop0;
	uint	LanRxDrop1;
	uint	LanRxDrop2;
	uint	LanRxDrop3;
	uint	LanRxDrop;
};

struct eth_linksts
{
	uint	deviceid;
	uint	wanport;
	uint	lanport;
};

struct net_device_stats
{
        uint   rx_packets;             /* total packets received       */
        uint   tx_packets;             /* total packets transmitted    */
        uint   rx_bytes;               /* total bytes received         */
        uint   tx_bytes;               /* total bytes transmitted      */
        uint   rx_errors;              /* bad packets received         */
        uint   tx_errors;              /* packet transmit problems     */
        uint   rx_dropped;             /* no space in linux buffers    */
        uint   tx_dropped;             /* no space available in linux  */
        uint   multicast;              /* multicast packets received   */
        uint   collisions;

        /* detailed rx_errors: */
        uint   rx_length_errors;
        uint   rx_over_errors;         /* receiver ring buff overflow  */
        uint   rx_crc_errors;          /* recved pkt with crc error    */
        uint   rx_frame_errors;        /* recv'd frame alignment error */
        uint   rx_fifo_errors;         /* recv'r fifo overrun          */
        uint   rx_missed_errors;       /* receiver missed packet       */

        /* detailed tx_errors */
        uint   tx_aborted_errors;
        uint   tx_carrier_errors;
        uint   tx_fifo_errors;
        uint   tx_heartbeat_errors;
        uint   tx_window_errors;

        /* for cslip etc */
        uint   rx_compressed;
        uint   tx_compressed;
};


struct genInfo
{
	char	portName[8];
	int	txdesc;
	int	rxdesc;
	uint	txdesc_addr;
	uint	rxdesc_addr;
	uint	bufstart;
	uint	bufsize;
};


#define MAX_PHY_DEVICE    2
struct phyInfo
{
	unsigned char	addr;
    int             id;
};

struct phyDev
{
	char	devname[20];
	struct phyInfo	phy[MAX_PHY_DEVICE];
};


#define	STAT_NET(p,x)     (ether_net_stats[p].x)
#define	uint		unsigned int

#define	WAN_PORT	0	//WAN port ID
#define	LAN_PORT	1	//LAN port ID
#define	ALL_PORT	2
#define	NONE_PORT	3
#define	MAX_PORT	NONE_PORT

#define	INBOUND		1
#define	OUTBOUND	0
#define	DEBUG_PACKET_LEN	0x01
#define	DEBUG_PACKET_CONTENT	0x02

#define TXDESCS         16
#define RXDESCS         16
#define BUFSIZE         2048
#define	TEST_BUFFER_START	(UBOOT_LOAD_RAMDISK + 0x3ff) & ~0x3ff

#define	TEST_BUFFER_COUNT	4
#define	TEST_BUFSIZ		0x20000
#define	MAX_TEST_BUFSIZ	(TEST_BUFFER_COUNT * TEST_BUFSIZ)
#define	MAX_REPEAT_COUNT	0x100000
#define	MAX_ETHER_PACKET	1514		//in bytes
#define	MAX_DELAY_COUNT		0x1000000

#define	CMD_SUCCESS	0
#define	CMD_FAIL	1 

#define	DEF_BUF_NO	0
#define	DEF_BUF_LEN	0x3c
#define	DEF_COUNT	1
#define	DEF_SAME_BUF	0
#define DEF_DELAY	0




	
