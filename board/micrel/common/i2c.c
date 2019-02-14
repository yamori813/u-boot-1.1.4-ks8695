/*
 * Copyright (C) 2007 Micrel Inc.
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
 * I2C Read/Write utilities support for Micrel KS8692
 * (ARM922T code) I2C Controller with Atmel AT24C128 EEPROM I2C device,
 * and Wolfson WM8974 I2C configuration interface.
 * Atmel AT24C128 EEPROM write operation requires two-8bit data word addresses 
 * following the device address word and acknowledgment, then data bytes.
 * WM8974 write operation requests controller sends the 1st byte of 
 * control data (b15 to b8, i.e. the WM8974 register address plus the 1st bit
 * of register data), then, the 2nd byte of control data (b7 to b0, i.e. the 
 * remaining 8 bits of register data). 
 */

#include <common.h>
#include <linux/ctype.h>
#include <i2c.h>
#include <asm/arch/platform.h>


/* KS8692 I2C registers mapping structure */
typedef volatile struct ks_i2c_t {
    ulong   i2c_gcr;    /* Global control reg */
    ulong   i2c_ar ;    /* I2C address data reg */
    ulong   i2c_scr;    /* I2C start command reg */
    ulong   i2c_sr ;    /* I2C status reg */
    ulong   i2c_rdr0;   /* Read Data0 reg */
    ulong   i2c_rdr1;   /* Read Data1 reg */
    ulong   i2c_wdr0;   /* Write Data0 reg */
    ulong   i2c_wdr1;   /* Write Data1 reg */
    ulong   i2c_debug;  /* Debug reg */
}ks_i2c_t;


#define I2C_OK		     0
#define I2C_NOK		    -1
#define I2C_NACK	     2
#define I2C_NOK_LA	     3		/* Lost arbitration */
#define I2C_NOK_TOUT     4		/* time out */

#define I2C_WRITE_FLAG   0
#define I2C_READ_FLAG    1

#define I2C_TIMEOUT      6	      /* 6 second */

#define ATMEL_DEVICE_ADDR       0x50  /* Atmel AT24C128 EEPROM device address */
#define ATMEL_EEPROM_ADDRESS_LEN   2

#define WM_DEVICE_ADDR          0x1A  /* Wolfson WM8974 I2C device address */
#define WM_DEVICE_ADDRESS_LEN      1

#define	DPRINTF(chip, a)	\
        if ( (chip==ATMEL_DEVICE_ADDR) || (chip==WM_DEVICE_ADDR) ) \
            printf a;

#if (0)
#define  DEBUG
#endif

#ifdef  DEBUG
#define	DPRINT(a)	printf a;
#else   /* !DEBUG */
#define	DPRINT(a)
#endif  /* DEBUG */


#if (CONFIG_COMMANDS & CFG_CMD_I2C)

#define I2C_BASE_ADDR         (KS8692_IO_BASE + KS8692_I2C_BASE ) 

ks_i2c_t *const ksI2c = (ks_i2c_t *)I2C_BASE_ADDR;

int i2cInitialized = 0;

/* function protocol */

static __inline__ int  WaitForSCL ( uchar );
static __inline__ int  WaitForXfer (void);
static __inline__ int  i2c_start (uchar chip );
static __inline__ int  CheckTransactionByte ( uchar, char, ulong );
static __inline__ void set_i2c_address ( unsigned char, unsigned char, char );
static __inline__ void set_i2c_data ( unsigned char, unsigned char *, int );
static __inline__ void get_i2c_data ( unsigned char *, int );
static __inline__ void i2c_write_addsOffset ( unsigned long * );
static __inline__ int  i2c_issue_addsOffset ( unsigned char, unsigned short );
static int i2c_write_data ( unsigned char, unsigned char *, unsigned char );
static int i2c_random_read_data ( uchar, ushort, uchar *, uchar );
static int i2c_send ( uchar, uint, uchar *, uchar );
static int i2c_receive ( uchar, uint, uchar *, uchar );

void i2c_init (int, int );
int  i2c_probe ( uchar );
int  i2c_read ( uchar, uint, int, uchar *, int );
int  i2c_write ( uchar, uint, int, uchar *, int );
void i2c_isr ( void );
void i2c_init_interrupt ( void );



/* 
 * WaitForSCL: Wait for SCL, SDA ready (I2C_SR). 
 */
static __inline__ int WaitForSCL (uchar chip )
{
    int i;
    unsigned long status;

    i = I2C_TIMEOUT * 10000;
    status = ksI2c->i2c_sr;
    while ( (i-- > 0) && !(status & I2C_SCL) && !(status & I2C_SDA) ) 
    {
        udelay (100);
        status = ksI2c->i2c_sr;
    }

    DPRINT(("%s: i2c_sr=%08X:%08x \n", __FUNCTION__, &ksI2c->i2c_sr, status ));
    if (!(status & I2C_SCL) || !(status & I2C_SDA) )
    {
        DPRINTF( chip,
               (" %s (chip 0x%02x): ERROR, i2c_sr=%08X:%08x, i=%d \n", __FUNCTION__, chip, &ksI2c->i2c_sr, status, i )); 
    }

    return ( (status & I2C_SCL) && (status & I2C_SDA) )? I2C_OK : I2C_NOK_TOUT ;
}

/* 
 * WaitForXfer: Wait for transaction completed (I2C_SCR). 
 */
static __inline__ int WaitForXfer (void)
{
    int i;
    unsigned long status;

    i = I2C_TIMEOUT * 10000;
    status = ksI2c->i2c_scr;
    while ( (i-- > 0) && (status & I2C_START) ) 
    {
        udelay (100);
        status = ksI2c->i2c_scr;
    }

    DPRINT(("%s: i2c_scr=%08X:%08x \n", __FUNCTION__, &ksI2c->i2c_scr, status ));
    return  (!(status & I2C_START)) ? I2C_OK : I2C_NOK_TOUT;

}

/* 
 * i2c_start: Start I2C tansaction (I2C_SCR). 
 */
static __inline__ int i2c_start (uchar chip )
{
    /* Wait for previous transaction done */
    if ( WaitForXfer () != I2C_OK )
    {
        DPRINTF( chip,
               ("%s (chip 0x%02x): ERROR, write transaction is not completed.\n", __FUNCTION__, chip));
        return I2C_NOK;
    }

    /* Wait for SCL, SDA ready */
    if ( WaitForSCL (chip) != I2C_OK )
    {
        DPRINTF( chip,
               ("%s (chip 0x%02x): ERROR, SCL, or SDA not ready.\n", __FUNCTION__, chip));
        return I2C_NOK;
    }

    /* Start transaction */
    ksI2c->i2c_scr = I2C_START;
    DPRINT(("%s: i2c_scr=%08X:%08x \n", __FUNCTION__, &ksI2c->i2c_scr, I2C_START ));

    /* Wait for transaction done */
    if ( WaitForXfer () != I2C_OK )
    {
        DPRINTF( chip,
               ("%s (chip 0x%02x): ERROR, write transaction is not completed.\n", __FUNCTION__, chip));
        return I2C_NOK;
    }

    return I2C_OK;
}

/* 
 * CheckTransactionByte: Check transaction byte done from I2C (I2C_SR). 
 */
static __inline__ int CheckTransactionByte 
(
    uchar  chip,
    char   rd_flag,
    ulong  expectLen
)
{
    unsigned long  status;
    int length=0;
    int i;


    i = I2C_TIMEOUT * 10000;
    status = ksI2c->i2c_sr ;
    DPRINT(("%s: i2c_sr=%08X:%08x, expectLen=%x \n", __FUNCTION__, &ksI2c->i2c_sr, status, expectLen ));
    while ( (i-- > 0) && !(status & I2C_DATA_DONE) ) 
    {
        udelay (100);
        status = ksI2c->i2c_sr ;
        DPRINT(("%s: i2c_sr=%08X:%08x, expectLen=%x \n", __FUNCTION__, &ksI2c->i2c_sr, status, expectLen ));
    }
   
    if ( !(status & I2C_DATA_DONE) )
    {
        DPRINTF( chip, 
                ("%s (chip 0x%02x): ERROR, %s transaction is not completed due to error %08x (i=%d).\n",  
                 __FUNCTION__, chip, rd_flag ? "read" : "write", status, i  ));
        return (I2C_NOK);
    }

    if ( rd_flag )
       /* number bytes available to be read out from I2C data reg. */
       length = (status & I2C_BYTE_READ_MASK) >> 8;   
    else
       /* number bytes that were written out from I2C data reg. */
       length = (status & I2C_BYTE_WRITE_MASK) >> 12;   

    if ( length > I2C_MAX_DATA_LEN )
    {
        DPRINTF( chip,
               ("%s (chip 0x%02x): ??? why %s transaction length %d is greater than %d.\n",  
                 __FUNCTION__, chip, rd_flag ? "read" : "write", length, I2C_MAX_DATA_LEN ));
    }

    return ( (length == expectLen) ? I2C_OK : I2C_NOK );
}

/* 
 * set_i2c_address: Program Address register (I2C_AR)
 */
static __inline__ void set_i2c_address
(
    unsigned char  chip,
    unsigned char  data_len,
    char           rd_flag
)
{
    unsigned long  regData=0;


#if (0)
    DPRINT(("%s: chip=%04x, data_len=%02x, rd_flag=%d \n", __FUNCTION__, chip, data_len, rd_flag ));
#endif

    regData = (chip << 1) & I2C_ADDR_MASK;

    if (rd_flag) 
       regData |= ( ((data_len << 16) & I2C_BYTE_TO_READ_MASK) | I2C_READ );
    else
       regData |= ( ((data_len << 20) & I2C_BYTE_TO_WRITE_MASK) );

    /* write to I2C_AR */
    ksI2c->i2c_ar = regData ;
    DPRINT(("%s: i2c_ar=%08X:%08x \n", __FUNCTION__, &ksI2c->i2c_ar, regData ));

}

/* 
 * set_i2c_data: Write data to Write Data Address register (I2C_WDR)
 *               Starting from I2C_WDR, byte_2.
 */
static __inline__ void set_i2c_data
(
    unsigned char  chip,
    unsigned char  *writeBuf, 
    int            data_len
)
{
    unsigned long   regData=0;
    unsigned short  len, i;


#if (0)
    {
    unsigned long  *dwData = (unsigned long *)writeBuf; 
    DPRINT(("%s: writeBuf=%08X: %08x, data_len=%02x \n", __FUNCTION__, writeBuf, *dwData, data_len ));
    }
#endif

    if (writeBuf == 0 || data_len == 0) 
    {
        /* Don't support data transfer of no length or to address 0 */
        printf ("%s: ERROR, bad call. \n", __FUNCTION__);
        return;
    }

    if (chip == WM_DEVICE_ADDR)
    {
        unsigned short *wData = (unsigned short *)writeBuf; 

        /* for WM8974, only one byte of data (b8-b1) per transaction, */
        *wData >>= 1;
        regData = ksI2c->i2c_wdr0;
        regData |= *wData << 8;
        ksI2c->i2c_wdr0 = regData;
        DPRINT(("%s: i2c_wdr0=%08X:%08x \n", __FUNCTION__, &ksI2c->i2c_wdr0, regData ));
    }
    else
    {
        /* Write to I2C_WDR0, start from 3rd byte */
        regData = ksI2c->i2c_wdr0;
        len = (data_len > 2 ) ? 2 : data_len;
        for (i=0; i<len; i++)
        {
           regData |= ( *writeBuf << (i*8) ) << 16;
           writeBuf++;
        }
        ksI2c->i2c_wdr0 = regData;
        DPRINT(("%s: i2c_wdr0=%08X:%08x \n", __FUNCTION__, &ksI2c->i2c_wdr0, regData ));

        /* Write to I2C_WDR1 */
        data_len -= 4;
        if ( data_len > 0 )
        {
           regData = 0;
           len = (data_len > 4) ? 4 : data_len;
           for (i=0; i<len; i++)
           {
              regData |= *writeBuf << (i*8);
              writeBuf++;
           }
           ksI2c->i2c_wdr1 = regData;
           DPRINT(("%s: i2c_wdr1=%08X:%08x \n", __FUNCTION__, &ksI2c->i2c_wdr1, regData ));
        }
    }
}

/* 
 * get_i2c_data: Read Data from Read Data register (I2C_RDR)
 */
static __inline__ void get_i2c_data
(
    unsigned char  *readBuf, 
    int             data_len
)
{
    int             i;
    unsigned long   regData;
    unsigned char  *bData; 


#if (0)
    DPRINT(("%s: readBuf=%08x, data_len=%02x \n", __FUNCTION__, 
             readBuf, data_len ));
#endif

    if (readBuf == 0 || data_len == 0) 
        /* Don't support data transfer of no length or to address 0 */
        printf ("%s: ERROR, bad call. \n", __FUNCTION__);
       
    /* Read from I2C_RDR0 */
    regData = ksI2c->i2c_rdr0;
    DPRINT(("%s: i2c_rdr0=%08X:%08x \n", __FUNCTION__, &ksI2c->i2c_rdr0, regData ));

    bData = (unsigned char *)&regData;
    for ( i=0; ((i<4) && (i < data_len)); i++)
        *readBuf++ = *bData++;

    /* Read from I2C_RDR1 */
    data_len -= i;
    if ( data_len > 0 )
    {
       regData = ksI2c->i2c_rdr1;
       DPRINT(("%s: i2c_rdr1=%08x \n", __FUNCTION__, regData ));
       bData = (unsigned char *)&regData;
       for ( i=0; ((i<4) && (i < data_len)); i++)
          *readBuf++ = *bData++;
    }
}


/*
 * i2c_write_addsOffset()
 *
 * Write two byte of addresses offset to I2C_WDR0 (byte_0, byte_1).
 * write data will start from byte_2 of I2C_WDR0 after this call.
 */
static __inline__ void i2c_write_addsOffset 
(   
    unsigned long *offset            /* EEPROM device offset */ 
)
{

#if (0)
    DPRINT(("%s: offset=%08x \n", __FUNCTION__, *offset ));
#endif
    ksI2c->i2c_wdr0 = *offset;
    DPRINT(("%s: i2c_wdr0=%08X:%08x \n", __FUNCTION__, &ksI2c->i2c_wdr0, ksI2c->i2c_wdr0 ));
}

/*
 * i2c_issue_addsOffset()
 *
 * Issue two byte of addresses offset following the device
 * address byte to I2C device.
 */
static __inline__ int i2c_issue_addsOffset 
(   
    unsigned char  chip,             /* EEPROM device address eg. 0x50 */
    unsigned short offset            /* EEPROM device offset */ 
)
{
    unsigned long regData=0;


#if (1)
    DPRINT(("%s: chip=%04x, offset=%04x \n", __FUNCTION__, chip, offset ));
#endif

    /* Program device address and to I2C_AR */
    set_i2c_address ( chip, ATMEL_EEPROM_ADDRESS_LEN, (char)I2C_WRITE_FLAG);

    /* Program device offset to I2C_WDR */
    regData = (ulong)offset;
    i2c_write_addsOffset ( &regData );

    /* Start Write transaction */
    if ( i2c_start (chip) != I2C_OK )
    {
        DPRINTF( chip,
               ("%s (chip 0x%02x): ERROR, Start operation fail.\n", __FUNCTION__, chip));
        return I2C_NOK;
    }
    
    /* Get read transaction byte done from I2C. */
#if (0)
    return ( CheckTransactionByte ( chip, I2C_WRITE_FLAG, ATMEL_EEPROM_ADDRESS_LEN ) );
#endif
#if (1)
    CheckTransactionByte ( chip, I2C_WRITE_FLAG, ATMEL_EEPROM_ADDRESS_LEN );
    return ( I2C_OK );
#endif

}


/*
 * i2c_write_data()
 *
 * Write data to I2C device to chip offset 'offset', from 
 * data buffer 'data' by length 'data_len' through KS8692 I2C controller 
 */
static int i2c_write_data 
(   
    unsigned char  chip,
    unsigned char  data[], 
    unsigned char  data_len
)
{
    unsigned char  expectLen;

#if (1)
    {
    unsigned char *bData= (unsigned char *)data; 
    DPRINT(("%s: chip=%04x, data=%08X:%02x, data_len=%02x \n", __FUNCTION__, 
             chip, data, *bData, data_len ));
    }
#endif

    if (data == 0 || data_len == 0) 
    {
        /* Don't support data transfer of no length or to address 0 */
        printf ("%s: bad call\n", __FUNCTION__);
        return I2C_NOK;
    }

    /* Program number of byte (plus ATMEL_EEPROM_ADDRESS_LEN) in I2C_AR */
    if (chip == ATMEL_DEVICE_ADDR)
        expectLen = data_len + ATMEL_EEPROM_ADDRESS_LEN;
    else
        expectLen = data_len + WM_DEVICE_ADDRESS_LEN;
    set_i2c_address ( chip, expectLen, (char)I2C_WRITE_FLAG);


    /* Write data to I2C_WDR0 - I2C_WDR1 */
    set_i2c_data ( chip, data, data_len);

    /* Start Write transaction */
    if ( i2c_start (chip) != I2C_OK )
    {
        DPRINTF( chip,
               ("%s (chip 0x%02x): ERROR, Start operation fail.\n", __FUNCTION__, chip));
        return I2C_NOK;
    }
    udelay (3000); /* wait 2ms */

    /* Check transaction byte done from I2C. */
    if ( CheckTransactionByte ( chip, I2C_WRITE_FLAG, expectLen ) != I2C_OK )
        return I2C_NOK;

    return (data_len);

}

/*
 * i2c_random_read_data()
 *
 * Random read data from I2C device chip 'addr', to  
 * data buffer 'data' by length 'data_len' through KS8692 I2C controller.
 */
static int i2c_random_read_data 
(   
    uchar  chip,
    ushort addr, 
    uchar  dataBuf[], 
    uchar  data_len
)
{

#if (1)
    DPRINT(("%s: addr=%04x, dataBuf=%0x8, data_len=%02x \n", __FUNCTION__, 
             addr, dataBuf, data_len ));
#endif

    if (dataBuf == 0 || data_len == 0) 
    {
        /* Don't support data transfer of no length or to address 0 */
        DPRINTF( chip,
                ("%s (chip 0x%02x) : ERROR, bad call. \n", __FUNCTION__, chip));
        return I2C_NOK;
    }

    /*
     * Don't issure Stop bit in the 1st operation when write 2-byte device 
     * address offset (for 2nd read operation's Start bit, it, then become
     * Restart bit).
     */

     ksI2c->i2c_gcr &= ~I2C_STOP_BIT;
     DPRINT(("%s: i2c_gcr=%08X:%08x \n", __FUNCTION__, &ksI2c->i2c_gcr, ksI2c->i2c_gcr ));

    /*
     * Transfer I2C device address follow by 2-byte device address offset 
     * to I2C bus.
     */

    if ( i2c_issue_addsOffset ( chip, addr ) != I2C_OK )
        return I2C_NOK;

    /*
     * Now, issure Stop bit for 2nd read operation.
     */

     ksI2c->i2c_gcr |= I2C_STOP_BIT;
     DPRINT(("%s: i2c_gcr=%08X:%08x \n", __FUNCTION__, &ksI2c->i2c_gcr, ksI2c->i2c_gcr ));

    /*
     * Send out read from current address read I2C bus.
     */

    /* Program number of byte to read in I2C_AR */
    set_i2c_address ( chip, data_len, (char)I2C_READ_FLAG );

    /* Start Read transaction */
    if ( i2c_start (chip) != I2C_OK )
    {
        DPRINTF( chip,
               ("%s (chip 0x%02x): ERROR, Start operation fail.\n", __FUNCTION__, chip));
        return I2C_NOK;
    }

    /* Check transaction byte done from I2C. */
    if ( CheckTransactionByte ( chip, I2C_READ_FLAG, data_len ) != I2C_OK ) 
        return (I2C_NOK);

    /* Read Data from I2C_RDR0 - I2C_RDR1 */
    get_i2c_data ( dataBuf, data_len);  

    return (data_len);

}


/*
 * i2c_send()
 *
 * Write data to I2C device 'chip' base address, offset 'addr', from 
 * data buffer 'data' by length 'data_len' through KS8692 I2C controller 
 * 'addr_len' is length of address bit (0:0bit, 1:8bit, 2:16bit).
 */
static int i2c_send 
(   
    uchar  chip, 
    uint   addr, 
    uchar  data[], 
    uchar  data_len
)
{
    int  retLength=0;
    ulong  wmOffset=0;
 

    DPRINT(("%s: chip=%02x, addr=0x%08x, data=%08x, data_len=%02x \n", __FUNCTION__, 
             chip, addr, data, data_len ));

    if (data == 0 || data_len == 0) 
    {
        /* Don't support data transfer of no length or to address 0 */
        printf ("%s: bad call\n", __FUNCTION__);
        return I2C_NOK;
    }

    /* Write i2c chip address and offset to i2c bus */ 
    if (chip == ATMEL_DEVICE_ADDR)
    {
        i2c_write_addsOffset ( (ulong *)&addr );
    }
    else
    {
        /* 1st byte contain 7bit of offset + data bit0 */
        wmOffset = ((uchar)addr << 1) | ( data[0] & 0x01);
        i2c_write_addsOffset ( (ulong *)&wmOffset );
    }

    /* Write data to i2c chip  */ 
    if ( (retLength = i2c_write_data ( chip, data, data_len )) != data_len ) 
    {
        printf ("%s: ERROR, write data to i2c fail, written length %d is not equal request write length %d. \n",  
                __FUNCTION__, data_len, retLength );
        return I2C_NOK; 
    }

    return (retLength);
}


/*
 * i2c_receive()
 *
 * Read data from I2C device 'chip' address, offset 'addr', to  
 * data buffer 'data' by length 'data_len' through KS8692 I2C controller.
 * 'addr_len' is length of address bit (0:0bit, 1:8bit, 2:16bit).
 */
static int i2c_receive 
(   
    uchar  chip, 
    uint   addr, 
    uchar  dataIn[], 
    uchar  data_len
)
{
    int  retLength=0;
    int  i;



    DPRINT(("%s: chip=%02x, addr=0x%08x, dataIn=%08x, data_len=%02x \n", __FUNCTION__, 
             chip, addr, dataIn, data_len ));

    if (dataIn == 0 || data_len == 0) 
    {
        /* Don't support data transfer of no length or to address 0 */
        DPRINTF( chip,
               ("%s (chip 0x%02x) : ERROR, bad call. \n", __FUNCTION__, chip));
        return I2C_NOK;
    }

    for ( i=0; i<data_len; i++ )
    {
       /* Random Read i2c chip data from offset address */ 
       if ( (retLength = i2c_random_read_data ( chip, (ushort)(addr+i), (dataIn+i), 1 )) != 1 ) 
       {
            DPRINTF( chip,
                 ("%s (chip 0x%02x) : ERROR, read data from i2c fail (retLength=%d). \n", 
                  __FUNCTION__, chip, retLength ));
            return (I2C_NOK); 
       }
    }
    retLength = data_len;

    return (retLength);
}

/*----------------------Extern Function --------------------------------*/

/* 
 * Init I2C Global Control register and enable it.
 * Set 400KHz bit rate for Atmel AT27C128, our system clock is 166MHz.
 * Bit Period = 125MHz/(400KHz*8) = 39 = 0x27
 * Bit Period = 166MHz/(400KHz*8) = 51 = 0x33  (default)
 * Bit Period = 200MHz/(400KHz*8) = 62 = 0x3E
 */
void i2c_init 
(
    int speed, 
    int slaveadd
)
{
    unsigned long  regData=0;


    /* only execute the following code once. */
    if (i2cInitialized == 0)
    {
        /* Set Timout value (using default value for now) */
        regData |= ( (( 0x10 << 16) & I2C_TIMEOUT_MASK) );
        DPRINT(("%s: regData=%08x \n", __FUNCTION__, regData ));

        /* Set Bit period by 'speed' (using default value for now) */
        regData |= ( ((0x33 << 1) & I2C_BIT_PERIOD_MASK) );
        DPRINT(("%s: regData=%08x \n", __FUNCTION__, regData ));

        /* Enable I2C controller */
        regData |= ( I2C_STOP_BIT | I2C_ENABLE );
        ksI2c->i2c_gcr = regData;

        DPRINT(("%s: i2c_gcr=%08X:%08x \n", __FUNCTION__, &ksI2c->i2c_gcr, regData ));

#ifdef CONFIG_USE_IRQ
        /* Initialize I2C interrupt */
        i2c_init_interrupt ();
#endif

        i2cInitialized ++;
    }
}


int i2c_probe (uchar chip)
{
    uchar buf[4]; 


    DPRINT(("%s: chip=%02x \n", __FUNCTION__, chip));

    if (i2cInitialized == 0)
        i2c_init(CFG_I2C_SPEED, CFG_I2C_SLAVE);

    memset(&buf[0], 0, sizeof(buf) );

    /*
     * What is needed is to send the chip address and verify that the
     * address was <ACK>ed (i.e. there was a chip at that address which
     * drove the data line low).
     */
    if ( i2c_receive ( chip, 0, buf, 4 ) == I2C_NOK ) 
    {
        return (I2C_NOK); 
    }

    return (I2C_OK); /* device exists - read succeeded */
}

/*
 *     imd 50 0 10      display 16 bytes starting at 0x000
 *                      On the bus: <S> A0 00 <E> <S> A1 <rd> ... <rd>
 *     imd 50 100 10    display 16 bytes starting at 0x100
 *                      On the bus: <S> A2 00 <E> <S> A3 <rd> ... <rd>
 *     imd 50 210 10    display 16 bytes starting at 0x210
 *                      On the bus: <S> A4 10 <E> <S> A5 <rd> ... <rd>
 *
 *     eg. to display AT24C128 EEPROM I2C device (Chip address is 0x50)
 *     imd 50 210 4
 *     chip=0x50, addr=0x210, alen=0x01, len=0x04
 */
int i2c_read 
(
    uchar chip, 
    uint addr, 
    int alen, 
    uchar * buffer, 
    int len
)
{
    int    readLen;
    int    actuallyReadLen=0;


    DPRINT(("%s: chip=%02x, addr=%08x, alen=%02x, buffer=%08x, len=%02x \n", __FUNCTION__, 
             chip, addr, alen, buffer, len ));

    if (i2cInitialized == 0)
        i2c_init(CFG_I2C_SPEED, CFG_I2C_SLAVE);

    if (alen > 4) {
        printf ("I2C read: addr len %d not supported\n", alen);
        return 1;
    }

    while ( len > 0 )
    {
        readLen = I2C_MAX_DATA_LEN < len ? I2C_MAX_DATA_LEN : len;
        if ( (actuallyReadLen = 
              i2c_receive ( chip, addr, buffer, readLen )) <= I2C_OK ) 
        {
            printf ("I2c read: ERROR, failed %d\n", actuallyReadLen);
            return 1;
        }
        addr += actuallyReadLen;
        buffer += actuallyReadLen;
        len -= actuallyReadLen;
    }

    return 0;
}

/*
 *     imd 50 100       Modify value on chip address at 0x50, offset 0x100
 *
 *     eg. to modify AT24C128 EEPROM I2C device (Chip address is 0x50)
 *     inm 50 100
 *     chip=0x50, addr=0x100, alen=0x01, len=0x01 
 */
int i2c_write 
(
    uchar chip, 
    uint addr, 
    int alen, 
    uchar * buffer, 
    int len
)
{
    char   writeLen;
    char   actuallyWrittenLen=0;


    DPRINT(("%s: chip=%02x, addr=%08x, alen=%02x, buffer=%08x, len=%02x \n", __FUNCTION__, 
             chip, addr, alen, buffer, len ));

    if (i2cInitialized == 0)
        i2c_init(CFG_I2C_SPEED, CFG_I2C_SLAVE);

    if (alen > 4) {
        printf ("I2C write: addr len %d not supported\n", alen);
        return 1;
    }

    while ( len > 0 )
    {
        writeLen = (I2C_MAX_DATA_LEN-ATMEL_EEPROM_ADDRESS_LEN) < len ? (I2C_MAX_DATA_LEN-ATMEL_EEPROM_ADDRESS_LEN) : len;
        if ( (actuallyWrittenLen = i2c_send ( chip, addr, buffer, writeLen )) <= I2C_OK )
        {
            printf ("i2c_write: ERROR, failed %d\n", actuallyWrittenLen);
            return 1;
        }
        addr += actuallyWrittenLen;
        buffer += actuallyWrittenLen;
        len -= actuallyWrittenLen;
    }

    return 0;
}

#ifdef CONFIG_USE_IRQ

void i2c_disable_interrupt(void)
{
    unsigned long intReg;


    /* enable I2C interrupt bit */
    intReg = ks8692_read_dword(KS8692_INT_ENABLE2);
    ks8692_write_dword(KS8692_INT_ENABLE2, (intReg & ~INT_I2C )); 

    printf("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_ENABLE2, ks8692_read_dword(KS8692_INT_ENABLE2) );
}

/*
 * i2c_isr()
 *
 * ISR to test I2C interrupt bits. 
 * Do nothing, but just clear I2C interrupt. 
 */
void i2c_isr(void)
{
    unsigned long intStatus;
    unsigned long status;

    intStatus = ks8692_read_dword(KS8692_INT_STATUS2);
    status = ksI2c->i2c_sr ;

#if (1)
    printf("%s: intStatus=0x%08X:%08x \n", __FUNCTION__, KS8692_INT_STATUS2, intStatus);
    printf("%s: i2c_sr=%08X:%08x \n", __FUNCTION__, &ksI2c->i2c_sr, status );
#endif

    /* Do nothing, but just clear interrupt */
    ks8692_write_dword(KS8692_INT_STATUS2, (intStatus & INT_I2C)); 
}


/*
 * i2c_init_interrupt()
 *
 * Hook up ISR and enable I2C interrupt to test I2C interrupt bits. 
 * irq 40 is I2C interrupt. 
 */
void i2c_init_interrupt ()
{    
    unsigned long intReg;

    /* initialize irq software vector table */
    interrupt_init();

    /* Clear I2C interrupt Tx/Rx bits */
    intReg = ks8692_read_dword(KS8692_INT_STATUS2);
    ks8692_write_dword(KS8692_INT_STATUS2, (intReg & INT_I2C )); 

    /* install i2c_isr() */
    irq_install_handler((8+32), (interrupt_handler_t *)i2c_isr, NULL);


    /* Configure I2C interrupt for FIQ mode */
    intReg = ks8692_read_dword(KS8692_INT_CONTL2);
    // ks8692_write_dword(KS8692_INT_CONTL2, (intReg | INT_I2C )); 

    /* enable I2C interrupt bit */
    intReg = ks8692_read_dword(KS8692_INT_ENABLE2);
    ks8692_write_dword(KS8692_INT_ENABLE2, (intReg | INT_I2C )); 

#if (1)
    printf("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_STATUS2, ks8692_read_dword(KS8692_INT_STATUS2) );
    printf("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_ENABLE2, ks8692_read_dword(KS8692_INT_ENABLE2) );
    printf("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_CONTL2,  ks8692_read_dword(KS8692_INT_CONTL2) );
#endif

}
#endif /* #ifdef CONFIG_USE_IRQ */

#endif /* #if (CONFIG_COMMANDS & CFG_CMD_I2C) */ 
