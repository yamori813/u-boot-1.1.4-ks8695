/*
 * Copyright (C) 2006 Micrel Inc.
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
 * SPI Read/Write utilities support for Micrel KS8692
 * (ARM922T code) SPI Controller. 
 * SPI controller is interface with KS8995MA SPI device.
 * or AT25160A EEPROM 
 */

#include <common.h>
#include <linux/ctype.h>
#include <spi.h>
#include <asm/arch/platform.h>


#if (1)
#define SPI_AT25160A      /* EEPROM */
#endif


/* KS8692 SPI registers mapping structure */
typedef volatile struct ks_spi_t {
    ulong   spi_ctr;    /* Control reg */
    ulong   spi_rdr;    /* Receive data reg */
    ulong   spi_tdr;    /* Transmit data reg */
    ulong   spi_mcr;    /* Micrel mode transmit data reg */
    ulong   spi_bfr;    /* Buffer status reg */
    ulong   spi_isr;    /* Interrupt status reg */
    ulong   spi_ier;    /* Interrupt Enable reg */
    ulong   spi_csr;    /* Chip select reg */
}ks_spi_t;

#define SPI_WRITE_CMD        0x02    // KS8895 SPI Write Command 
#define SPI_READ_CMD         0x03    // KS8895 SPI Read  Command 
#define SPI_WRDI_CMD         0x04    /* SPI device write protect Command */
#define SPI_RDSR_CMD         0x05    /* SPI device read status Command */
#define SPI_WREN_CMD         0x06    /* SPI device write enable Command */


#define MSB(x)	       (((x) >> 8) & 0xff)	   /* most signif byte of 2-byte integer */
#define LSB(x)	       ((x) & 0xff)		       /* least signif byte of 2-byte integer*/
#define SWAP_WORD(x)   (MSB(x) | LSB(x) << 8)


#define SPI_ADDRESS_OFFSET_LEN      1   /* 1-byte of address offset */
#define	CFG_KS_SPI0_BITS           16	/* SPI transfer data with (bit) */

#ifdef SPI_AT25160A
#undef SPI_ADDRESS_OFFSET_LEN
#undef CFG_KS_SPI0_BITS
#define SPI_ADDRESS_OFFSET_LEN      2   /* 2-byte of address offset */
#define	CFG_KS_SPI0_BITS            8	/* SPI transfer data with (bit) */
#endif


#if (CFG_KS_SPI0_BITS != 8) && (CFG_KS_SPI0_BITS != 16) && (CFG_KS_SPI0_BITS != 32)
#error "*** CFG_NIOS_SPIBITS should be either 8, 16 or 32 ***"
#endif

#if (0)
#define	DEBUG
#endif

#ifdef  DEBUG

#define	DPRINT(a)	printf a;
/* -----------------------------------------------
 * Helper functions to peek into tx and rx buffers
 * ----------------------------------------------- */
static const char * const hex_digit = "0123456789ABCDEF";

static char quickhex (int i)
{
	return hex_digit[i];
}

static void memdump (void *pv, int num)
{
	int i;
	unsigned char *pc = (unsigned char *) pv;

	for (i = 0; i < num; i++)
		printf ("%c%c ", quickhex (pc[i] >> 4), quickhex (pc[i] & 0x0f));
	printf ("\t");
	for (i = 0; i < num; i++)
		printf ("%c", isprint (pc[i]) ? pc[i] : '.');
	printf ("\n");
}
#else   /* !DEBUG */

#define	DPRINT(a)
#define	memdump(p,n)

#endif  /* DEBUG */


#if (CONFIG_COMMANDS & CFG_CMD_SPI)

ks_spi_t *ksSpi = (ks_spi_t *)(KS8692_IO_BASE + KS8692_SPI_BASE );


/* function protocol */

void spi_register_display (void);
void spi_reset (void);
void spi_init (void);
static ssize_t spi_issue_cmd ( uchar );
int  spi_xfer ( spi_chipsel_type, int, uchar *, uchar * );
void spi_ks_chipsel ( int );

void spi_disable_interrupt ( void );
void spi_init_interrupt ( void );
void spi_isr ( void );

static ssize_t spi_read_byte ( ushort, uchar *, int);
static ssize_t spi_read_word ( uchar, ushort *, int);
static ssize_t spi_read_dword ( uchar, ulong *, int);
static ssize_t spi_write_byte ( ushort, uchar *, uchar *, int );
static ssize_t spi_write_word ( uchar, ushort *, ushort *, int );
static ssize_t spi_write_dword ( uchar, ulong *, ulong *, int );


/*
 * spi_register_display
 *
 * Display all the SPI registers.
 */
void spi_register_display ()
{
    DPRINT(("spi_ctr %08X:%08x \n", &ksSpi->spi_ctr, ksSpi->spi_ctr));
    DPRINT(("spi_rdr %08X:%08x \n", &ksSpi->spi_rdr, ksSpi->spi_rdr));
    DPRINT(("spi_tdr %08X:%08x \n", &ksSpi->spi_tdr, ksSpi->spi_tdr));
    DPRINT(("spi_mcr %08X:%08x \n", &ksSpi->spi_mcr, ksSpi->spi_mcr));
    DPRINT(("spi_bfr %08X:%08x \n", &ksSpi->spi_bfr, ksSpi->spi_bfr));
    DPRINT(("spi_isr %08X:%08x \n", &ksSpi->spi_isr, ksSpi->spi_isr));
    DPRINT(("spi_ier %08X:%08x \n", &ksSpi->spi_ier, ksSpi->spi_ier));
    DPRINT(("spi_csr %08X:%08x \n", &ksSpi->spi_csr, ksSpi->spi_csr));

}

/* 
 * WaitForTDBU: Wait for TxBuf empty (SPI_ISR). 
 */
static __inline__ int WaitForTDBU (void)
{
    int    timeOut=20;
    unsigned long status;

    /* Wait for SPI transmit buffer empty */
    status = ksSpi->spi_isr; 
    while ( (--timeOut > 0 ) && (status & SPI_INT_TDBU) ) 
    {
        udelay(10);
        status = ksSpi->spi_isr; 
    };

    if ( status & SPI_INT_TDBU ) 
    {
        printf ("%s: Time out while waiting for TDBU! spi_isr %08X:%08x\n", __FUNCTION__, &ksSpi->spi_isr, status);
        return ( -1 );
    }
    return ( 0 ) ;
}

/* 
 * WaitForRDRDY: Wait for RxBuf ready (SPI_ISR). 
 */
static __inline__ int WaitForRDRDY (void)
{
    int    timeOut=200;
    unsigned long status;

    /* Wait for SPI receive buffer ready */
    status = ksSpi->spi_isr; 
    while ( (--timeOut > 0 ) && ( !(status & SPI_INT_RDRDY)) ) 
    {
        udelay(10);
        status = ksSpi->spi_isr; 
    };

    if ( !(status & SPI_INT_RDRDY) ) 
    {
        printf ("%s: Time out while waiting for RDRDY! spi_isr %08X:%08x\n", __FUNCTION__, &ksSpi->spi_isr, status);
        return ( -1 );
    }

    return ( 0 ) ;
}

/*
 * spi_reset
 *
 * Reset SPI controller.
 */
void spi_reset ()
{

    DPRINT(("%s: \n", __FUNCTION__));

    /* reset */
    ksSpi->spi_ctr = SPI_RESET;	   
    udelay(100);
}

/*
 * spi_init
 *
 * Init SPI register default values.
 */
void spi_init ()
{


    DPRINT(("%s: \n", __FUNCTION__));


    ksSpi->spi_csr |= ( SPI_SYSCLK_BY128 | SPI_DATA_DELAY_1 | SPI_CS_DELAY_1);

#ifdef SPI_AT25160A
    /* mode 2  CPOL=1, CPHA=0 */
    ksSpi->spi_csr |=  ( SPI_SPCK_INACTIVE_HIGH ); 
    ksSpi->spi_csr &= ~( SPI_DATA_LEADING_EDGE ); 
#endif

    #if (0)
    /* mode 0  CPOL=0, CPHA=0 (hardware default value) */
    ksSpi->spi_csr &= ~( SPI_SPCK_INACTIVE_HIGH | SPI_DATA_LEADING_EDGE ); 
    #endif

    #if (0)
    /* mode 1  CPOL=0, CPHA=1 */
    ksSpi->spi_csr &= ~( SPI_SPCK_INACTIVE_HIGH ); 
    ksSpi->spi_csr |=  ( SPI_DATA_LEADING_EDGE ); 
    #endif

    #if (0)
    /* mode 2  CPOL=1, CPHA=0 */
    ksSpi->spi_csr |=  ( SPI_SPCK_INACTIVE_HIGH ); 
    ksSpi->spi_csr &= ~( SPI_DATA_LEADING_EDGE ); 
    #endif

    #if (0)
    /* mode 3  CPOL=1, CPHA=1 */
    ksSpi->spi_csr |=  ( SPI_SPCK_INACTIVE_HIGH | SPI_DATA_LEADING_EDGE ); 
    #endif

    udelay(10);
}

/*
 * write_enable
 *
 * Send SPI write enable cpmmand to spi target.
 */
static ssize_t write_enable
(
    uchar   cmd
)
{
    unsigned long regData=0;


    DPRINT(("%s: cmd=%02x\n", __FUNCTION__, cmd));
 
    /* Wait for SPI transmit buffer empty */
    if ( WaitForTDBU () != 0 )
        return -1; 

    /* Transfer WRITE command, SPI target register offset to SPI controller SPI_TDR */
    regData = ( (cmd << 16) | SPI_TX_CS_END );
    ksSpi->spi_tdr = regData;
    DPRINT(("%s: spi_tdr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_tdr, regData ));

    return ( 0 );
}

/*
 * spi_issue_cmd
 *
 * Send SPI read\write cpmmand to spi target.
 */
static ssize_t spi_issue_cmd
(
    uchar   cmd
)
{
    unsigned long regData=0;


    DPRINT(("%s: cmd=%02x\n", __FUNCTION__, cmd));
 
    /* Wait for SPI transmit buffer empty */
    if ( WaitForTDBU () != 0 )
        return -1; 

    /* Transfer WRITE command, SPI target register offset to SPI controller SPI_TDR */
    regData = (cmd << 16) ;
    ksSpi->spi_tdr = regData;
    DPRINT(("%s: spi_tdr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_tdr, regData ));

    return ( 0 );
}


/*
 * spi_read_byte
 *
 * Read data 'length' from SPI slave register, and store in 'bReadData' buffer
 * by BYTE (8 bit per SPI transfer).
 */
static ssize_t spi_read_byte
(
    ushort  regOffset,
    uchar  *bReadData,
	int     length
)
{
    unsigned long regData=0;
    int    j, readLen=0;
    uchar  *pOffset = (uchar *)&regOffset;


    DPRINT(("%s: regOffset=%04X, length=%d \n", __FUNCTION__, regOffset, length));

    /* Issue SPI READ cmd */
    if  ( spi_issue_cmd (SPI_READ_CMD) != 0 )
        return -1;

    /* Wait for SPI transmit buffer empty */
    if ( WaitForTDBU () != 0 )
        return -1; 

    /* Write SPI slave register offset to SPI controller SPI_TDR */
    for (j=0; j<SPI_ADDRESS_OFFSET_LEN; j++, pOffset++) {
        regData = (*pOffset << 16);
        if (j == (SPI_ADDRESS_OFFSET_LEN - 1) ) /* last byte of address offset */
           regData |= ((SPI_TX_CS_END | SPI_TX_HIZEXT_ENABLE | 
                       (length & SPI_TX_CEXT_MASK)) );
        ksSpi->spi_tdr = regData;
        DPRINT(("%s: spi_tdr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_tdr, regData ));
    }

    /* Read SPI target register value from SPI controller SPI_RDR */
    for ( j=0; j<length; j++ )
    {
        /* Wait for SPI receive buffer ready */
        if ( WaitForRDRDY () != 0 )
            return -1; 
	
        regData = ksSpi->spi_rdr;
        DPRINT(("%s: spi_rdr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_rdr, regData));

        *bReadData++ = (uchar)( (regData >> 16) & 0x000000ff );
        readLen++;
    }

    return (readLen);
}

/*
 * spi_read_word
 *
 * Read data 'length' from SPI slave register, and store in 'bReadData' buffer
 * by WORD (16 bit per SPI transfer).
 */
static ssize_t spi_read_word
(
    uchar   regOffset,
    ushort *wReadData,
    int     length
)
{
    unsigned long regData=0;
    int    j, readLen=0;


    DPRINT(("%s: regOffset=%02X, length=%d \n", __FUNCTION__, regOffset, length));
 
    /* Wait for SPI transmit buffer empty */
    if ( WaitForTDBU () != 0 )
        return -1; 

    /* Write SPI READ cmd, SPI slave register offset to SPI controller SPI_TDR */
    regData = ((SPI_READ_CMD << 24) | (regOffset << 16) | 
               (SPI_TX_CS_END | SPI_TX_HIZEXT_ENABLE | SPI_TX_16_BITS | 
               (length & SPI_TX_CEXT_MASK)) );
    ksSpi->spi_tdr = regData;
    DPRINT(("%s: spi_tdr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_tdr, ksSpi->spi_tdr));


    /* Read SPI target register value from SPI controller SPI_RDR */
    for ( j=0; j<length; j+=2 )
    {
        /* Wait for SPI receive buffer ready */
        if ( WaitForRDRDY () != 0 )
            return -1; 
	
        regData = ksSpi->spi_rdr;
        DPRINT(("%s: spi_rdr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_rdr, regData));

        *wReadData = (ushort)( (regData >> 16) & 0x0000ffff );
        *wReadData = SWAP_WORD( *wReadData );
        wReadData++;
          
        readLen += 2;
    }

    return (readLen);
}


/*
 * spi_read_dword
 *
 * Read data 'length' from SPI slave register, and store in 'bReadData' buffer
 * by DWORD (32 bit per SPI transfer - Micrel mode).
 */
static ssize_t spi_read_dword
(
    uchar   regOffset,
    ulong  *dwReadData,
    int     length
)
{
    unsigned long regData=0;
    int    j, readLen=0;


    DPRINT(("%s: regOffset=%02X, length=%d \n", __FUNCTION__, regOffset, length));
 
    /* Wait for SPI transmit buffer empty */
    if ( WaitForTDBU () != 0 )
        return -1; 

    /* Write SPI READ cmd, SPI slave register offset to SPI controller SPI_MCR */
    regData = ( (SPI_READ_CMD << 16) | (regOffset << 8) );
    ksSpi->spi_mcr = regData;
    DPRINT(("%s: spi_mcr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_mcr, regData));

    /* Issue READ operation to SPI controller SPI_TDR */
    regData = ( SPI_TX_CS_END | SPI_MICREL_MODE | (length & SPI_TX_CEXT_MASK) );
    ksSpi->spi_tdr = regData;
    DPRINT(("%s: spi_tdr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_tdr, ksSpi->spi_tdr));

    /* Read SPI target register value from SPI controller SPI_RDR (32bit) */
    for ( j=0; j<length; j+=4 )
    {
        /* Wait for SPI receive buffer ready */
        if ( WaitForRDRDY () != 0 )
            return -1; 
	
        regData = ksSpi->spi_rdr;
        DPRINT(("%s: spi_rdr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_rdr, regData));

        *dwReadData++ = regData;
        readLen += 4;
    }

    return (readLen);
}


/*
 * spi_write_byte
 *
 * Write data 'length' to  SPI slave register,
 * by BYTE (8 bit per SPI transfer).
 */
static ssize_t spi_write_byte
(
    ushort  regOffset,
    uchar  *bWriteData,
    uchar  *bReadData,
    int     length
)
{
    unsigned long regData=0;
    int           j, writeLen=0;
    unsigned long intStatus;
    uchar  *pOffset = (uchar *)&regOffset;


    DPRINT(("%s: regOffset=%04X, length=%d \n", __FUNCTION__, regOffset, length));
 
#ifdef SPI_AT25160A
    /* Enable SPI WRITE cmd */
    if  ( write_enable (SPI_WREN_CMD) != 0 )
        return -1;
#endif

    /* Issue SPI WRITE cmd */
    if  ( spi_issue_cmd (SPI_WRITE_CMD) != 0 )
        return -1;

    /* Wait for SPI transmit buffer empty */
    if ( WaitForTDBU () != 0 )
        return -1; 

    /* Write SPI slave register offset througth SPI controller SPI_TDR */
    for (j=0; j<SPI_ADDRESS_OFFSET_LEN; j++, pOffset++) {
        regData = (*pOffset << 16);
        ksSpi->spi_tdr = regData;
        DPRINT(("%s: spi_tdr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_tdr, ksSpi->spi_tdr));
    }

    /* Write data to SPI slave register througth SPI controller SPI_TDR */
    for ( j=0; j<length; j++ )
    {
        /* Wait for SPI transmit buffer empty */
        if ( WaitForTDBU () != 0 )
            return -1; 

        regData = ( *bWriteData++ << 16 );
        if ( j == (length-1) )
            regData |= ( SPI_TX_CS_END );

        ksSpi->spi_tdr = regData;
        DPRINT(("%s: spi_tdr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_tdr, ksSpi->spi_tdr));

        writeLen++;
    }
    
    /* 
     * Read them back
     */
#if (1)
    udelay(10);
    spi_read_byte ( regOffset, bReadData, length );
#endif

    return (writeLen);
}

/*
 * spi_write_word
 *
 * Write data 'length' to  SPI slave register,
 * by WORD (16 bit per SPI transfer).
 */
static ssize_t spi_write_word
(
    uchar   regOffset,
    ushort *wWriteData,
    ushort *wReadData,
    int     length
)
{
    unsigned long regData=0;
    int           j, writeLen=0;
    unsigned long intStatus;


    DPRINT(("%s: regOffset=%02X, length=%d \n", __FUNCTION__, regOffset, length));

 
    /* Wait for SPI transmit buffer empty */
    if ( WaitForTDBU () != 0 )
        return -1; 

    /* Write SPI WRITE cmd, SPI slave register offset througth SPI controller SPI_TDR */
    regData = ((SPI_WRITE_CMD << 24) | (regOffset << 16) | SPI_TX_16_BITS );
    ksSpi->spi_tdr = regData;
    DPRINT(("%s: spi_tdr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_tdr, ksSpi->spi_tdr));

    /* Write data to SPI slave register througth SPI controller SPI_TDR */
    for ( j=0; j<length; j+=2 )
    {
        /* Wait for SPI transmit buffer empty */
        if ( WaitForTDBU () != 0 )
            return -1; 

        *wWriteData = SWAP_WORD( *wWriteData );
        regData = ( (*wWriteData++ << 16) | SPI_TX_16_BITS );
        if ( j == (length-2) )
            regData |= ( SPI_TX_CS_END );

        ksSpi->spi_tdr = regData;
        DPRINT(("%s: spi_tdr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_tdr, ksSpi->spi_tdr));
        writeLen+=2;
    }
    
    /* 
     * Read them back
     */
    // spi_read_word ( regOffset, wReadData, length );

    return (writeLen);
}


/*
 * spi_write_dword
 *
 * Write data 'length' to  SPI slave register,
 * by DWORD (32 bit per SPI transfer - MICREL mode).
 */
static ssize_t spi_write_dword
(
    uchar   regOffset,
    ulong  *dwWriteData,
    ulong  *dwReadData,
    int     length
)
{
    unsigned long regData=0;
    int           j, writeLen=0;


    DPRINT(("%s: regOffset=%02X, length=%d \n", __FUNCTION__, regOffset, length));

 
    /* Wait for SPI transmit buffer empty */
    if ( WaitForTDBU () != 0 )
        return -1; 

    /* Write SPI WRITE cmd, SPI slave register offset to SPI controller SPI_MCR */
    regData = ( (SPI_WRITE_CMD << 16) | (regOffset << 8) ); 
    ksSpi->spi_mcr = regData;
    DPRINT(("%s: spi_mcr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_mcr, regData));

    /* Issue WRITE operation to SPI controller SPI_TDR */
    regData = ( SPI_MICREL_MODE | SPI_TX_HIZEXT_ENABLE );
    ksSpi->spi_tdr = regData;
    DPRINT(("%s: spi_tdr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_tdr, ksSpi->spi_tdr));


    /* Write data to SPI slave register througth SPI controller SPI_MCR */
    for ( j=0; j<length; j+=4 )
    {
        /* Wait for SPI transmit buffer empty */
        if ( WaitForTDBU () != 0 )
            return -1; 

        if ( j == (length-4) )
        {
            regData = ksSpi->spi_tdr;
            regData |= ( SPI_TX_CS_END );
            ksSpi->spi_tdr = regData;
            DPRINT(("%s: spi_tdr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_tdr, ksSpi->spi_tdr));
        }

        ksSpi->spi_mcr = *dwWriteData++;
        DPRINT(("%s: spi_mcr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_mcr, ksSpi->spi_mcr));
        writeLen+=4;
    }
    
    /* 
     * Read them back
     */
    spi_read_dword ( regOffset, dwReadData, length );

    return (writeLen);
}


/*
 * SPI transfer:
 *
 * See include/spi.h and http://www.altera.com/literature/ds/ds_nios_spi.pdf
 * for more informations.
 * Implement spi uboot cmd.
 * Test spi slave target is ks8995.
 * eg. cmd from uboot:
 * 
 *     sspi 0 8  0300      -> read 1 byte from spi slave address offset 0
 *     sspi 0 16 0360      -> read 2 byte from spi slave address offset 0x60
 *     sspi 0 32 0360      -> read 4 byte from spi slave address offset 0x60
 *
 *     sspi 0 8  026012        -> write 1 byte to spi slave address offset 0x60 with value 0x12
 *     sspi 0 16 02601234      -> write 2 byte to spi slave address offset 0x60 with value 0x1234
 *     sspi 0 32 0260abcd5678  -> write 4 byte to spi slave address offset 0x60 with value 0xabcd5678
 *
 */
int spi_xfer
(
    spi_chipsel_type chipsel, 
    int bitlen, 
    uchar *dout, 
    uchar *din
)
{
    int    writeLen, readLen=0;


    writeLen = (bitlen + 7) / 8;

    DPRINT(("spi_xfer: chipsel %08X dout %08X din %08X bitlen %d writeLen %d\n",
		(int)chipsel, *(uint *)dout, *(uint *)din, bitlen, writeLen));

    spi_reset();
    spi_init();

    if(chipsel != NULL) {

#ifdef CONFIG_USE_IRQ
       /* Initialize SPI interrupt */
       spi_init_interrupt ();
#endif
       chipsel(1);	/* Enable SPI controller */
    }

#if	(CFG_KS_SPI0_BITS == 8)
    /* 
     * 8 bit Per SPI Transfer.  
     */
    if ( *dout == SPI_READ_CMD )
    {
        /* read data from SPI slave (ks8995) */
        readLen += spi_read_byte ( dout[1], &din[0], writeLen );
    }
    else if ( *dout == SPI_WRITE_CMD )
    {
        /* write data to SPI slave (ks8995) */
        spi_write_byte ( dout[1], &dout[2], &din[0], writeLen );
    }

#elif (CFG_KS_SPI0_BITS == 16)
    /* 
     * 16 bit Per SPI Transfer.  
     */
    if ( *dout == SPI_READ_CMD )
    {
        /* read data from SPI slave (ks8995) */
        readLen += spi_read_word ( dout[1], &din[0], writeLen );
    }
    else if ( *dout == SPI_WRITE_CMD )
    {
        /* write data to SPI slave (ks8995) */
        spi_write_word ( dout[1], (ushort *)&dout[2], (ushort *)&din[0], writeLen );
    }

#elif (CFG_KS_SPI0_BITS == 32)
    /* 
     * 32 bit Per SPI Transfer (Micrel mode).  
     */
    if ( *dout == SPI_READ_CMD )
    {
        /* read data from SPI slave () */
        readLen += spi_read_dword ( dout[1], &din[0], writeLen );
    }
    else if ( *dout == SPI_WRITE_CMD )
    {
        /* write data to SPI slave () */
        spi_write_dword ( dout[1], (ulong *)&dout[2], (ulong *)&din[0], writeLen );
    }

#else
#error "spi_xfer: *** unsupported value of CFG_KS_SPI0_BITS ***"
#endif

    if(chipsel != NULL) {
       chipsel(0);	/* Disable SPI controller */

#ifdef CONFIG_USE_IRQ
       /* disable SPI interrupt */
       spi_disable_interrupt (); 
#endif
    }

    return 0;
}


/*============================================================================
 * The following are used to control the SPI chip selects for the SPI command.
 *============================================================================*/

void spi_ks_chipsel(int cs)
{

    DPRINT(("%s: cs %d\n", __FUNCTION__, cs));

    if (cs)
       ksSpi->spi_ctr = SPI_ENABLE;	   /* enable SPI (1) */
    else
       ksSpi->spi_ctr = 0;             /* disable SPI (0) */

    udelay(10);
}

/*
 * The SPI command uses this table of functions for controlling the SPI
 * chip selects: it calls the appropriate function to control the SPI
 * chip selects.
 */
spi_chipsel_type spi_chipsel[] = {
	spi_ks_chipsel
};
int spi_chipsel_cnt = sizeof(spi_chipsel) / sizeof(spi_chipsel[0]);


#ifdef CONFIG_USE_IRQ


/*
 * spi_isr()
 *
 * ISR to test SPI interrupt bits. 
 * Do nothing, but just clear SPI interrupt. 
 */
void spi_isr(void)
{
    unsigned long intStatus;
    unsigned long status;

    intStatus = ks8692_read_dword(KS8692_INT_STATUS2);
    status = ksSpi->spi_isr ;


    /* For test only: disable SPI_INT_RDRDY interrupt */
    if (status  & SPI_INT_RDRDY)
    {
        ksSpi->spi_ier &= ~SPI_INT_RDRDY;
    }
    if (status  & SPI_INT_RDBU)
    {
        ksSpi->spi_ier &= ~SPI_INT_RDBU;
    }

#if (1)
    printf("%s: intStatus=0x%08X:%08x \n", __FUNCTION__, KS8692_INT_STATUS2, intStatus);
    printf("%s: spi_isr=%08X:%08x \n", __FUNCTION__, &ksSpi->spi_isr, status );
    printf("%s: spi_ier=%08X:%08x\n", __FUNCTION__, &ksSpi->spi_ier, ksSpi->spi_ier );
    printf("%s: spi_bfr %08X:%08x \n", __FUNCTION__, &ksSpi->spi_bfr, ksSpi->spi_bfr);
#endif

    /* Do nothing, but just clear interrupt */
    ks8692_write_dword(KS8692_INT_STATUS2, (intStatus & INT_SPI)); 
}


/*
 * spi_disable_interrupt()
 *
 * Unhook up ISR and disable SPI interrupt. 
 * irq 43 is SPI interrupt. 
 */
void spi_disable_interrupt ()
{    
    unsigned long intReg;

    /* disable SPI interrupt bit */
    intReg = ks8692_read_dword(KS8692_INT_ENABLE2);
    ks8692_write_dword(KS8692_INT_ENABLE2, (intReg & ~INT_SPI )); 

    /* disable SPI Controller interrupt source */
    ksSpi->spi_ier = 0;

#if (1)
    printf("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_STATUS2, ks8692_read_dword(KS8692_INT_STATUS2) );
    printf("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_ENABLE2, ks8692_read_dword(KS8692_INT_ENABLE2) );
    printf("%s: %08X:%08x\n", __FUNCTION__, &ksSpi->spi_ier, ksSpi->spi_ier );
#endif

    /* install spi_isr() */
    irq_free_handler( 11+32 );

}

/*
 * spi_init_interrupt()
 *
 * Hook up ISR and enable SPI interrupt to test SPI interrupt bits. 
 * irq 43 is SPI interrupt. 
 */
void spi_init_interrupt ()
{    
    unsigned long intReg;

    /* initialize irq software vector table */
    interrupt_init();

    /* Clear SPI interrupt bits */
    intReg = ks8692_read_dword(KS8692_INT_STATUS2);
    ks8692_write_dword(KS8692_INT_STATUS2, (intReg & INT_SPI )); 

    /* install spi_isr() */
	irq_install_handler((11+32), (interrupt_handler_t *)spi_isr, NULL);

    /* enable SPI interrupt bit */
    intReg = ks8692_read_dword(KS8692_INT_ENABLE2);
    ks8692_write_dword(KS8692_INT_ENABLE2, (intReg | INT_SPI )); 

    /* enable SPI Controller interrupt source */
    ksSpi->spi_ier = ( SPI_INT_RDBU | SPI_INT_RDRDY | SPI_INT_XRDY | SPI_INT_TDBU | SPI_INT_TDTH);

#if (1)
    printf("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_STATUS2, ks8692_read_dword(KS8692_INT_STATUS2) );
    printf("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_ENABLE2, ks8692_read_dword(KS8692_INT_ENABLE2) );
    printf("%s: %08X:%08x\n", __FUNCTION__, &ksSpi->spi_ier, ksSpi->spi_ier );
#endif
}

#endif /* #ifdef CONFIG_USE_IRQ */


#endif /* #if (CONFIG_COMMANDS & CFG_CMD_SPI) */ 
