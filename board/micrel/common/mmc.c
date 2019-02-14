/*
 * Copyright (C) 2006 Micrel, Inc.
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

 * MMC initialize\read\write throught Micrel KS8692 SD/SDIO Host Controller.
 * SD/SDIO Host Controller is interface with SD memory card.
 */

#include <config.h>
#include <common.h>
#include <malloc.h>
#include <asm/errno.h>
#include <mmc.h>
#include <part.h>
#include <asm/arch/ks8692Reg.h>
#include <asm/arch/platform.h>

#if (CONFIG_COMMANDS & CFG_CMD_MMC)

/* SD host didn't issue transfer complete interrupt if SD read data (DMA mode) are operation with 
   Ethernet LAN traffic at the same time. due to SD and LAN port are share with same DAM channel.*/
#if (0)
#define SD_TIMEOUT_PROBLEM  
#endif

/* for Uboot */

#define CFG_MMC_BASE   0x20000000

#if (1)
#define TEST_MULTI_BLOCK 1
#endif

struct request;
struct mmc_data;
struct mmc_request;

struct mmc_command {
	u32			opcode;
	u32			arg;
	u32			resp[4];
	unsigned int		flags;		/* expected response type */
#define MMC_RSP_NONE	(0ul << 0)
#define MMC_RSP_SHORT	(1ul << 0)
#define MMC_RSP_LONG	(2ul << 0)
#define MMC_RSP_MASK	(3ul << 0)
#define MMC_RSP_CRC     (1ul << 3)	/* expect valid crc */
#define MMC_RSP_BUSY	(1ul << 4)	/* card may send busy */
#define MMC_RSP_OPCODE	(1ul << 5)	

/*
 * These are the response types, and correspond to valid bit
 * patterns of the above flags.  One additional valid pattern
 * is all zeros, which means we don't expect a response.
 */
#define MMC_RSP_R1	(MMC_RSP_SHORT|MMC_RSP_CRC)
#define MMC_RSP_R1B	(MMC_RSP_SHORT|MMC_RSP_CRC|MMC_RSP_BUSY)
#define MMC_RSP_R2	(MMC_RSP_LONG|MMC_RSP_CRC)
#define MMC_RSP_R3	(MMC_RSP_SHORT)

	unsigned int		retries;	/* max number of retries */
	unsigned int		error;		/* command error */

#define MMC_ERR_NONE	0
#define MMC_ERR_TIMEOUT	1
#define MMC_ERR_BADCRC	2
#define MMC_ERR_FIFO	3
#define MMC_ERR_FAILED	4
#define MMC_ERR_INVALID	5

	struct mmc_data		*data;		/* data segment associated with cmd */
	struct mmc_request	*mrq;		/* assoicated request */
};

struct mmc_data {
	unsigned int		timeout_ns;	/* data timeout (in ns, max 80ms) */
	unsigned int		timeout_clks;	/* data timeout (in clocks) */
	unsigned int		blksz_bits;	/* data block size */
	unsigned int		blocks;		/* number of blocks */
	struct request		*req;		/* request structure */
	unsigned int		error;		/* data error */
	unsigned int		flags;

#define MMC_DATA_WRITE	(1 << 8)
#define MMC_DATA_READ	(1 << 9)
#define MMC_DATA_STREAM	(1 << 10)

	unsigned int        bytes_xfered;
	char               *buffer;

	struct mmc_command	*stop;		/* stop command */
	struct mmc_request	*mrq;		/* assoicated request */
};

struct mmc_request {
	struct mmc_command	*cmd;
	struct mmc_data		*data;
	struct mmc_command	*stop;

	void                *done_data;	/* completion data */
	void               (*done)(struct mmc_request *);/* completion function */
};


struct uboot_sdhci_host;
typedef struct uboot_mmc_host 
{
    struct uboot_sdhci_host  *host;          /* host SD device */

    unsigned int        f_min;
    unsigned int        f_max;
    u32                 ocr_avail;

    /* host specific block data */
    unsigned int        max_seg_size;	/* see blk_queue_max_segment_size */
    unsigned short      max_hw_segs;	/* see blk_queue_max_hw_segments */
    unsigned short      max_phys_segs;	/* see blk_queue_max_phys_segments */
    unsigned short      max_sectors;	/* see blk_queue_max_sectors */
    unsigned short      unused;

	/* private data */
    u32                 ocr;            /* the current OCR setting */
} UBOOT_MMC_HOST;

typedef struct uboot_sdhci_host 
{
    UBOOT_MMC_HOST *mmc;        /* MMC structure */

    int             flags;      /* Host attributes */
#define SDHCI_USE_DMA		(1<<0)

    int             mode; 		/* Host connect to SD Memory */
#define MMC_MODE_SD		    (1<<0)

    int             bus_width; 	/* 0:1 bit; 2:4 bit */
#define SCR_BUS_WIDTH_4      2
#define SCR_BUS_WIDTH_1      0

    unsigned int    error;      /* command or data error */
    unsigned int    max_clk;    /* Max possible freq (MHz) */
    unsigned int    clock;      /* Current clock (MHz) */
    unsigned int    vdd_avail;  /* HC voltage support */
#define HC_VDD_18     0x00000040	/* VDD voltage 1.8 */
#define HC_VDD_30     0x00040000	/* VDD voltage 3.0 */
#define HC_VDD_33     0x00200000	/* VDD voltage 3.3 */

    struct mmc_request  *mrq;   /* Current request */
    struct mmc_command  *cmd;   /* Current command */
    struct mmc_data     *data;  /* Current data request */

    int             num_sg;     /* Entries left */
    int             offset;     /* Offset into current sg */
    int             remain;     /* Bytes left in current */
    int             size;       /* Remaining bytes in transfer */
    int             irq;        /* Device IRQ */
    int             bar;        /* PCI BAR index */
    int             dmaSize;    /* SDHCI DMA Buffer size (0:4KB,1:8KB,2:16kB,3:32KB,4:64KB,5:128KB,6:256KB,7:512KB)*/
    unsigned long   addr;       /* Bus address */
    unsigned long   ioaddr;     /* Mapped address */

} UBOOT_SDHCI_HOST;

#if (0)
#define  CONFIG_MMC_DEBUG
#endif
#define MMC_DEBUG

#ifdef MMC_DEBUG        /* if debugging driver */
#    define MMC_DBG_OFF           0x000000
#    define MMC_DBG_READ          0x000001
#    define MMC_DBG_WRITE         0x000002
#    define MMC_DBG_INIT          0x000004
#    define MMC_TX_PIO            0x000010
#    define MMC_HC_INIT           0x000020
#    define MMC_HC_P_DATA         0x000040
#    define MMC_HC_CMD            0x000080
#    define MMC_HC_INT            0x000100
#    define MMC_DISPLAY_READ      0x000200
#    define MMC_DISPLAY_WRITE     0x000400
#    define MMC_DISPLAY_SD_STATE  0x000800
#    define MMC_DBG_INT           0x001000
#    define MMC_DBG_ERROR         0x800000
#    define MMC_DBG_ALL           0xffffff

static int ksDebug = ( MMC_DBG_ERROR \
                     | MMC_DBG_INT | MMC_HC_CMD /*| MMC_DISPLAY_SD_STATE  | MMC_DBG_READ */);

#define ks_debug(FLG,X)     \
	    if (ksDebug & FLG) printf X;

#else /* MMC_DEBUG */

#define ks_debug(FLG,X)     \

#endif /* MMC_DEBUG */


/*
 * TO access KS8692 device.
 */
#define	ks_read(a)                ks8692_read_dword(a)    
#define	ks_read_w(a)              ks8692_read_word(a)	        
#define	ks_read_b(a)              ks8692_read_byte(a)	        

#define	ks_write(a,v)             ks8692_write_dword(a,v) 
#define	ks_write_w(a,v)           ks8692_write_word(a,v)       
#define	ks_write_b(a,v)           ks8692_write_byte(a,v)       

#define DRIVER_NAME         "sdhci"

#define MMC_BUS_WIDTH_4     4      /* 4bit data width */
#define MMC_BUS_WIDTH_1     1      /* 1bit data width */
#define DEFAULT_BUS_WIDTH   MMC_BUS_WIDTH_4

#if (0)
#define DEFAULT_MMC_CLOCK   12000000   /* 12MHz */      
#endif
#define DEFAULT_MMC_CLOCK   25000000   /* 25MHz */      


int mmcInitialized = 0;

/*
 * FIXME needs to read cid and csd info to determine block size
 * and other parameters
 */
UBOOT_SDHCI_HOST  *ghost;
static uchar mmc_buf[MMC_BLOCK_SIZE];
static mmc_csd_t mmc_csd;
static int mmc_ready = 0;
static u32 card_rca;

static block_dev_desc_t mmc_dev;

/* SD device current state from R1 response format */
static uchar *sd_statet[] = {"IDLE",
                             "READY", 
                             "IDENT", 
                             "STBY", 
                             "TRAN",
                             "DATA",
                             "RCV",
                             "PROG",
                             "DIS",
                             "UNKNOWN","UNKNOWN","UNKNOWN","UNKNOWN","UNKNOWN","UNKNOWN","UNKNOWN",
                              ""}; 

static uchar *file_format[] = {"Hard disk-like",
                              "DOS FAT", 
                              "Universal File Format", 
                              "Unknow", 
                              "Reserved",
                              ""}; 


/* Function protocol */
static void sdhci_dumpregs (UBOOT_SDHCI_HOST *);
static void sdhci_reset ( UBOOT_SDHCI_HOST *, u8 );
static void sdhci_set_clock (UBOOT_SDHCI_HOST *, unsigned int );
static UBOOT_SDHCI_HOST *sdhci_init ( void );
static void sdhci_activate_led ( UBOOT_SDHCI_HOST * );
static void sdhci_deactivate_led ( UBOOT_SDHCI_HOST * );
static void sdhci_tasklet_finish ( unsigned long );
static void sdhci_transfer_pio ( UBOOT_SDHCI_HOST * );
static void sdhci_prepare_data ( UBOOT_SDHCI_HOST *, struct mmc_data * );
static void sdhci_set_transfer_mode ( UBOOT_SDHCI_HOST *, struct mmc_data * );
static void sdhci_finish_data ( UBOOT_SDHCI_HOST * );
static void sdhci_send_command ( UBOOT_SDHCI_HOST *, struct mmc_command * );
static void sdhci_finish_command ( UBOOT_SDHCI_HOST * );
static void sdhci_request ( UBOOT_MMC_HOST *, struct mmc_request * );
static ulong sdhci_irq ( UBOOT_SDHCI_HOST *, int );
static void sdhci_cmd_irq ( UBOOT_SDHCI_HOST *, u32 );
static void sdhci_data_irq ( UBOOT_SDHCI_HOST *, u32 );

int   mmc_block_read ( uchar *, ulong, ulong, int );
int   mmc_read ( ulong, uchar *, int, int );
ulong mmc_bread ( int, ulong, ulong, ulong * );
int   mmc_block_write (ulong, uchar *, int, int );
int   mmc_write ( uchar *, ulong, int, int );
int   mmc_init ( int );
int   mmc_ident( block_dev_desc_t * );
int   mmc2info ( ulong );
#ifdef CONFIG_USE_IRQ
void  sd_isr (void);
void  sd_init_interrupt ( unsigned int );
#endif

extern int fat_register_device(block_dev_desc_t *dev_desc, int part_no);




block_dev_desc_t * mmc_get_dev(int dev)
{
    ks_debug(MMC_DBG_INIT,("%s: \n",__FUNCTION__));
    return ((block_dev_desc_t *)&mmc_dev);
}

/*****************************************************************************\
 *                                                                           *
 * KS8692 SD/SDIO Host Controller Low level functions                        *
 *                                                                           *
\*****************************************************************************/

static void sdhci_dumpregs
(
    UBOOT_SDHCI_HOST *host
)
{
    printf( DRIVER_NAME ": ============== REGISTER DUMP ==============\n");

    printf( DRIVER_NAME ": Sys addr(0x%08X): 0x%08x | Version (0x%08X): 0x%04x\n",
            SDHCI_DMA_ADDRESS, ks_read(host->ioaddr + SDHCI_DMA_ADDRESS),
            SDHCI_HOST_VERSION,ks_read_w(host->ioaddr + SDHCI_HOST_VERSION));
    printf( DRIVER_NAME ": Blk size(0x%08X): 0x%04x     | Blk cnt (0x%08X): 0x%04x\n",
            SDHCI_BLOCK_SIZE, ks_read_w(host->ioaddr + SDHCI_BLOCK_SIZE),
            SDHCI_BLOCK_COUNT,ks_read_w(host->ioaddr + SDHCI_BLOCK_COUNT));
    printf( DRIVER_NAME ": Argument(0x%08X): 0x%08x | Trn mode(0x%08X): 0x%04x\n",
            SDHCI_ARGUMENT, ks_read(host->ioaddr + SDHCI_ARGUMENT),
            SDHCI_TRANSFER_MODE, ks_read_w(host->ioaddr + SDHCI_TRANSFER_MODE));
    printf( DRIVER_NAME ": Command (0x%08X): 0x%04x     | Response(0x%08X): 0x%08x\n",
            SDHCI_COMMAND, ks_read_w(host->ioaddr + SDHCI_COMMAND),
            SDHCI_RESPONSE,ks_read(host->ioaddr + SDHCI_RESPONSE));
    printf( DRIVER_NAME ": Present (0x%08X): 0x%08x | Host ctl(0x%08X): 0x%02x\n",
            SDHCI_PRESENT_STATE, ks_read(host->ioaddr + SDHCI_PRESENT_STATE),
            SDHCI_HOST_CONTROL,  ks_read_b(host->ioaddr + SDHCI_HOST_CONTROL));
    printf( DRIVER_NAME ": Power   (0x%08X): 0x%02x       | Blk gap (0x%08X): 0x%02x\n",
            SDHCI_POWER_CONTROL, ks_read_b(host->ioaddr + SDHCI_POWER_CONTROL),
            SDHCI_BLOCK_GAP_CONTROL, ks_read_b(host->ioaddr + SDHCI_BLOCK_GAP_CONTROL));
    printf( DRIVER_NAME ": Wake-up (0x%08X): 0x%02x       | Clock   (0x%08X): 0x%04x\n",
            SDHCI_WALK_UP_CONTROL, ks_read_b(host->ioaddr + SDHCI_WALK_UP_CONTROL),
            SDHCI_CLOCK_CONTROL, ks_read_w(host->ioaddr + SDHCI_CLOCK_CONTROL));
    printf( DRIVER_NAME ": Timeout (0x%08X): 0x%02x       | Int stat(0x%08X): 0x%08x\n",
            SDHCI_TIMEOUT_CONTROL, ks_read_b(host->ioaddr + SDHCI_TIMEOUT_CONTROL),
            SDHCI_INT_STATUS, ks_read(host->ioaddr + SDHCI_INT_STATUS));
    printf( DRIVER_NAME ": Int enab(0x%08X): 0x%08x | Sig enab(0x%08X): 0x%08x\n",
            SDHCI_INT_ENABLE, ks_read(host->ioaddr + SDHCI_INT_ENABLE),
            SDHCI_SIGNAL_ENABLE, ks_read(host->ioaddr + SDHCI_SIGNAL_ENABLE));
    printf( DRIVER_NAME ": AC12 err(0x%08X): 0x%04x     | Slot int(0x%08X): 0x%04x\n",
            SDHCI_ACMD12_ERR, ks_read_w(host->ioaddr + SDHCI_ACMD12_ERR),
            SDHCI_SLOT_INT_STATUS, ks_read_w(host->ioaddr + SDHCI_SLOT_INT_STATUS));
    printf( DRIVER_NAME ": Caps    (0x%08X): 0x%08x | Max curr(0x%08X): 0x%08x\n",
            SDHCI_CAPABILITIES, ks_read(host->ioaddr + SDHCI_CAPABILITIES),
            SDHCI_MAX_CURRENT, ks_read(host->ioaddr + SDHCI_MAX_CURRENT));

    printf( DRIVER_NAME ": ===========================================\n");
}


static void sdhci_reset
(
    UBOOT_SDHCI_HOST *host, 
    u8 mask
)
{
    unsigned long timeout;

    ks_debug(/*MMC_HC_CMD*/ MMC_DBG_INIT ,("%s: mask=0x%x\n",__FUNCTION__, mask));

    ks_write_b(host->ioaddr + SDHCI_SOFTWARE_RESET, mask);

    if (mask & SDHCI_RESET_ALL) 
    {
        host->clock = 0;
    }

    /* Wait max 100 ms */
    timeout = 100;

    /* hw clears the bit when it's done */
    while (ks_read_b(host->ioaddr + SDHCI_SOFTWARE_RESET) & mask) {
        if (timeout == 0) {
            ks_debug(MMC_DBG_ERROR,
                    ("%s: Reset 0x%x never completed.\n", __FUNCTION__, (int)mask));
            sdhci_dumpregs(host);
            return;
        }
        timeout--;
        udelay(1000);
    }
}


static void sdhci_set_clock
(   
    UBOOT_SDHCI_HOST *host, 
    unsigned int      clock
)
{
    int div;
    u16 clk;
    int timeOut=10;


    ks_debug(MMC_DBG_INIT,("%s: clock=%d, max_clk=%d\n",__FUNCTION__, clock, host->max_clk));

    if (clock == host->clock)
        return;

    ks_write_w(host->ioaddr + SDHCI_CLOCK_CONTROL, 0);

    if (clock == 0)
       goto out;

    for (div = 1;div < 256;div *= 2) 
    {
        if ((host->max_clk / div) <= clock)
            break;
    }
    ks_debug(MMC_DBG_INIT,("%s: div=%d\n",__FUNCTION__, div));
    div >>= 1;

    clk = div << SDHCI_DIVIDER_SHIFT;
    clk |= SDHCI_CLOCK_INT_EN;
    ks_write_w(host->ioaddr + SDHCI_CLOCK_CONTROL, clk);

    ks_debug(MMC_DBG_INIT,("%s: clk=0x%x\n",__FUNCTION__, clk));

    /* Wait max 10 ms */
    do {
        udelay(1000);
        clk = ks_read_w(host->ioaddr + SDHCI_CLOCK_CONTROL);
    } while ( (timeOut-- >0) && !(clk & SDHCI_CLOCK_INT_STABLE) );

    if (!(clk & SDHCI_CLOCK_INT_STABLE))
    {
        ks_debug(MMC_DBG_ERROR,("%s: Internal clock never stabilised (clk=0x%x).\n",__FUNCTION__, clk));
        return;
    }

    clk |= SDHCI_CLOCK_CARD_EN;
    ks_write_w(host->ioaddr + SDHCI_CLOCK_CONTROL, clk);
    ks_debug(MMC_DBG_INIT,
            ("%s: %08X:%04x.\n",__FUNCTION__, 
             (host->ioaddr + SDHCI_CLOCK_CONTROL), ks_read_w(host->ioaddr + SDHCI_CLOCK_CONTROL )));

out:
   host->clock = clock;
}

static UBOOT_SDHCI_HOST *sdhci_init (void)
{
    UBOOT_SDHCI_HOST *host;
    unsigned int caps;
    u8 ctrl;
    u8 bus_width=0;
    unsigned int mmc_clock=0;
    unsigned int intmask;

#if (1)
    {
	u32 uReg;

    /* GPIO 6, 16 as output pin, GPIO 16 init to high, GPIO 6 init to low */
	uReg = ks8692_read_dword(KS8692_GPIO_MODE);
	uReg |= 0x00010040;    
	ks8692_write_dword(KS8692_GPIO_MODE, uReg);

	uReg = ks8692_read_dword(KS8692_GPIO_DATA);
	uReg |= 0x00010000;    
	ks8692_write_dword(KS8692_GPIO_DATA, uReg);

	uReg = ks8692_read_dword(KS8692_GPIO_DATA);
	uReg &= ~0x00000040;    
	ks8692_write_dword(KS8692_GPIO_DATA, uReg);

    }
#endif

    ks_debug(MMC_DBG_INIT,("%s: \n",__FUNCTION__ ));

    if (mmcInitialized != 0)
        return (ghost);

    /* only execute the following code once. */
    mmcInitialized++;

    host = malloc( sizeof(UBOOT_SDHCI_HOST) );
    if ( !host) 
       return (NULL);

    memset(host, 0, sizeof(UBOOT_SDHCI_HOST) );

    /* Sed IO base address to access SD/SDIO host controller */
    host->ioaddr = SDIO_HOST_BASE;
    ks_debug(MMC_HC_INIT,("%s: ioaddr=%08x\n",__FUNCTION__, host->ioaddr));

    /* Sed SD/SDIO host controller 4KB DMA buufer boundary */
    host->dmaSize = 0;

    /* Reset SD/SDIO host controller */
    sdhci_reset(host, SDHCI_RESET_ALL);

    /* 
     * Parameter initialization 
     */

    bus_width = DEFAULT_BUS_WIDTH;
    mmc_clock = DEFAULT_MMC_CLOCK;

    if (bus_width == MMC_BUS_WIDTH_4)
       host->bus_width = SCR_BUS_WIDTH_4;
    else
       host->bus_width = SCR_BUS_WIDTH_1;
  
    /* Read SD/SDIO host controller capabilities */
    caps = ks_read(host->ioaddr + SDHCI_CAPABILITIES);

    ks_debug(MMC_HC_INIT,("%s: caps=%08x\n",__FUNCTION__, caps));

    if (caps & SDHCI_CAN_DO_DMA) 
       host->flags |= SDHCI_USE_DMA;

#if (0)
    if (host->flags & SDHCI_USE_DMA) 
    {
        ks_debug(MMC_HC_INIT,("%s: DMA is not available in the uboot.\n", __FUNCTION__ ));
        host->flags &= ~SDHCI_USE_DMA; 
    }
#endif

    host->max_clk = (caps & SDHCI_CLOCK_BASE_MASK) >> SDHCI_CLOCK_BASE_SHIFT;
    host->max_clk *= 1000000;

    if (caps & 0x04000000) 
       host->vdd_avail |= HC_VDD_18;
    if (caps & 0x02000000) 
       host->vdd_avail |= HC_VDD_30;
    if (caps & 0x01000000) 
       host->vdd_avail |= HC_VDD_33;

    /* 
     * Host Controller initialization 
     */

    /* Set HC clock */ 
    sdhci_set_clock(host, DEFAULT_MMC_CLOCK);

    /* Set HC bus voltage */
    ctrl = 0x01;
    if (host->vdd_avail & HC_VDD_33)
        ctrl |= 0x0E;
    else if (host->vdd_avail & HC_VDD_30)
        ctrl |= 0x0C;
    else if (host->vdd_avail & HC_VDD_18)
        ctrl |= 0x0A;
    ks_write_b(host->ioaddr + SDHCI_POWER_CONTROL, ctrl);
    ks_debug(MMC_DBG_INIT,
            ("%s: %08X:%02x.\n",__FUNCTION__, 
             (host->ioaddr + SDHCI_POWER_CONTROL), ks_read_b(host->ioaddr + SDHCI_POWER_CONTROL )));

    /* Set HC data width */
    ctrl = ks_read_b(host->ioaddr + SDHCI_HOST_CONTROL);
    if (bus_width == MMC_BUS_WIDTH_4)
        ctrl |= SDHCI_CTRL_4BITBUS;
    else
       ctrl &= ~SDHCI_CTRL_4BITBUS;
    ks_write_b(host->ioaddr + SDHCI_HOST_CONTROL, ctrl);
    ks_debug(MMC_DBG_INIT,
            ("%s: %08X:%02x.\n",__FUNCTION__, 
             (host->ioaddr + SDHCI_HOST_CONTROL), ks_read_b(host->ioaddr + SDHCI_HOST_CONTROL )));

    intmask = (SDHCI_INT_NORMAL_MASK | SDHCI_INT_ERROR_MASK);
    intmask &= ~(SDHCI_INT_CARD_INT | SDHCI_INT_BUF_EMPTY | SDHCI_INT_BUF_FULL );

    /* Disable interrupts */
    ks_write(host->ioaddr + SDHCI_SIGNAL_ENABLE, 0);

    /* Enable interrupt status */
    ks_write(host->ioaddr + SDHCI_INT_ENABLE, intmask);

#ifdef CONFIG_USE_IRQ
    /* Initialize SD interrupt */
    //sd_init_interrupt (intmask);
#endif

    /* Clear interrupt status */
    ks_write(host->ioaddr + SDHCI_INT_STATUS, 0xffffffff);

    ks_debug(MMC_DBG_INIT,
            ("%s: intmask=%08x\n", __FUNCTION__, intmask ));
    ks_debug(MMC_DBG_INIT,
            ("%s: 0x%08X:%08x\n", __FUNCTION__, (host->ioaddr + SDHCI_INT_STATUS), ks_read(host->ioaddr + SDHCI_INT_STATUS)));
    ks_debug(MMC_DBG_INIT,
            ("%s: 0x%08X:%08x\n", __FUNCTION__, (host->ioaddr + SDHCI_INT_ENABLE), ks_read(host->ioaddr + SDHCI_INT_ENABLE)));
    ks_debug(MMC_DBG_INIT,
            ("%s: 0x%08X:%08x\n", __FUNCTION__, (host->ioaddr + SDHCI_SIGNAL_ENABLE), ks_read(host->ioaddr + SDHCI_SIGNAL_ENABLE)));

    /* This is unknown magic. */
    ks_write_b(host->ioaddr + SDHCI_TIMEOUT_CONTROL, 0x0E);

#ifdef CONFIG_MMC_DEBUG
    sdhci_dumpregs(host);
#endif

    ghost = host;

    return (ghost);
}


static void sdhci_activate_led
(
    UBOOT_SDHCI_HOST *host
)
{
    u8 ctrl;


    ctrl = ks_read_b(host->ioaddr + SDHCI_HOST_CONTROL);
    ctrl |= SDHCI_CTRL_LED;
    ks_write_b(host->ioaddr + SDHCI_HOST_CONTROL, ctrl);

    ks_debug(MMC_DBG_INIT,
            ("%s: %08X:%02x.\n",__FUNCTION__, 
             (host->ioaddr + SDHCI_HOST_CONTROL), ks_read_b(host->ioaddr + SDHCI_HOST_CONTROL )));
}

static void sdhci_deactivate_led
(
    UBOOT_SDHCI_HOST *host
)
{
    u8 ctrl;


    ctrl = ks_read_b(host->ioaddr + SDHCI_HOST_CONTROL);
    ctrl &= ~SDHCI_CTRL_LED;
    ks_write_b(host->ioaddr + SDHCI_HOST_CONTROL, ctrl);

    ks_debug(MMC_DBG_INIT,
            ("%s: %08X:%02x.\n",__FUNCTION__, 
             (host->ioaddr + SDHCI_HOST_CONTROL), ks_read_b(host->ioaddr + SDHCI_HOST_CONTROL )));
}

/*
 * sdhci_tasklet_finish - 
 *   The last step after issued CMD. 
 *   Reset SD Host Controller's CMD/DATA line if any error happen during issued CMD.
 */
static void sdhci_tasklet_finish
(
    unsigned long param
)
{
    UBOOT_SDHCI_HOST *host;
    struct mmc_request *mrq;

    host = (UBOOT_SDHCI_HOST *)param;
    mrq = host->mrq;

    ks_debug(/*MMC_HC_CMD*/MMC_DBG_INIT,("%s: Ending request, cmd:%d \n", __FUNCTION__, mrq->cmd->opcode));
    //printf("%s: host=%08x, mrq->data=%08x\n",__FUNCTION__, host, mrq->data);

    /*
     * The controller needs a reset of internal state machines
     * upon error conditions.
     */
    if ((mrq->cmd->error != MMC_ERR_NONE) ||
        (mrq->data && ((mrq->data->error != MMC_ERR_NONE) ||
        (mrq->data->stop && (mrq->data->stop->error != MMC_ERR_NONE))))) 
    {
        sdhci_dumpregs(host);
#ifdef PING_READY
        sdhci_reset(host, SDHCI_RESET_CMD);
        sdhci_reset(host, SDHCI_RESET_DATA);
#endif
    }

#if (0)
    host->mrq = NULL;
    host->cmd = NULL;
    host->data = NULL;
#endif

    sdhci_deactivate_led(host);

}

/*
 * sdhci_transfer_pio - 
 *   Read\Write data from\to SDHC register on DATA line.
 */
static void sdhci_transfer_pio
(
    UBOOT_SDHCI_HOST *host
)
{
    char  *buffer;
    u32   mask;
    int   bytes, size;
    int   timeOut=20;
    u32   present;


    ks_debug(MMC_TX_PIO,
             ("%s: transfer data size (0x%x) %s buffer (0x%x) \n", __FUNCTION__, 
               host->size, ((host->data->flags & MMC_DATA_READ)? "to":"from"), host->data->buffer ));

    if (host->size == 0)
       return;

    bytes = 0;
    if (host->data->flags & MMC_DATA_READ)
        mask = SDHCI_DATA_AVAILABLE;
    else
        mask = SDHCI_SPACE_AVAILABLE;

    buffer = host->data->buffer;
    buffer += host->offset;

	/* Transfer shouldn't take more than 5 s */

    while (host->size > 0) 
    {
        if (timeOut <= 0) 
        {
            ks_debug(MMC_DBG_ERROR,
                    ("%s: ERROR, PIO transfer stalled. Data not presend (%08X:%08x)\n", 
                      __FUNCTION__, (host->ioaddr + SDHCI_PRESENT_STATE), present ));

            host->data->error = MMC_ERR_FAILED;
            sdhci_finish_data(host);
            return;
        }

        present = ks_read(host->ioaddr + SDHCI_PRESENT_STATE);
        if (!(present & mask))
        {
            udelay(10);
            timeOut--;
            continue;
        }

        size = min(host->size, host->remain);
        ks_debug(MMC_TX_PIO,("%s: size=%d, host->size=%d, host->remain=%d\n",
                        __FUNCTION__, size, host->size,host->remain ));

        if (size >= 4) 
        {
            if (host->data->flags & MMC_DATA_READ)
                *(u32*)buffer = ks_read(host->ioaddr + SDHCI_BUFFER);
            else
                ks_write(host->ioaddr + SDHCI_BUFFER, *(u32*)buffer);
            size = 4;
        } 
        else if (size >= 2) 
        {
            if (host->data->flags & MMC_DATA_READ)
                *(u16*)buffer = ks_read_w(host->ioaddr + SDHCI_BUFFER);
            else
                ks_write_w(*(u16*)buffer, host->ioaddr + SDHCI_BUFFER);
            size = 2;
        } 
        else 
        {
            if (host->data->flags & MMC_DATA_READ)
                *(u8*)buffer = ks_read_b(host->ioaddr + SDHCI_BUFFER);
            else
                ks_write_b(*(u8*)buffer, host->ioaddr + SDHCI_BUFFER);
            size = 1;
        }

        buffer += size;
        host->offset += size;
        host->remain -= size;

        bytes += size;
        host->size -= size;
    }

    ks_debug(MMC_TX_PIO,("%s: PIO transfer: %d bytes, remain bytes %d\n", __FUNCTION__, bytes, host->remain));
}

/*
 * sdhci_prepare_data - 
 *   Write transmfer mode, data block size, data block count to the SDHC registers.
 */
static void sdhci_prepare_data
(
    UBOOT_SDHCI_HOST *host, 
    struct mmc_data  *data
)
{

    ks_debug(MMC_DBG_INIT,("%s: data pointer is %08x\n",__FUNCTION__, data ));

    if (data == NULL) 
    {
        ks_debug(MMC_DBG_INIT,("%s: exit, do nothing due to data pointer is null.\n",__FUNCTION__ ));
        return;
    }
		          
    ks_debug(MMC_HC_P_DATA,("%s: data->blksz_bits=%x, blksz=%04x, blks=%04x, flags=%08x\n",
             __FUNCTION__, data->blksz_bits, (1 << data->blksz_bits), data->blocks, data->flags));
    ks_debug(MMC_HC_P_DATA,("%s: tsac %d ms, nsac %d clk\n",
             __FUNCTION__, data->timeout_ns / 1000000, data->timeout_clks));

    if (host->flags & SDHCI_USE_DMA)
    {
        if (data->buffer == NULL)
        {
            ks_debug(MMC_DBG_ERROR,
                    ("%s: exit, do nothing due to data buffer pointer is null in DMA mode.\n",__FUNCTION__ ));
            return;
        }
        ks_write(host->ioaddr + SDHCI_DMA_ADDRESS, (u32) (data->buffer));
    }
    else
    {
        host->size = (1 << data->blksz_bits) * data->blocks;
        ks_debug(MMC_HC_P_DATA,("%s: host->size=%x\n", __FUNCTION__, host->size));

        host->offset = 0;
        host->remain = host->size;
        /*host->remain = host->cur_sg->length;*/
    }

    //ks_write_w(host->ioaddr + SDHCI_BLOCK_SIZE, 1 << data->blksz_bits);
    ks_write_w(host->ioaddr + SDHCI_BLOCK_SIZE, SDHCI_MAKE_BLKSZ(host->dmaSize, 1 << data->blksz_bits));
    ks_write_w(host->ioaddr + SDHCI_BLOCK_COUNT, data->blocks);
}

static void sdhci_set_transfer_mode
(
    UBOOT_SDHCI_HOST *host, 
    struct mmc_data  *data
)
{
    u16 mode;


    ks_debug(MMC_DBG_INIT,("%s: data pointer is %08x\n",__FUNCTION__, data ));

    if (data == NULL) {
       return;
    }

    mode = SDHCI_TRNS_BLK_CNT_EN;
    if (data->blocks > 1)
        mode |= SDHCI_TRNS_MULTI /*| SDHCI_TRNS_ACMD12 */;
    if (data->flags & MMC_DATA_READ)
        mode |= SDHCI_TRNS_READ;
    if (host->flags & SDHCI_USE_DMA)
        mode |= SDHCI_TRNS_DMA;

    ks_write_w(host->ioaddr + SDHCI_TRANSFER_MODE, mode);
}

/*
 * sdhci_finish_data - 
 *   Disable BUF_EMPTY and BUF_FULL interrupt. 
 *   If there is STOP CMD request, call sdhci_send_command() to send STOP CMD,
 *   otherwise, call sdhci_tasklet_finish() to finished current CMD house work. 
 */
static void sdhci_finish_data
(
    UBOOT_SDHCI_HOST *host
)
{
    struct mmc_data *data;
    u32 intmask;
    u16 blocks;


    ks_debug(MMC_DBG_INIT,("%s: \n",__FUNCTION__ ));

    data = host->data;

#if (0)
    host->data = NULL;
#endif

    if ( !(host->flags & SDHCI_USE_DMA) ) 
    {

#ifdef CONFIG_USE_IRQ
        /* Disable interrupt enable reg */
        intmask = ks_read(host->ioaddr + SDHCI_SIGNAL_ENABLE);
        intmask &= ~(SDHCI_INT_BUF_EMPTY | SDHCI_INT_BUF_FULL);
        ks_write(host->ioaddr + SDHCI_SIGNAL_ENABLE, intmask);
#endif

        /* Disable interrupt status reg */
        intmask = ks_read(host->ioaddr + SDHCI_INT_ENABLE);
        intmask &= ~(SDHCI_INT_BUF_EMPTY | SDHCI_INT_BUF_FULL);
        ks_write(host->ioaddr + SDHCI_INT_ENABLE, intmask);
    }

    /*
     * Controller doesn't count down when in single block mode.
     */
    if ((data->blocks == 1) && (data->error == MMC_ERR_NONE))
        blocks = 0;
    else
        blocks = ks_read_w(host->ioaddr + SDHCI_BLOCK_COUNT);
    data->bytes_xfered = (1 << data->blksz_bits) * (data->blocks - blocks);

    if ((data->error == MMC_ERR_NONE) && blocks) 
    {
        ks_debug(MMC_DBG_ERROR,("%s: ERROR, Controller signalled completion even though there were blocks left.\n", __FUNCTION__));
        data->error = MMC_ERR_FAILED;
    }

    if (host->size != 0) 
    {
        ks_debug(MMC_DBG_ERROR,("%s: ERROR, %d bytes were left untransferred.\n", __FUNCTION__, host->size));
        data->error = MMC_ERR_FAILED;
    }

    ks_debug(MMC_DBG_INIT,("%s: Ending data transfer (%d bytes)\n", __FUNCTION__, data->bytes_xfered));
    //ks_debug(MMC_HC_CMD,("%s: Ending data transfer (cmd=%d)\n", __FUNCTION__, host->cmd->opcode));

    //printf("%s: host=%08x, host->data=%08x, host->mrq=%08x, host->mrq->data=%08x\n",__FUNCTION__, 
    //        host, host->data, host->mrq, host->mrq->data);

    if (data->stop) 
    {
        ks_debug(MMC_DBG_INIT,("%s: data->stop=0x%x\n", __FUNCTION__, data->stop));

        /*
         * The controller needs a reset of internal state machines
         * upon error conditions.
         */
        if (data->error != MMC_ERR_NONE) 
        {
            sdhci_reset(host, SDHCI_RESET_CMD);
            sdhci_reset(host, SDHCI_RESET_DATA);
        }

        sdhci_send_command(host, data->stop);
     } 
     else
        sdhci_tasklet_finish((unsigned long)host);
}

/*
 * sdhci_request - 
 *   If card is not CMD and DATA inhibit, write argument, CMD to the SDHC registers.
 *   If there is a data need to send, call sdhci_prepare_data() to set transmfer mode,  
 *   data block size, data block count to the SDHC registers.
 */
static void sdhci_send_command
(
    UBOOT_SDHCI_HOST   *host, 
    struct mmc_command *cmd
)
{
    int flags=0;
    u32 mask;
    u32 present=0;
    int timeOut=100/*10*/;
    u32 command=0;



    //ks_debug(MMC_HC_CMD,("%s:Sending cmd=%d, data=%08x\n",__FUNCTION__, cmd->opcode, cmd->data));

    mask = SDHCI_CMD_INHIBIT;
    if ((cmd->data != NULL) || (cmd->flags & MMC_RSP_BUSY))
       mask |= SDHCI_DATA_INHIBIT;

    /* Wait max 10 ms */
    do {
        udelay(1000);
        if (timeOut-- <= 0 ) 
        {
            ks_debug(MMC_DBG_ERROR,("%s:cmd (%d), Controller never released inhibit bits. present=%08x (mask=%08x)\n",
                     __FUNCTION__, cmd->opcode, present, mask));
            cmd->error = MMC_ERR_FAILED;
            sdhci_tasklet_finish ((unsigned long)host);
            return;
        }
        present = ks_read(host->ioaddr + SDHCI_PRESENT_STATE);
    } while (present & mask);
    ks_debug(MMC_DBG_INIT,
                ("%s: %08X:%08x.\n",__FUNCTION__, (host->ioaddr + SDHCI_PRESENT_STATE), present));

    host->cmd = cmd;

    /* Set DMA address, Block Size, and Block Count reg. */
    sdhci_prepare_data(host, cmd->data);

    /* Set Argument reg. */
    ks_write(host->ioaddr + SDHCI_ARGUMENT, cmd->arg);

    /* Set Transfer mode reg. */
    sdhci_set_transfer_mode(host, cmd->data);

    ks_debug(MMC_DBG_INIT,("%s: response type=%x \n", __FUNCTION__, cmd->flags ));

    if ((cmd->flags & MMC_RSP_LONG) && (cmd->flags & MMC_RSP_BUSY)) 
    {
        ks_debug(MMC_DBG_ERROR,("%s: Unsupported response type! \n", __FUNCTION__ ));
        cmd->error = MMC_ERR_INVALID;
        sdhci_tasklet_finish ((unsigned long)host);
        return;
    }

    if (!(cmd->flags & MMC_RSP_MASK))
        flags = SDHCI_CMD_RESP_NONE;
    else if (cmd->flags & MMC_RSP_LONG)
        flags = SDHCI_CMD_RESP_LONG;
    else if (cmd->flags & MMC_RSP_BUSY)
        flags = SDHCI_CMD_RESP_SHORT_BUSY;
    else
        flags = SDHCI_CMD_RESP_SHORT;

    if (cmd->flags & MMC_RSP_CRC)
        flags |= SDHCI_CMD_CRC;
    if (cmd->flags & MMC_RSP_OPCODE)
        flags |= SDHCI_CMD_INDEX;
    if (cmd->data)
        flags |= SDHCI_CMD_DATA;

    ks_debug(MMC_DBG_INIT,("%s:flags=0x%x \n",__FUNCTION__, flags));
    command = SDHCI_MAKE_CMD(cmd->opcode, flags);

#if (0)
    ks_debug(MMC_HC_CMD,
             ("addr: %08x.\n", ks_read(host->ioaddr + SDHCI_DMA_ADDRESS )));
    ks_debug(MMC_HC_CMD,
            ("size: %04x.\n", ks_read_w(host->ioaddr + SDHCI_BLOCK_SIZE )));
    ks_debug(MMC_HC_CMD,
            ("cnt : %04x.\n", ks_read_w(host->ioaddr + SDHCI_BLOCK_COUNT )));
    ks_debug(MMC_HC_CMD,
            ("Arg : %08x\n", ks_read(host->ioaddr + SDHCI_ARGUMENT) ));
    ks_debug(MMC_HC_CMD,
            ("Tran: %04x.\n", ks_read_w(host->ioaddr + SDHCI_TRANSFER_MODE )));
    ks_debug(MMC_HC_CMD,
            ("Cmd : %04x\n", (int)command ));
    ks_debug(MMC_HC_CMD,
            ("Present: %08x \n", (int)ks_read(host->ioaddr + SDHCI_PRESENT_STATE) ));
#endif

#if (1)
    if (cmd->data)
    {
 	        u32 uReg;

            /* Start data DMA transfer, set GPIO 6 to high */
	        uReg = ks8692_read_dword(KS8692_GPIO_DATA);
	        uReg |= 0x00000040;    
	        ks8692_write_dword(KS8692_GPIO_DATA, uReg);
    }
#endif

    ks_write_w(host->ioaddr + SDHCI_COMMAND, SDHCI_MAKE_CMD(cmd->opcode, flags));

    ks_debug(MMC_DBG_INIT,("%s: exit.\n",__FUNCTION__));
}

/*
 * sdhci_finish_command - 
 *   Read CMD response from SDHC register. If previous CMD is Data transfer, 
 *   call sdhci_transfer_pio() to read Data from DATA line also.
 */
static void sdhci_finish_command
(
    UBOOT_SDHCI_HOST *host
)
{
    int i;


    ks_debug(MMC_DBG_INIT,("%s: \n",__FUNCTION__ ));

    if (host->cmd->flags & MMC_RSP_MASK) 
    {
        if (host->cmd->flags & MMC_RSP_LONG) 
        {
            /* CRC is stripped so we need to do some shifting. */
            for (i = 0;i < 4;i++) 
            {
                host->cmd->resp[i] = ks_read(host->ioaddr + (SDHCI_RESPONSE + (i*4)) );
            }
            ks_debug(MMC_DBG_INIT,("%s: Read the short response from CMD line \n(resp:%08x.%08x.%08x.%08x).\n", __FUNCTION__, 
                             host->cmd->resp[0],host->cmd->resp[1],host->cmd->resp[2],host->cmd->resp[3]));
        } 
        else 
        {
            /* CRC is stripped so we need to do some shifting. */
            host->cmd->resp[0] = ks_read(host->ioaddr + SDHCI_RESPONSE);
            ks_debug(MMC_DBG_INIT,("%s: Read the short response from CMD line (resp[0]:%08x).\n", __FUNCTION__, 
                             host->cmd->resp[0]));
        }
    }

    host->cmd->error = MMC_ERR_NONE;

    ks_debug(/*MMC_HC_CMD*/MMC_DBG_INIT,("%s: Ending cmd (%d)\n", __FUNCTION__, host->cmd->opcode));

    if (host->cmd->data) 
    {
        u32 intmask;

        host->data = host->cmd->data;

        ks_debug(MMC_DBG_INIT,("%s: Going to transfer data on DAT line.\n", __FUNCTION__));
        if (!(host->flags & SDHCI_USE_DMA)) 
        {
            /*
             * Don't enable the interrupts until now to make sure we
             * get stable handling of the FIFO.
             */
            intmask = ks_read(host->ioaddr + SDHCI_INT_ENABLE);
            intmask |= SDHCI_INT_BUF_EMPTY | SDHCI_INT_BUF_FULL;
            ks_write(host->ioaddr + SDHCI_INT_ENABLE, intmask);

#ifdef CONFIG_USE_IRQ
            intmask = ks_read(host->ioaddr + SDHCI_SIGNAL_ENABLE);
            intmask |= SDHCI_INT_BUF_EMPTY | SDHCI_INT_BUF_FULL;
            ks_write(host->ioaddr + SDHCI_SIGNAL_ENABLE, intmask);
#endif

            /*
             * The buffer interrupts are to unreliable so we
             * start the transfer immediatly.
             */
            sdhci_transfer_pio(host);
        }
    } 
    else
       sdhci_tasklet_finish((unsigned long)host);


#if (0)
    host->cmd = NULL;
#endif
}

/*
 * sdhci_request - 
 *   If card is present, call sdhci_send_command() to send CMD command.
 */
static void sdhci_request
(
    UBOOT_MMC_HOST *mmc, 
    struct mmc_request *mrq
)
{
    UBOOT_SDHCI_HOST *host=mmc->host;
    unsigned long presendState;


    ks_debug(MMC_DBG_INIT,("%s: cmd=0x%x, arg=0x%x, resp=0x%x \n",__FUNCTION__,
                     mrq->cmd->opcode, mrq->cmd->arg, mrq->cmd->flags));

    sdhci_activate_led(host);

    host->mrq = mrq;

    presendState = ks_read(host->ioaddr + SDHCI_PRESENT_STATE); 
    ks_debug(MMC_DBG_INIT,
                ("%s: %08X:%08x.\n",__FUNCTION__, (host->ioaddr + SDHCI_PRESENT_STATE), presendState));

    if (!(presendState & SDHCI_CARD_PRESENT)) 
    {
        ks_debug(MMC_DBG_ERROR,
                ("%s: card not presend (%08X:%08x).\n",__FUNCTION__, (host->ioaddr + SDHCI_PRESENT_STATE), presendState));
        host->mrq->cmd->error = MMC_ERR_TIMEOUT;
        sdhci_tasklet_finish ((unsigned long)host);
    } 
    else
    {
        sdhci_send_command(host, mrq->cmd);
    }
}

/*****************************************************************************\
 *                                                                           *
 * SDHCI Polling Interrupt Status                                            *
 *                                                                           *
\*****************************************************************************/

static ulong sdhci_irq
(
    UBOOT_SDHCI_HOST *host, 
    int               intmask 
)
{
    int timeOut=1000/*100*/;
    u32 intStatus;

    ks_debug(MMC_HC_INT,("%s: intmask %08x\n",__FUNCTION__, intmask ));

    intStatus = ks_read(host->ioaddr + SDHCI_INT_STATUS);
    /* wait max 1ms */
    while ( !(intStatus & intmask) ) {
        if (timeOut <= 0) {
            ks_debug(MMC_HC_INT/*MMC_HC_CMD*/,("%s: timeOut=%d, intStatus=%08x\n",__FUNCTION__, timeOut, intStatus ));
            break;
        }
        timeOut--;
        udelay(1);  /* wait 1us */
        intStatus = ks_read(host->ioaddr + SDHCI_INT_STATUS);
    }
    ks_debug(MMC_HC_INT/*MMC_HC_CMD*/,("%s: timeOut=%d, intStatus=%08x\n",__FUNCTION__, timeOut, intStatus ));

    intStatus = ks_read(host->ioaddr + SDHCI_INT_STATUS);
    if (!intStatus) {
            {
 	        u32 uReg;

            /* Timeout for data done interrupt, Reset GPIO 16 to low */
	        uReg = ks8692_read_dword(KS8692_GPIO_DATA);
	        uReg &= ~0x00010000;    
	        ks8692_write_dword(KS8692_GPIO_DATA, uReg);
            }
        ks_debug(MMC_DBG_ERROR,("%s: ERROR, timeOut(%d) with no interrupt (%08x).\n", __FUNCTION__,
                 timeOut, intStatus));
        if (host->cmd)
           host->cmd->error = MMC_ERR_TIMEOUT;
        if (host->data)
           host->data->error = MMC_ERR_TIMEOUT;

        sdhci_finish_data(host);

        goto out;
    }

    ks_debug(MMC_HC_INT,
            ("%s: intStatus 0x%08X:%08x\n", __FUNCTION__, 
              (host->ioaddr + SDHCI_INT_STATUS), intStatus));
    ks_debug(MMC_HC_INT,
            ("%s: intEnable 0x%08X:%08x\n", __FUNCTION__, 
             (host->ioaddr + SDHCI_INT_ENABLE), ks_read(host->ioaddr + SDHCI_INT_ENABLE) ));


    host->error = MMC_ERR_NONE;

    if (intStatus & intmask & SDHCI_INT_CMD_MASK) 
    {
        sdhci_cmd_irq(host, intStatus);
        ks_write(host->ioaddr + SDHCI_INT_STATUS, (intStatus & SDHCI_INT_CMD_MASK));
    }

    if (intStatus & intmask & SDHCI_INT_DATA_MASK) 
    {
        sdhci_data_irq(host, intStatus);
        ks_write(host->ioaddr + SDHCI_INT_STATUS, (intStatus & SDHCI_INT_DATA_MASK));
    }

    intStatus &= ~(SDHCI_INT_CMD_MASK | SDHCI_INT_DATA_MASK);

    if (intStatus & SDHCI_INT_CARD_INT) 
    {
        ks_debug(MMC_DBG_ERROR,("%s: ERROR, Unexpected card interrupt.\n", __FUNCTION__));
        sdhci_dumpregs(host);
        ks_write(host->ioaddr + SDHCI_INT_STATUS, (intStatus & SDHCI_INT_CARD_INT));
    }

    if (intStatus & SDHCI_INT_BUS_POWER) 
    {
        ks_debug(MMC_DBG_ERROR,("%s: ERROR, Unexpected bus power interrupt.\n", __FUNCTION__));
        sdhci_dumpregs(host);
        ks_write(host->ioaddr + SDHCI_INT_STATUS, (intStatus & SDHCI_INT_BUS_POWER));
    }

    if (intStatus & SDHCI_INT_ACMD12ERR) 
    {
        ks_debug(MMC_DBG_ERROR,("%s: ERROR, Unexpected auto CMD12 error.\n", __FUNCTION__));
        sdhci_dumpregs(host);
        ks_write_w(host->ioaddr + SDHCI_ACMD12_ERR, (intStatus & SDHCI_INT_ACMD12ERR));
    }

    if (intStatus) {
        ks_debug(MMC_DBG_ERROR,("%s: ERROR, Unexpected interrupts.\n", __FUNCTION__));
        sdhci_dumpregs(host);
        //ks_write(host->ioaddr + SDHCI_INT_STATUS, intStatus );
    }
   
out:
   if (host->cmd)
        host->error = host->cmd->error;
    if (host->data)
        host->error |= host->data->error;

    return ( host->error );
}


/*
 * sdhci_cmd_irq - 
 *   Checking interrupt status. If it is 'Command Complete',  
 *   call sdhci_finish_command() to read CMD  response from CMD line.
 */
static void sdhci_cmd_irq
(
    UBOOT_SDHCI_HOST *host, 
    u32 intStatus
)
{

    ks_debug(MMC_DBG_INIT,("%s: intStatus %08x\n",__FUNCTION__, intStatus ));

    if (!host->cmd) 
    {
        ks_debug(MMC_DBG_ERROR,("%s: Got command interrupt even though no command operation was in progress.\n", __FUNCTION__ ));
        /* sdhci_dumpregs(host); */
        return;
    }

    if (intStatus & SDHCI_INT_RESPONSE)
        sdhci_finish_command(host);
    else 
    {
        if (intStatus & SDHCI_INT_TIMEOUT)
            host->cmd->error = MMC_ERR_TIMEOUT;
        else if (intStatus & SDHCI_INT_CRC)
            host->cmd->error = MMC_ERR_BADCRC;
        else if (intStatus & (SDHCI_INT_END_BIT | SDHCI_INT_INDEX))
            host->cmd->error = MMC_ERR_FAILED;
        else
            host->cmd->error = MMC_ERR_INVALID;

        ks_debug(MMC_DBG_ERROR,
                ("%s: ERROR, cmd error code %d. intStatus %08x.\n", __FUNCTION__, host->cmd->error, intStatus ));

        sdhci_tasklet_finish ((unsigned long)host);
    }
}

static void sdhci_data_irq
(
    UBOOT_SDHCI_HOST *host, 
    u32 intStatus
)
{

    ks_debug(MMC_DBG_INIT,("%s: intStatus %08x\n",__FUNCTION__, intStatus ));

    if (!host->data) 
    {
        /*
         * A data end interrupt is sent together with the response
         * for the stop command.
         */
        if (intStatus & SDHCI_INT_DATA_END)
            return;

        ks_debug(MMC_DBG_ERROR,("%s: Got data interrupt even though no data operation was in progress.\n", __FUNCTION__));
        /* sdhci_dumpregs(host); */
        return;
    }

    if (intStatus & SDHCI_INT_DATA_TIMEOUT)
        host->data->error = MMC_ERR_TIMEOUT;
    else if (intStatus & SDHCI_INT_DATA_CRC)
        host->data->error = MMC_ERR_BADCRC;
    else if (intStatus & SDHCI_INT_DATA_END_BIT)
        host->data->error = MMC_ERR_FAILED;

    //printf("%s: host=%08x, host->data=%08x\n",__FUNCTION__, host, host->data);

    if (host->data->error != MMC_ERR_NONE)
    {
        ks_debug(MMC_DBG_ERROR,("%s: data error (0x%x). intStatus %08x \n", __FUNCTION__, host->data->error, intStatus ));
        sdhci_finish_data(host);
    }
    else 
    {
        if (intStatus & (SDHCI_INT_BUF_FULL | SDHCI_INT_BUF_EMPTY)) {
           if (!(host->flags & SDHCI_USE_DMA)) 
               sdhci_transfer_pio(host);
        }

        if (intStatus & SDHCI_INT_DATA_END)
            sdhci_finish_data(host);
    }
}


/*****************************************************************************\
 *                                                                           *
 *       MMC functions                                                       *
 *                                                                           *
\*****************************************************************************/


/****************************************************
mmc_block_read
*****************************************************/
int mmc_block_read
(
    uchar *dst, 
    ulong src, 
    ulong len,
    int   dmaSize
)
{
    UBOOT_SDHCI_HOST    *host;
    struct mmc_request  *sd_mrq;
    struct mmc_command	*sd_cmd;
    struct mmc_data     *sd_data;
    struct mmc_command	*sd_stop;
    uint dataBlocks;
    u32 *resp;
    int  fError=0;
    int  rc=0;


    if (len == 0) {
       return 0;
    }

    ks_debug(MMC_DISPLAY_READ,("%s: dst=%lx, src=%lx, len=%d\n", __FUNCTION__, (ulong)dst, src, len));

    /*
     * Set MMC parameters.
     */
    host = ghost;
    if ( !host )
    {
        ks_debug(MMC_DBG_ERROR, ("%s: host is null.\n", __FUNCTION__));
        return (-1);
    }

    sd_cmd = malloc( sizeof(struct mmc_command) );
    if ( !sd_cmd) 
       goto MMC_BLOCK_READ_EXIT;

    sd_mrq = malloc( sizeof(struct mmc_request) );
    if ( !sd_mrq) 
       goto MMC_BLOCK_READ_EXIT;

    sd_data = malloc( sizeof(struct mmc_data) );
    if ( !sd_data) 
       goto MMC_BLOCK_READ_EXIT;

    host->dmaSize = dmaSize;
    /* 
     * Issue CMD16: Addressed commands with response (ac) -  
     * sent on CMD, response (R1) on CMD.
     * Sets a block length (in byte) for all following block commands (read\write). 
     */
    memset(sd_cmd, 0, sizeof(struct mmc_command) );
    memset(sd_mrq, 0, sizeof(struct mmc_request) );
    memset(sd_data, 0, sizeof(struct mmc_data) );

    sd_cmd->opcode = MMC_CMD_SET_BLOCKLEN;        /* CMD16 */
    sd_cmd->arg    = MMC_BLOCK_SIZE;              /* Argument (block length=512) */
    sd_cmd->flags  = (MMC_RSP_R1|MMC_RSP_OPCODE); /* req Short Resp */
    sd_cmd->data   = NULL;  

    sd_mrq->stop = NULL;
    sd_mrq->data = NULL;
    sd_mrq->cmd  = sd_cmd;

    resp = &sd_cmd->resp[0];
    ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
    sdhci_request (host->mmc, sd_mrq);

    if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
    {
        ks_debug(MMC_DBG_ERROR,
                ("%s: Issue CMD%d not completed. host cmd error code %d\n",
                  __FUNCTION__, sd_cmd->opcode, host->cmd->error));
        return (-1);
    }   

    /* Check R1 Response data */
    if (resp[0] & R1_ANYERROR_MASK)
    {
        ks_debug(MMC_DBG_ERROR,
                 ("%s: ERROR on R1 Response 0x%08x\n", __FUNCTION__, (resp[0] & R1_ANYERROR_MASK) ));
        return (-1);
    }
    else
        ks_debug(MMC_DISPLAY_SD_STATE,
                ( "*** In %s State ***\n", sd_statet[(resp[0] & CURRENT_STATE_MASK) >> 9]) );

    ks_debug(MMC_DBG_INIT,
             ( "------------------------------------------------------------------\n"));

    {
    u32 uReg;

    /* Set GPIO 16 to high */
    uReg = ks8692_read_dword(KS8692_GPIO_DATA);
    uReg |= 0x00010000;    
    ks8692_write_dword(KS8692_GPIO_DATA, uReg);
    }

    dataBlocks = len >> 9;

    /* 
     * Issue CMD17 or CMD18: Addressed data transfer commands (adtc) -  
     * sent on CMD, response (R1) on CMD, data transfer on DAT.
     * Let MMC into Sending-data state (Read a single block data). 
     */
    memset(sd_cmd, 0, sizeof(struct mmc_command) );
    memset(sd_mrq, 0, sizeof(struct mmc_request) );
    memset(sd_data, 0, sizeof(struct mmc_data) );

    sd_cmd->opcode = dataBlocks > 1 ? MMC_CMD_RD_BLK_MULTI : MMC_CMD_READ_BLOCK;   /* CMD17 or 18 */
    sd_cmd->arg    = src;                         /* Argument (data address) */
    sd_cmd->flags  = (MMC_RSP_R1|MMC_RSP_OPCODE); /* req Short Resp */
    sd_cmd->data   = sd_data;  

    sd_data->blocks       = dataBlocks;
    sd_data->blksz_bits   = 8+1; /* I don't know why is 9 to get (1<<9) = block size(len) */;
    sd_data->timeout_ns   = mmc_csd.nsac * 10;
    sd_data->timeout_clks = mmc_csd.taac * 10;
    sd_data->flags        = MMC_DATA_READ;
    sd_data->buffer       = dst;
    sd_data->stop         = NULL;

    sd_mrq->stop = NULL;
    sd_mrq->data = sd_data;
    sd_mrq->cmd  = sd_cmd;

    resp = &sd_cmd->resp[0];
    ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
    //printf("%s: host->mmc=%08x, sd_mrq=%08x, sd_data=%08x\n",__FUNCTION__, host->mmc, sd_mrq, sd_data);
    sdhci_request (host->mmc, sd_mrq);

    if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
    {
        ks_debug(MMC_DBG_ERROR,
                ("%s: Issue CMD%d not completed. host cmd error code %d\n",
                  __FUNCTION__, sd_cmd->opcode, host->error));

        fError = 1;
        rc = -1;
    }   
    ks_debug(MMC_DBG_INIT/*MMC_HC_CMD*/,("%s: intStatus=%08x\n", __FUNCTION__, ks_read(host->ioaddr + SDHCI_INT_STATUS) ));
    ks_debug(MMC_DBG_INIT/*MMC_HC_CMD*/,("%s: intEnable=%08x\n", __FUNCTION__, ks_read(host->ioaddr + SDHCI_INT_ENABLE) ));

    /* Check R1 Response data */
    if (resp[0] & R1_ANYERROR_MASK)
    {
        ks_debug(MMC_DBG_ERROR,
                 ("%s: ERROR on R1 Response 0x%08x\n", __FUNCTION__, (resp[0] & R1_ANYERROR_MASK) ));
        return (-1);
    }
    else
        ks_debug(MMC_DISPLAY_SD_STATE,
                ( "*** In %s State ***\n", sd_statet[(resp[0] & CURRENT_STATE_MASK) >> 9]) );

#if (1)
    if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_DATA_MASK) != MMC_ERR_NONE)
    {

        ks_debug(MMC_DBG_ERROR,
                ("%s: Issue CMD%d, data transfer not completed. host cmd error code %d\n",
                  __FUNCTION__, sd_cmd->opcode, host->error));

        fError = 1;
        rc = -1;
    }   
    else   
    {
 	        u32 uReg;

            /* Got data done interrupt, Reset GPIO 6 to low */
	        uReg = ks8692_read_dword(KS8692_GPIO_DATA);
	        uReg &= ~0x00000040;    
	        ks8692_write_dword(KS8692_GPIO_DATA, uReg);
    }

#endif
    

#if (0)
    ks_debug(MMC_HC_CMD,("%s: intStatus=%08x\n", __FUNCTION__, ks_read(host->ioaddr + SDHCI_INT_STATUS) ));
    ks_debug(MMC_HC_CMD,("%s: intEnable=%08x\n", __FUNCTION__, ks_read(host->ioaddr + SDHCI_INT_ENABLE) ));
    ks_debug(MMC_HC_CMD,
                ("cnt : %04x.\n", ks_read_w(host->ioaddr + SDHCI_BLOCK_COUNT )));
    ks_debug(MMC_HC_CMD,
               ("Present: %08x \n", (int)ks_read(host->ioaddr + SDHCI_PRESENT_STATE) ));
#endif

    ks_debug(MMC_DBG_INIT,
             ( "------------------------------------------------------------------\n"));

#if (1)
    if (dataBlocks > 1)
    {
        /* 
         * Issue CMD12: Addressed commands with response (ac) -  
         * sent on CMD, response (R1b) on CMD.
         * Force card to stop transmission. 
         */
         memset(sd_cmd, 0, sizeof(struct mmc_command) );
         memset(sd_mrq, 0, sizeof(struct mmc_request) );
         memset(sd_data, 0, sizeof(struct mmc_data) );

         sd_cmd->opcode = MMC_STOP_TRANSMISSION;       /* CMD12 */
         sd_cmd->arg    = 0;                           /* Argument RCA */
         sd_cmd->flags  = (MMC_RSP_R1|MMC_RSP_OPCODE); /* req Short Resp */
         sd_cmd->data   = NULL;  

         sd_mrq->stop = NULL;
         sd_mrq->data = NULL;
         sd_mrq->cmd  = sd_cmd;

         resp = &sd_cmd->resp[0];
         ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
         sdhci_request (host->mmc, sd_mrq);

         if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
         {
             ks_debug(MMC_DBG_ERROR,
                ("%s: Issue CMD%d not completed. host cmd error code %d\n",
                  __FUNCTION__, sd_cmd->opcode, host->cmd->error));
             return (-1);
         }   

         ks_debug(MMC_DBG_INIT,
                 ( "------------------------------------------------------------------\n"));
    }
#endif
    if (fError) {
        sdhci_dumpregs(host);
        sdhci_reset(host, SDHCI_RESET_CMD);
        sdhci_reset(host, SDHCI_RESET_DATA);
    }

MMC_BLOCK_READ_EXIT:
    if ( sd_cmd) 
       free(sd_cmd);
    if ( sd_mrq) 
       free(sd_mrq);
    if ( sd_data) 
       free(sd_data);

    return (rc);
}


/****************************************************
 mmc_block_write
 ****************************************************/
int mmc_block_write
(
    ulong  dst, 
    uchar *src, 
    int    len,
    int    dmaSize
)
{
    UBOOT_SDHCI_HOST    *host;
    struct mmc_request  *sd_mrq;
    struct mmc_command	*sd_cmd;
    struct mmc_data     *sd_data;
    uint dataBlocks;
    u32 *resp;
    int  fError=0;
    int  rc=0;



    if (len == 0) 
    {
        return 0;
    }

    ks_debug(MMC_DISPLAY_WRITE,("%s: dst=%lx, src=%lx, len=%d\n", 
             __FUNCTION__, dst, (ulong)src, len));

    /*
     * Set MMC parameters.
     */
    host = ghost;
    if ( !host )
    {
        ks_debug(MMC_DBG_ERROR, ("%s: host is null.\n", __FUNCTION__));
        return (-1);
    }

    sd_cmd = malloc( sizeof(struct mmc_command) );
    if ( !sd_cmd) 
       goto MMC_BLOCK_WRITE_EXIT;

    sd_mrq = malloc( sizeof(struct mmc_request) );
    if ( !sd_mrq) 
       goto MMC_BLOCK_WRITE_EXIT;

    sd_data = malloc( sizeof(struct mmc_data) );
    if ( !sd_data) 
       goto MMC_BLOCK_WRITE_EXIT;

    host->dmaSize = dmaSize;

    /* 
     * Issue CMD16: Addressed commands with response (ac) -  
     * sent on CMD, response (R1) on CMD.
     * Selects a block length (in byte) for all following block commands (read\write). 
     */
    sd_cmd->opcode = MMC_CMD_SET_BLOCKLEN;        /* CMD16 */
    sd_cmd->arg    = MMC_BLOCK_SIZE;              /* Argument (block length) */
    sd_cmd->flags  = (MMC_RSP_R1|MMC_RSP_OPCODE); /* req Short Resp */
    sd_cmd->data   = NULL;  

    sd_mrq->stop = NULL;
    sd_mrq->data = NULL;
    sd_mrq->cmd  = sd_cmd;

    resp = &sd_cmd->resp[0];
    ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
    sdhci_request (host->mmc, sd_mrq);

    if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
    {
        ks_debug(MMC_DBG_ERROR,
                ("%s: Issue CMD%d not completed. host cmd error code %d\n",
                  __FUNCTION__, sd_cmd->opcode, host->cmd->error));
        return (-1);
    }   

    /* Check R1 Response data */
    if (resp[0] & R1_ANYERROR_MASK)
    {
        ks_debug(MMC_DBG_ERROR,
                 ("%s: ERROR on R1 Response 0x%08x\n", __FUNCTION__, (resp[0] & R1_ANYERROR_MASK) ));
        return (-1);
    }
    else
        ks_debug(MMC_DISPLAY_SD_STATE,
                ( "*** In %s State ***\n", sd_statet[(resp[0] & CURRENT_STATE_MASK) >> 9]) );

    ks_debug(MMC_DBG_INIT,
             ( "------------------------------------------------------------------\n"));

    dataBlocks = len >> 9;

    /* 
     * Issue CMD24 or CMD25: Addressed data transfer commands (adtc) -  
     * sent on CMD, response on CMD, data transfer on DAT.
     * Write a block of size selected by CMD16. 
     */
    sd_cmd->opcode = dataBlocks > 1 ? MMC_WRITE_MULTIPLE_BLOCK : MMC_CMD_WRITE_BLOCK; /* CMD24 or 25 */
    sd_cmd->arg    = dst;                         /* Argument (data address) */
    sd_cmd->flags  = (MMC_RSP_R1|MMC_RSP_OPCODE); /* req Short Resp */
    sd_cmd->data   = sd_data;  

    sd_data->blocks       = dataBlocks;
    sd_data->blksz_bits   = 8+1; /* I don't know why is 9 to get (1<<9) = block size(len) */;
    sd_data->timeout_ns   = mmc_csd.nsac * 10;
    sd_data->timeout_clks = mmc_csd.taac * 10;
    sd_data->flags        = MMC_DATA_WRITE;
    sd_data->buffer       = src;
    sd_data->stop         = NULL;

    sd_mrq->stop = NULL;
    sd_mrq->data = sd_data;
    sd_mrq->cmd  = sd_cmd;

    resp = &sd_cmd->resp[0];
    ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
    sdhci_request (host->mmc, sd_mrq);

    if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
    {
        ks_debug(MMC_DBG_ERROR,
                ("%s: Issue CMD%d not completed. host cmd error code %d\n",
                  __FUNCTION__, sd_cmd->opcode, host->cmd->error));
        fError = 1;
        rc = -1;
    }   
    ks_debug(MMC_HC_CMD,("%s: intStatus=%08x\n", __FUNCTION__, ks_read(host->ioaddr + SDHCI_INT_STATUS) ));
    ks_debug(MMC_HC_CMD,("%s: intEnable=%08x\n", __FUNCTION__, ks_read(host->ioaddr + SDHCI_INT_ENABLE) ));

    /* Check R1 Response data */
    if (resp[0] & R1_ANYERROR_MASK)
    {
        ks_debug(MMC_DBG_ERROR,
                 ("%s: ERROR on R1 Response 0x%08x\n", __FUNCTION__, (resp[0] & R1_ANYERROR_MASK) ));
        return (-1);
    }
    else
        ks_debug(MMC_DISPLAY_SD_STATE,
                ( "*** In %s State ***\n", sd_statet[(resp[0] & CURRENT_STATE_MASK) >> 9]) );

#if (1)
    if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_DATA_MASK) != MMC_ERR_NONE)
    {
        ks_debug(MMC_DBG_ERROR,("%s: Issue CMD%d, data transfer not completed.host cmd error code %d\n",
                 __FUNCTION__, sd_cmd->opcode, host->error));
        fError = 1;
        rc = -1;
    }   
#endif

#if (1)
    ks_debug(MMC_HC_CMD,("%s: intStatus=%08x\n", __FUNCTION__, ks_read(host->ioaddr + SDHCI_INT_STATUS) ));
    ks_debug(MMC_HC_CMD,("%s: intEnable=%08x\n", __FUNCTION__, ks_read(host->ioaddr + SDHCI_INT_ENABLE) ));
    ks_debug(MMC_HC_CMD,
                ("cnt : %04x.\n", ks_read_w(host->ioaddr + SDHCI_BLOCK_COUNT )));
    ks_debug(MMC_HC_CMD,
               ("Present: %08x \n", (int)ks_read(host->ioaddr + SDHCI_PRESENT_STATE) ));
#endif

    ks_debug(MMC_DBG_INIT,
             ( "------------------------------------------------------------------\n"));


#if (1)
    if (dataBlocks > 1)
    {
        /* 
         * Issue CMD12: Addressed commands with response (ac) -  
         * sent on CMD, response (R1b) on CMD.
         * Force card to stop transmission. 
         */
         memset(sd_cmd, 0, sizeof(struct mmc_command) );
         memset(sd_mrq, 0, sizeof(struct mmc_request) );
         memset(sd_data, 0, sizeof(struct mmc_data) );

         sd_cmd->opcode = MMC_STOP_TRANSMISSION;       /* CMD12 */
         sd_cmd->arg    = 0;                           /* Argument RCA */
         sd_cmd->flags  = (MMC_RSP_R1|MMC_RSP_OPCODE); /* req Short Resp */
         sd_cmd->data   = NULL;  

         sd_mrq->stop = NULL;
         sd_mrq->data = NULL;
         sd_mrq->cmd  = sd_cmd;

         resp = &sd_cmd->resp[0];
         ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
         sdhci_request (host->mmc, sd_mrq);

         if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
         {
             ks_debug(MMC_DBG_ERROR,
                ("%s: Issue CMD%d not completed. host cmd error code %d\n",
                  __FUNCTION__, sd_cmd->opcode, host->cmd->error));
             return (-1);
         }   

         ks_debug(MMC_DBG_INIT,
                 ( "------------------------------------------------------------------\n"));
    }
#endif
    if (fError) {
        sdhci_dumpregs(host);
        sdhci_reset(host, SDHCI_RESET_CMD);
        sdhci_reset(host, SDHCI_RESET_DATA);
    }

MMC_BLOCK_WRITE_EXIT:
    if ( sd_cmd) 
       free(sd_cmd);

    if ( sd_mrq) 
       free(sd_mrq);

    if ( sd_data) 
       free(sd_data);

    return 0;
}


/****************************************************
 mmc_read
 ****************************************************/
int mmc_read
(
    ulong  src,
    uchar *dst,
    int    size,
    int    dmaSize
)
{
    ulong end, part_start, part_end, part_len, aligned_start, aligned_end;
    ulong mmc_block_size, mmc_block_address, tx_size;


    if (size == 0) 
    {
       return 0;
    }

    if (!mmc_ready) 
    {
        printf("Please initial the MMC first\n");
        return -1;
    }

    mmc_block_size = MMC_BLOCK_SIZE;
    mmc_block_address = ~(mmc_block_size - 1);

    ks_debug(MMC_DBG_READ,("%s: src=%lx, dst=%lx, size=%lx, mmc_block_size=%lx, mmc_block_address=%lx\n",
             __FUNCTION__, src, (ulong)dst, size, mmc_block_size, mmc_block_address));

    src -= CFG_MMC_BASE;
    end = src + size;
    part_start = ~mmc_block_address & src;
    part_end = ~mmc_block_address & end;
    aligned_start = mmc_block_address & src;
    aligned_end = mmc_block_address & end;

    /* all block aligned accesses */
    ks_debug(MMC_DBG_READ,("%s 1: src=%lx, dst=%lx, end=%lx, pstart=%lx, pend=%lx, astart=%lx, aend=%lx\n",
             __FUNCTION__, src, (ulong)dst, end, part_start, part_end, aligned_start, aligned_end));
    if (part_start) 
    {
        part_len = mmc_block_size - part_start;
        ks_debug(MMC_DBG_READ,("%s 2: ps src=%lx, dst=%lx, end=%lx, pstart=%lx, pend=%lx, astart=%lx, aend=%lx\n",
		         __FUNCTION__, src, (ulong)dst, end, part_start, part_end, aligned_start, aligned_end));
        if ((mmc_block_read(mmc_buf, aligned_start, mmc_block_size, dmaSize)) < 0) 
        {
            return -1;
        }
        memcpy(dst, mmc_buf+part_start, part_len);
        dst += part_len;
        src += part_len;
    }
    ks_debug(MMC_DBG_READ,("%s 3: src=%lx, dst=%lx, end=%lx, pstart=%lx, pend=%lx, astart=%lx, aend=%lx\n",
	          __FUNCTION__, src, (ulong)dst, end, part_start, part_end, aligned_start, aligned_end));

#ifdef TEST_MULTI_BLOCK
    tx_size = (aligned_end- src);
    ks_debug(MMC_DBG_READ,("%s 4: al src=%lx, dst=%lx, end=%lx, pstart=%lx, pend=%lx, astart=%lx, aend=%lx, tx_size=%x\n",
             __FUNCTION__, src, (ulong)dst, end, part_start, part_end, aligned_start, aligned_end, tx_size));
    if ((mmc_block_read((uchar *)(dst), src, tx_size, dmaSize )) < 0) 
    {
        return -1;
    }
    src += tx_size;
    dst += tx_size;
#else
    for (; src < aligned_end; src += mmc_block_size, dst += mmc_block_size) 
    {
        ks_debug(MMC_DBG_READ,("%s 4: al src=%lx, dst=%lx, end=%lx, pstart=%lx, pend=%lx, astart=%lx, aend=%lx\n",
		         __FUNCTION__, src, (ulong)dst, end, part_start, part_end, aligned_start, aligned_end));
        if ((mmc_block_read((uchar *)(dst), src, mmc_block_size, dmaSize)) < 0) 
        {
            return -1;
        }
    }
#endif /* #ifdef TEST_MULTI_BLOCK */

    ks_debug(MMC_DBG_READ,("%s 5: src=%lx, dst=%lx, end=%lx, pstart=%lx, pend=%lx, astart=%lx, aend=%lx\n",
	          __FUNCTION__, src, (ulong)dst, end, part_start, part_end, aligned_start, aligned_end));
    if (part_end && src < end) 
    {
        ks_debug(MMC_DBG_READ,("%s 6: pe src=%lx, dst=%lx, end=%lx, pstart=%lx, pend=%lx, astart=%lx, aend=%lx\n",
		         __FUNCTION__, src, (ulong)dst, end, part_start, part_end, aligned_start, aligned_end));
        if ((mmc_block_read(mmc_buf, aligned_end, mmc_block_size, dmaSize)) < 0) 
        {
            return -1;
        }
        memcpy(dst, mmc_buf, part_end);
    }
    return 0;
}

/****************************************************
mmc_write
*****************************************************/
int mmc_write
(
    uchar *src, 
    ulong dst, 
    int size,
    int dmaSize
)
{
    ulong end, part_start, part_end, part_len, aligned_start, aligned_end;
    ulong mmc_block_size, mmc_block_address, rx_size;


    if (size == 0) {
        return 0;
    }

    if (!mmc_ready) {
        printf("Please initial the MMC first\n");
        return -1;
    }

    mmc_block_size = MMC_BLOCK_SIZE;
    mmc_block_address = ~(mmc_block_size - 1);

    ks_debug(MMC_DBG_WRITE,("\n%s 1: src=%lx, dst=%lx, size=%lx, mmc_block_size=%lx, mmc_block_address=%lx\n",
                    __FUNCTION__, (ulong)src, dst, size, mmc_block_size, mmc_block_address));

    dst -= CFG_MMC_BASE;
    end = dst + size;
    part_start = ~mmc_block_address & dst;
    part_end = ~mmc_block_address & end;
    aligned_start = mmc_block_address & dst;
    aligned_end = mmc_block_address & end;

    /* all block aligned accesses */
    ks_debug(MMC_DBG_WRITE,("%s 1: src=%lx, dst=%lx, end=%lx, pstart=%lx, pend=%lx, astart=%lx, aend=%lx\n",
	          __FUNCTION__, src, (ulong)dst, end, part_start, part_end, aligned_start, aligned_end));
    if (part_start) 
    {
        part_len = mmc_block_size - part_start;
        ks_debug(MMC_DBG_WRITE,("%s 2: ps src=%lx, dst=%lx, end=%lx, pstart=%lx, pend=%lx, astart=%lx, aend=%lx\n",
		         __FUNCTION__, (ulong)src, dst, end, part_start, part_end, aligned_start, aligned_end));

        if ((mmc_block_read(mmc_buf, aligned_start, mmc_block_size, dmaSize)) < 0) 
        {
            ks_debug(MMC_DBG_ERROR,("%s: mmc_block_read() fail.\n", __FUNCTION__));
            return -1;
        }

        memcpy(mmc_buf+part_start, src, part_len);
        if ((mmc_block_write(aligned_start, mmc_buf, mmc_block_size, dmaSize)) < 0) 
        {
            ks_debug(MMC_DBG_ERROR,("%s: mmc_block_write() fail.\n", __FUNCTION__));
            return -1;
        }
        dst += part_len;
        src += part_len;
    }

    ks_debug(MMC_DBG_WRITE,("%s 3: src=%lx, dst=%lx, end=%lx, pstart=%lx, pend=%lx, astart=%lx, aend=%lx\n",
             __FUNCTION__, src, (ulong)dst, end, part_start, part_end, aligned_start, aligned_end));

#ifdef TEST_MULTI_BLOCK
    rx_size = (aligned_end- dst);
    ks_debug(MMC_DBG_WRITE,("%s 4: al src=%lx, dst=%lx, end=%lx, pstart=%lx, pend=%lx, astart=%lx, aend=%lx, rx_size=%x\n",
             __FUNCTION__, src, (ulong)dst, end, part_start, part_end, aligned_start, aligned_end, rx_size));

    if ((mmc_block_write(dst, (uchar *)src, rx_size, dmaSize)) < 0) 
    {
        ks_debug(MMC_DBG_ERROR,("%s: mmc_block_write() fail.\n", __FUNCTION__));
        return -1;
    }
    src += rx_size;
    dst += rx_size;

#else
    for (; dst < aligned_end; src += mmc_block_size, dst += mmc_block_size) 
    {
        ks_debug(MMC_DBG_WRITE,("%s 4: al src=%lx, dst=%lx, end=%lx, pstart=%lx, pend=%lx, astart=%lx, aend=%lx\n",
                 __FUNCTION__, src, (ulong)dst, end, part_start, part_end, aligned_start, aligned_end));

        if ((mmc_block_write(dst, (uchar *)src, mmc_block_size, dmaSize)) < 0) 
        {
            ks_debug(MMC_DBG_ERROR,("%s: mmc_block_write() fail.\n", __FUNCTION__));
            return -1;
        }
    }
#endif /* #ifdef TEST_MULTI_BLOCK */

    ks_debug(MMC_DBG_WRITE,("%s 5: src=%lx, dst=%lx, end=%lx, pstart=%lx, pend=%lx, astart=%lx, aend=%lx\n",
             __FUNCTION__, src, (ulong)dst, end, part_start, part_end, aligned_start, aligned_end));

    if (part_end && dst < end) 
    {
        ks_debug(MMC_DBG_WRITE,("%s 6: pe src=%lx, dst=%lx, end=%lx, pstart=%lx, pend=%lx, astart=%lx, aend=%lx\n",
                 __FUNCTION__, src, (ulong)dst, end, part_start, part_end, aligned_start, aligned_end));

        if ((mmc_block_read(mmc_buf, aligned_end, mmc_block_size, dmaSize)) < 0) 
        {
            ks_debug(MMC_DBG_ERROR,("%s: mmc_block_read() fail.\n", __FUNCTION__));
            return -1;
        }

        memcpy(mmc_buf, src, part_end);
        if ((mmc_block_write(aligned_end, mmc_buf, mmc_block_size, dmaSize)) < 0) 
        {
            ks_debug(MMC_DBG_ERROR,("%s: mmc_block_write() fail.\n", __FUNCTION__));
            return -1;
        }
    }
    return 0;
}

/****************************************************
 mmc_bread
 ****************************************************/
ulong mmc_bread
(
    int dev_num, 
    ulong blknr, 
    ulong blkcnt, 
    ulong *dst
)
{

    int mmc_block_size = MMC_BLOCK_SIZE;
    ulong src = blknr * mmc_block_size + CFG_MMC_BASE;

    ks_debug(MMC_DBG_INIT,("%s: dev_num=%d, blknr=%d, dst=0x%lx\n",
                    __FUNCTION__, dev_num, blknr, (ulong)dst));

    mmc_read(src, (uchar *)dst, blkcnt*mmc_block_size, 0);

    return blkcnt;
}

/****************************************************
  mmc_init
 ****************************************************/
int mmc_init
(
    int verbose
)
{
    UBOOT_SDHCI_HOST    *host;
    UBOOT_MMC_HOST      *mmc;
    struct mmc_request  *sd_mrq;
    struct mmc_command  *sd_cmd;
    struct mmc_data     *sd_data=NULL;
    uchar *statusBuf=NULL;
    sd_csd_t *csd;
    int retries, rc = -ENODEV;
    u32 *resp;

    ks_debug(MMC_DBG_INIT,("%s: \n",__FUNCTION__ ));

    {
       u32 dwReg;

       dwReg = ks8692_read_dword(KS8692_SYSTEM_BUS_CLOCK);
       dwReg &= ~0x00000003;
       dwReg |=  0x00000000;
       //ks8692_write_dword(KS8692_SYSTEM_BUS_CLOCK, dwReg);
    }
    printf("%s: %08X:%08x.\n", __FUNCTION__, KS8692_SYSTEM_BUS_CLOCK, ks8692_read_dword(KS8692_SYSTEM_BUS_CLOCK) );


    host = sdhci_init ();
    if ( !host )
        return (-ENODEV);

    /*
     * Set MMC parameters.
     */
    mmc = malloc( sizeof(UBOOT_MMC_HOST) );
    if ( !mmc) 
       return (-ENODEV);

    memset(mmc, 0, sizeof(UBOOT_MMC_HOST) );
    mmc->host = host;
    host->mmc = mmc;

    ks_debug(MMC_DBG_INIT,("%s: host=%x mmc->host=%x  mmc=%x host->mmc=%x\n",__FUNCTION__,
                     host, mmc->host, mmc, host->mmc));


    sd_cmd = malloc( sizeof(struct mmc_command) );
    if ( !sd_cmd) 
       goto MMC_INIT_EXIT;

    sd_mrq = malloc( sizeof(struct mmc_request) );
    if ( !sd_mrq) 
       goto MMC_INIT_EXIT;

    /* Set SD Host Controller is interface with SD Memory card */
	host->mode = MMC_MODE_SD;

    ks_debug(MMC_DBG_INIT,
             ( "------------------------------------------------------------------\n"));

    /* 
     * Issue CMD0: Broadcast commands (bc) - 
     * sent on CMD, no response.
     * Let MMC into IDLE state 
     */
    sd_cmd->opcode = MMC_CMD_RESET; /* CMD0 */
    sd_cmd->arg    = 0;             /* No Argument */
    sd_cmd->flags  = MMC_RSP_NONE;  /* No Resp */
    sd_cmd->data   = NULL;  

    sd_mrq->stop = NULL;
    sd_mrq->data = NULL;
    sd_mrq->cmd  = sd_cmd;

    ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
    sdhci_request (mmc, sd_mrq);

    if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
    {
        ks_debug(MMC_DBG_ERROR,
                ("%s: Issue CMD%d not completed. host cmd error code %d\n",__FUNCTION__, sd_cmd->opcode, host->cmd->error));
        return (-ENODEV);
    }   
    ks_debug(MMC_DISPLAY_SD_STATE, ( "*** In IDLE State ***\n"));

    retries = 3;

    do {
       udelay(50);
       ks_debug(MMC_DBG_INIT,
             ( "------------------------------------------------------------------\n"));

       if (host->mode == MMC_MODE_SD )
       {
       /* 
        * Issue CMD55: Addressed commands with response (ac) - 
        * sent on CMD, response (R1) on CMD.
        * Let SD card knows that next command is an application specific command 
        * rather than a standard one.
        */
           sd_cmd->opcode = 55;                          /* CMD55 */
           sd_cmd->arg    = 0;                           /* Argument RCA=0x0000 */
           sd_cmd->flags  = (MMC_RSP_R1|MMC_RSP_OPCODE); /* req Short Resp */
           sd_cmd->data   = NULL;  

           sd_mrq->stop = NULL;
           sd_mrq->data = NULL;
           sd_mrq->cmd  = sd_cmd;

           resp = &sd_cmd->resp[0];

           ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
           sdhci_request (mmc, sd_mrq);

           if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
           {
               ks_debug(MMC_DBG_ERROR,
                       ("%s: Issue CMD%d not completed. host cmd error code %d\n",
                        __FUNCTION__, sd_cmd->opcode, host->cmd->error));
               return (-ENODEV);
           }  
          
           /* Check R1 Response data */
           if (resp[0] & R1_ANYERROR_MASK)
           {
               ks_debug(MMC_DBG_ERROR,
                       ("%s: ERROR on R1 Response 0x%08x\n", __FUNCTION__, (resp[0] & R1_ANYERROR_MASK) ));
               return (-ENODEV);
           }
           /* Check if ACMD is expected by SD device */
           else if ( !(resp[0] & APP_CMD))   
           {
               ks_debug(MMC_DBG_ERROR,
                       ("%s: ERROR on R1 Response (0x%08x). SD device is not expected AMCD. \n", 
                        __FUNCTION__, (resp[0] & R1_ANYERROR_MASK) ));
               return (-ENODEV);
           }
           else
               ks_debug(MMC_DISPLAY_SD_STATE,
                        ( "*** In %s State ***\n", sd_statet[(resp[0] & CURRENT_STATE_MASK) >> 9]) );
       }

       /* 
        * Issue AMD41 or (CMD1): Broadcast commands with response (bcr) - 
        * sent on CMD, response (R3) on CMD.
        * Let MMC into READY state (response OCR register)
        */

       ks_debug(MMC_DBG_INIT,
             ( "------------------------------------------------------------------\n"));
       if (host->mode == MMC_MODE_SD )
          sd_cmd->opcode = 41;                   /* ACMD41 */
       else
          sd_cmd->opcode = MMC_CMD_SEND_OP_COND; /* CMD1 */
       sd_cmd->arg    = 0x00300000;           /* 3.2v-3.3v, 3.3v-3.4v */
       sd_cmd->flags  = MMC_RSP_R3;           /* req Short Resp */
       sd_cmd->data   = NULL;  

       sd_mrq->stop = NULL;
       sd_mrq->data = NULL;
       sd_mrq->cmd  = sd_cmd;

       resp = &sd_cmd->resp[0];

       ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
       sdhci_request (mmc, sd_mrq);

       if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
       {
           ks_debug(MMC_DBG_ERROR,
                ("%s: Issue CMD%d not completed. host cmd error code %d\n",__FUNCTION__, sd_cmd->opcode, host->cmd->error));
           return (-ENODEV);
       }   
       ks_debug(MMC_DBG_INIT,("%s: resp[0]=0x%08x.\n",__FUNCTION__, resp[0]));

    } while (--retries && !(resp[0] & 0x80000000));

    if (!(resp[0] & 0x80000000))
    {
        ks_debug(MMC_DBG_ERROR,("%s: ERROR, resp[0]=0x%08x.\n", __FUNCTION__, resp[0] ));
        return (-ENODEV);
    }
    ks_debug(MMC_DISPLAY_SD_STATE, ( "*** In READY State ***\n"));

    /* 
     * Issue CMD2: Broadcast commands with response (bcr) - 
     * sent on CMD, response (R2) on CMD.
     * Let MMC into IDENT state (response Card Id - CID number)
     */

    ks_debug(MMC_DBG_INIT,
             ( "------------------------------------------------------------------\n"));
    sd_cmd->opcode = MMC_CMD_ALL_SEND_CID; /* CMD2 */
    sd_cmd->arg    = 0;                    /* No Argument */
    sd_cmd->flags  = MMC_RSP_R2;           /* req Long Resp */
    sd_cmd->data   = NULL;  

    sd_mrq->stop = NULL;
    sd_mrq->data = NULL;
    sd_mrq->cmd  = sd_cmd;

    resp = &sd_cmd->resp[0];

    ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
    sdhci_request (mmc, sd_mrq);

    if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
    {
        ks_debug(MMC_DBG_ERROR,
                ("%s: Issue CMD%d not completed. host cmd error code %d\n",__FUNCTION__, sd_cmd->opcode, host->cmd->error));
        return (-ENODEV);
    }  
    ks_debug(MMC_DISPLAY_SD_STATE, ( "*** In IDENT State ***\n"));

    /* TODO configure mmc driver depending on card attributes */
    if (verbose) 
    {
       printf("\n");
       if (host->mode == MMC_MODE_SD )
       {
           sd_cid_t *cid = (sd_cid_t *)resp;

           printf("SD device found. Card desciption (CID) is:\n");
           printf("Manufacturer ID = %02x\n", cid->mid);
           printf("OEM ID          = %02x%02x\n", cid->oid[0], cid->oid[1]);
           printf("Product Revision= %d.%d\n",((cid->hwrev >> 4) & 0x0f), (cid->hwrev & 0x0f) );
           printf("Product Name    = %s\n",cid->name);
           printf("Serial Number   = %02x%02x%02x%02x\n",
                                         cid->sn[0], cid->sn[1], cid->sn[2], cid->sn[3]);
           printf("Month           = %d\n",cid->month);
           printf("Year            = %d\n",(2000 + cid->year1));
       }
       else
       {
           mmc_cid_t *cid = (mmc_cid_t *)resp;

           printf("MMC found. Card desciption (CID) is:\n");
           printf("Manufacturer ID = %02x%02x%02x\n",
                                         cid->id[0], cid->id[1], cid->id[2]);
           printf("HW/FW Revision  = %x %x\n",cid->hwrev, cid->fwrev);
           cid->hwrev = cid->fwrev = 0;	/* null terminate string */
           printf("Product Name    = %s\n",cid->name);
           printf("Serial Number   = %02x%02x%02x\n",
                                         cid->sn[0], cid->sn[1], cid->sn[2]);
           printf("Month           = %d\n",cid->month);
           printf("Year            = %d\n",(1997 + cid->year));
       }
       printf("\n");
     }

     /* fill in device description */
     mmc_dev.if_type = IF_TYPE_MMC;
     mmc_dev.part_type = PART_TYPE_DOS;
     mmc_dev.dev = 0;
     mmc_dev.lun = 0;
     mmc_dev.type = 0;
     /* FIXME fill in the correct size (is set to 32MByte) */
     mmc_dev.blksz = 512;
     mmc_dev.lba = 0x10000;
     if (host->mode == MMC_MODE_SD )
     {
         sd_cid_t *cid = (sd_cid_t *)resp;

         sprintf(mmc_dev.vendor,"Man %02x Snr %02x%02x%02x%02x",
                 cid->mid, 
                 cid->sn[0], cid->sn[1], cid->sn[2], cid->sn[3]);
         sprintf(mmc_dev.product,"%s",cid->name);
         sprintf(mmc_dev.revision,"%x %x",((cid->hwrev>>4) & 0x0f), (cid->hwrev & 0x0f));
     }
     else
     {
         mmc_cid_t *cid = (mmc_cid_t *)resp;

         sprintf(mmc_dev.vendor,"Man %02x%02x%02x Snr %02x%02x%02x",
                 cid->id[0], cid->id[1], cid->id[2],
                 cid->sn[0], cid->sn[1], cid->sn[2]);
         sprintf(mmc_dev.product,"%s",cid->name);
         sprintf(mmc_dev.revision,"%x %x",cid->hwrev, cid->fwrev);
     }
     mmc_dev.removable = 0;
     mmc_dev.block_read = mmc_bread;

    /* 
     * Issue CMD3: Addressed commands with response (ac) - 
     * sent on CMD, response (R6) on CMD.
     * Let MMC into STBY state (assigns relative address to the card)
     */

    ks_debug(MMC_DBG_INIT,
             ( "------------------------------------------------------------------\n"));

    sd_cmd->opcode = MMC_CMD_SET_RCA;             /* CMD3 */
    sd_cmd->arg    = 0;                           /* Argument */
    sd_cmd->flags  = (MMC_RSP_R1|MMC_RSP_OPCODE); /* req Short Resp */
    sd_cmd->data   = NULL;  

    sd_mrq->stop = NULL;
    sd_mrq->data = NULL;
    sd_mrq->cmd  = sd_cmd;

    resp = &sd_cmd->resp[0];

    ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
    sdhci_request (mmc, sd_mrq);

    if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
    {
        ks_debug(MMC_DBG_ERROR,
                ("%s: Issue CMD%d not completed. host cmd error code %d\n",__FUNCTION__, sd_cmd->opcode, host->cmd->error));
        return (-ENODEV);
    }  
    ks_debug(MMC_DISPLAY_SD_STATE, ( "*** In STBY State ***\n"));

    card_rca = resp[0] ;
    ks_debug(MMC_DBG_INIT,("%s: resp[0]=0x%08x.\n",__FUNCTION__, resp[0]));

    /* 
     * Issue CMD9: Addressed commands with response (ac) - 
     * sent on CMD, response (R2) on CMD.
     * Get CSD (addressed card sends its CSD)
     */

    ks_debug(MMC_DBG_INIT,
             ( "------------------------------------------------------------------\n"));

    sd_cmd->opcode = MMC_CMD_SEND_CSD; /* CMD9 */
    sd_cmd->arg    = card_rca;         /* Argument RCA */
    sd_cmd->flags  = MMC_RSP_R2;       /* req Long Resp */
    sd_cmd->data   = NULL;  

    sd_mrq->stop = NULL;
    sd_mrq->data = NULL;
    sd_mrq->cmd  = sd_cmd;

    resp = &sd_cmd->resp[0];

    ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
    sdhci_request (mmc, sd_mrq);

    if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
    {
        ks_debug(MMC_DBG_ERROR,
                ("%s: Issue CMD%d not completed. host cmd error code %d\n",__FUNCTION__, sd_cmd->opcode, host->cmd->error));
        return (-ENODEV);
    }  

    csd = (sd_csd_t *)resp;
    memcpy(&mmc_csd, csd, sizeof(csd));
    rc = 0;
    mmc_ready = 1;

    /* FIXME add verbose printout for csd */
    if (verbose) 
    {
        printf("\n");
        printf("CSD configuration information is:\n");
        printf("Read data block length    = %d\n", (1 << csd->read_bl_len));
        printf("Read block partial        = %s\n", (csd->read_bl_partial ==1) ? "Yes" : "No");
        printf("Read block misalignment   = %s\n", (csd->read_blk_misalign == 1) ? "Yes" : "No");
        printf("Write data block length   = %d\n", (1 << csd->write_bl_len));
        printf("Write block partial       = %s\n", (csd->write_bl_partial == 1) ? "Yes" : "No");
        printf("Write block misalignment  = %s\n", (csd->write_blk_misalign == 1) ? "Yes" : "No");
        printf("Permanent write protection= %s\n", (csd->perm_write_protect == 1) ? "Yes" : "No");
        printf("Temporary write protection= %s\n", (csd->tmp_write_protect == 1) ? "Yes" : "No");
        printf("Write protection group    = %s\n", (csd->wp_grp_enable == 1) ? "Enable" : "Disable");
        printf("Erase single block        = %s\n", (csd->erase_single == 1) ? "Enable" : "Disable");
        printf("Erase sector size         = %d\n", (csd->erase_size+1));
        printf("File format               = %s\n", file_format[csd->file_format]);
        printf("\n");
    }

    /* 
     * Issue CMD7: Addressed commands with response (ac) - 
     * sent on CMD, response (R1b) on CMD.
     * Select card. Let MMC into TRAN state 
     */

    ks_debug(MMC_DBG_INIT,
             ( "------------------------------------------------------------------\n"));

    sd_cmd->opcode = 7;             /* CMD7 */
    sd_cmd->arg    = card_rca;       /* Argument RCA */
    sd_cmd->flags  = (MMC_RSP_R1|MMC_RSP_OPCODE);  /* req Short Resp */
    sd_cmd->data   = NULL;  

    sd_mrq->stop = NULL;
    sd_mrq->data = NULL;
    sd_mrq->cmd  = sd_cmd;

    resp = &sd_cmd->resp[0];

    ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
    sdhci_request (mmc, sd_mrq);

    if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
    {
        ks_debug(MMC_DBG_ERROR,
                ("%s: Issue CMD%d not completed. host cmd error code %d\n",__FUNCTION__, sd_cmd->opcode, host->cmd->error));
        return (-ENODEV);
    }  
    ks_debug(MMC_DISPLAY_SD_STATE, ( "*** In TRAN State ***\n"));
    ks_debug(MMC_DBG_INIT,
             ( "------------------------------------------------------------------\n"));

    /*
     * Issue CMD55: Addressed commands with response (ac) - 
     * sent on CMD, response (R1) on CMD.
     * Let SD card knows that next command is an application specific command 
     * rather than a standard one.
     */
    sd_cmd->opcode = 55;                          /* CMD55 */
    sd_cmd->arg    = card_rca;                    /* Argument RCA */
    sd_cmd->flags  = (MMC_RSP_R1|MMC_RSP_OPCODE); /* req Short Resp */
    sd_cmd->data   = NULL;  

    sd_mrq->stop = NULL;
    sd_mrq->data = NULL;
    sd_mrq->cmd  = sd_cmd;

    resp = &sd_cmd->resp[0];

    ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
    sdhci_request (mmc, sd_mrq);

    if ( sdhci_irq (host, /*SDHCI_INT_MASK*/ SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
    {
         ks_debug(MMC_DBG_ERROR,
                 ("%s: Issue CMD%d not completed. host cmd error code %d\n",
                 __FUNCTION__, sd_cmd->opcode, host->cmd->error));
         return (-ENODEV); 
    }  
          
    /* Check R1 Response data */
    if (resp[0] & R1_ANYERROR_MASK)
    {
         ks_debug(MMC_DBG_ERROR,
                 ("%s: ERROR on R1 Response 0x%08x\n", __FUNCTION__, (resp[0] & R1_ANYERROR_MASK) ));
         return (-ENODEV);
    }
    /* Check if ACMD is expected by SD device */
    else if ( !(resp[0] & APP_CMD))   
    {
         ks_debug(MMC_DBG_ERROR,
                 ("%s: ERROR on R1 Response (0x%08x). SD device is not expected AMCD. \n", 
                  __FUNCTION__, (resp[0] & R1_ANYERROR_MASK) ));
         return (-ENODEV);
    }
    else
         ks_debug(MMC_DISPLAY_SD_STATE,
                 ( "*** In %s State ***\n", sd_statet[(resp[0] & CURRENT_STATE_MASK) >> 9]) );

    /* 
     * Issue ACMD6: Addressed commands with response (ac) -  
     * sent on CMD, response (R1) on CMD.
     * Sets bus width (1 bit or 4 bit) to be used for data transfer. 
     */

    sd_cmd->opcode = 6;                           /* ACMD6 */
    sd_cmd->arg    = host->bus_width;             /* Argument (bus width) */
    sd_cmd->flags  = (MMC_RSP_R1|MMC_RSP_OPCODE); /* req Short Resp */
    sd_cmd->data   = NULL;  

    sd_mrq->stop = NULL;
    sd_mrq->data = NULL;
    sd_mrq->cmd  = sd_cmd;

    resp = &sd_cmd->resp[0];
    ks_debug(MMC_DBG_INIT,("%s: Issue CMD%d \n",__FUNCTION__, sd_cmd->opcode));
    sdhci_request (host->mmc, sd_mrq);

    if ( sdhci_irq (host, /*SDHCI_INT_MASK*/SDHCI_INT_CMD_MASK) != MMC_ERR_NONE)
    {
        ks_debug(MMC_DBG_ERROR,
                ("%s: Issue CMD%d not completed. host cmd error code %d\n",
                  __FUNCTION__, sd_cmd->opcode, host->cmd->error));
        return (-1);
    }   

    /* Check R1 Response data */
    if (resp[0] & R1_ANYERROR_MASK)
    {
        ks_debug(MMC_DBG_ERROR,
                 ("%s: ERROR on R1 Response 0x%08x\n", __FUNCTION__, (resp[0] & R1_ANYERROR_MASK) ));
        return (-1);
    }
    /* Check if ACMD is expected by SD device */
    else if ( !(resp[0] & APP_CMD))   
    {
         ks_debug(MMC_DBG_ERROR,
                 ("%s: ERROR on R1 Response (0x%08x). SD device is not expected AMCD. \n", 
                        __FUNCTION__, (resp[0] & R1_ANYERROR_MASK) ));
         return (-ENODEV);
    }
    else
        ks_debug(MMC_DISPLAY_SD_STATE,
                ( "*** In %s State ***\n", sd_statet[(resp[0] & CURRENT_STATE_MASK) >> 9]) );

    ks_debug(MMC_DBG_INIT,
             ( "------------------------------------------------------------------\n"));


    /* Register SD device */
    fat_register_device(&mmc_dev,1); /* partitions start counting with 1 */

#ifdef SD_TIMEOUT_PROBLEM
{
#include "../common/cmd_ks8692eth.h"

	bd_t *bd;

	eth_init(bd);
    eth_init_interrupt();
}
#endif


MMC_INIT_EXIT:
    if ( sd_cmd) 
       free(sd_cmd);
    if ( sd_mrq) 
       free(sd_mrq);
    if ( sd_data) 
       free(sd_data);
    if ( statusBuf) 
       free(statusBuf);

    ks_debug(MMC_DBG_INIT,("%s: exit.\n",__FUNCTION__ ));
    return rc;

}

int mmc_ident
(
    block_dev_desc_t *dev
)
{
    ks_debug(MMC_DBG_INIT,("%s: dev=0x%lx\n",__FUNCTION__, dev ));
    return 0;
}

int mmc2info
(   
    ulong addr
)
{
    ks_debug(MMC_DBG_INIT,("%s: addr=0x%lx\n",__FUNCTION__, addr ));

    /* FIXME hard codes to 32 MB device */
    if (addr >= CFG_MMC_BASE && addr < CFG_MMC_BASE + 0x02000000) 
    {
       return 1;
    }
    return 0;
}


#ifdef CONFIG_USE_IRQ

/*
 * sd_isr()
 *
 * ISR to test SD/SDIO interrupt bits. 
 * Do nothing, but just clear SD interrupt. 
 */
void sd_isr(void)
{
    unsigned long intStatus;
    unsigned long sdIntStatus;
    unsigned long intReg;

    intStatus = ks8692_read_dword(KS8692_INT_STATUS1);
    sdIntStatus = ks8692_read_dword(SDIO_HOST_BASE + SDHCI_INT_STATUS);

    ks_debug(MMC_DBG_INT,
            ("%s: intEnable  =0x%08X:%08x\n", __FUNCTION__, KS8692_INT_ENABLE1, ks8692_read_dword(KS8692_INT_ENABLE1)));
    ks_debug(MMC_DBG_INT,
            ("%s: intStatus  =0x%08X:%08x \n", __FUNCTION__, KS8692_INT_STATUS1, intStatus));
    ks_debug(MMC_DBG_INT,
            ("%s: sdIntStatus=0x%08X:%08x \n", __FUNCTION__, (SDIO_HOST_BASE + SDHCI_INT_STATUS), sdIntStatus ));

    /* Do nothing, but just clear interrupt, and disable interrupt */
   
    if ( sdIntStatus & (SDHCI_INT_CARD_INT | SDHCI_INT_CARD_INSERT | SDHCI_INT_CARD_REMOVE) )
    {
       ks_write((SDIO_HOST_BASE + SDHCI_INT_STATUS), sdIntStatus);
    }
    else
    {
       intReg = ks8692_read_dword(SDIO_HOST_BASE+SDHCI_SIGNAL_ENABLE);
       ks_write((SDIO_HOST_BASE + SDHCI_SIGNAL_ENABLE), (intReg & ~sdIntStatus));
    }

    ks8692_write_dword(KS8692_INT_STATUS1, (intStatus & INT_SDIO)); 

    ks_debug(MMC_DBG_INT,
            ("%s: %08X:%08x \n", __FUNCTION__, (SDIO_HOST_BASE + SDHCI_INT_STATUS), sdIntStatus ));
    ks_debug(MMC_DBG_INT,
            ("%s: %08X:%08x\n", __FUNCTION__, 
            (SDIO_HOST_BASE + SDHCI_INT_ENABLE), ks8692_read_dword(SDIO_HOST_BASE + SDHCI_INT_ENABLE)));
    ks_debug(MMC_DBG_INT,
            ("%s: %08X:%08x\n", __FUNCTION__, 
            (SDIO_HOST_BASE + SDHCI_SIGNAL_ENABLE), ks8692_read_dword(SDIO_HOST_BASE + SDHCI_SIGNAL_ENABLE)));

}


/*
 * sd_init_interrupt()
 *
 * Hook up ISR and enable SD interrupt to test SD interrupt bits. 
 * irq 7 is SD interrupt. 
 */
void sd_init_interrupt 
(
    unsigned int intmask
)
{    
    unsigned long intReg;

    /* initialize irq software vector table */
    interrupt_init();

    /* Clear SD interrupt */
    intReg = ks8692_read_dword(KS8692_INT_STATUS1);
    ks8692_write_dword(KS8692_INT_STATUS1, (intReg & INT_SDIO )); 

    /* install sd_isr() */
    irq_install_handler(7, (interrupt_handler_t *)sd_isr, NULL);

    /* enable SD interrupt bit */
    intReg = ks8692_read_dword(KS8692_INT_ENABLE1);
    ks8692_write_dword(KS8692_INT_ENABLE1, (intReg | INT_SDIO )); 

    /* enable SD Host Controller interrupt source bits */
    ks_write((SDIO_HOST_BASE + SDHCI_SIGNAL_ENABLE), intmask);


    ks_debug(MMC_DBG_INT,
          ("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_STATUS1, ks8692_read_dword(KS8692_INT_STATUS1)));
    ks_debug(MMC_DBG_INT,
          ("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_ENABLE1, ks8692_read_dword(KS8692_INT_ENABLE1)));
    ks_debug(MMC_DBG_INT,
          ("%s: %08X:%08x\n", __FUNCTION__, 
          (SDIO_HOST_BASE + SDHCI_INT_ENABLE), ks8692_read_dword(SDIO_HOST_BASE + SDHCI_INT_ENABLE)));
    ks_debug(MMC_DBG_INT,
          ("%s: %08X:%08x\n", __FUNCTION__, 
          (SDIO_HOST_BASE + SDHCI_SIGNAL_ENABLE), ks8692_read_dword(SDIO_HOST_BASE + SDHCI_SIGNAL_ENABLE)));
}

#endif /* #ifdef CONFIG_USE_IRQ */

#endif	/* CONFIG_MMC */
