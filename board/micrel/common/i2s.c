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
 * I2S Read/Write utilities Micrel KS8692 (ARM922T code) I2S Controller.
 * IS2 controller is interface with WM8974 Codec I2S device.
 */

#include <common.h>

#if (MICREL_CMD_I2S)

#include <linux/ctype.h>
#include <asm/arch/platform.h>


/* Data Chunk */
typedef volatile struct {
    ulong   chunkId;
    ulong   chunkSize;
    char    waveformData;
} DATA_CHUNK;

#define  DATA_ID                0x61746164    /* chunkID 'data' for Data Chunk */


/* Format ('fmt ') Chunk */
typedef volatile struct {
    ulong   chunkId;
    ulong   chunkSize;
    short   wCompressionCode;
    ushort  wChannels;
    ulong   dwSamplesPerSec;
    ulong   dwAvgBytesPerSec;
    ushort  wBlockalign;
    ushort  wBitsPerSample;
} FORMAT_CHUNK;

#define  FMT_ID                 0x20746D66    /* chunkID 'fmt ' for Format Chunk */
#define  UNCOMPRESSED_DATA      0x0001        /* uncompressed Wave data */
#define  STEREO_SIGNAL_CHANNEL  0x0002        /* if number of channel is 2, it is stereo signal */
#define  MONO_SIGNAL_CHANNEL    0x0001        /* if number of channel is 1, it is mono signal */
#define  BITS8_PER_SAMPLE       0x0008        /* 8  bit per sample */
#define  BITS16_PER_SAMPLE      0x0010        /* 16 bit per sample */
#define  BITS18_PER_SAMPLE      0x0012        /* 18 bit per sample */
#define  BITS20_PER_SAMPLE      0x0014        /* 20 bit per sample */
#define  BITS24_PER_SAMPLE      0x0018        /* 24 bit per sample */


/* Wave file header - RIFF type Chunk */
typedef volatile struct {
    ulong          chunkId;
    ulong          chunkSize;
    ulong          riffType;
    FORMAT_CHUNK  *pFmt;
} RIFF_CHUNK;

#define  RIFF_ID                0x46464952    /* chunkID 'RIFF' for RIFF Chunk */
#define  WAVE_FORM              0x45564157    /* Waveform type 'WAVE' */


/* KS8692 I2C registers mapping structure */
typedef volatile struct ks_i2s_t {
    ulong   i2s_cr;     /* I2S control reg */
    ulong   i2s_tsr ;   /* I2S Tx status reg */
    ulong   i2s_rsr;    /* I2S Rx status reg */
    ulong   i2s_dr ;    /* I2S data reg */
}ks_i2s_t;


#define I2S_OK		     0
#define I2S_NOK		    -1

#define I2S_WRITE_FLAG   0
#define I2S_READ_FLAG    1

#define I2S_TIMEOUT      1		      /* 1 second */

#if (1)
#define  DEBUG
#endif

#ifdef  DEBUG
#    define I2S_DBG_OFF           0x000000
#    define I2S_DBG_READ          0x000001
#    define I2S_DBG_WRITE         0x000002
#    define I2S_DBG_INIT          0x000004
#    define I2S_DBG_INT           0x000008
#    define I2S_DBG_STEP_READ     0x000010
#    define I2S_DBG_STEP_WRITE    0x000020
#    define I2S_DBG_ERROR         0x800000
#    define I2S_DBG_ALL           0xffffff

static int ksDebug = ( I2S_DBG_ERROR | I2S_DBG_INT );
#define	DPRINT(FLG, a) \
       if (ksDebug & FLG) printf a;

#else   /* !DEBUG */
#define	DPRINT(FLG, a) 
#endif  /* DEBUG */


#define I2S_BASE_ADDR         (KS8692_IO_BASE + KS8692_I2S_BASE ) 

ks_i2s_t *const ksI2s = (ks_i2s_t *)I2S_BASE_ADDR;

int i2sInitialized = 0;

/* function protocol */

static __inline__ int  GetTxCounter ( int * );
static __inline__ void set_i2s_data ( ushort, char *, int, ulong );
static __inline__ void get_i2s_data ( ushort, char *, int, ulong );

static int i2s_write_data ( uchar *, ulong, ushort, ulong );
static int i2s_read_data ( uchar *, int, ushort, ulong );

void i2s_init ( char, ulong, ushort );
void i2s_disable ( char );
int  i2s_read  ( uchar *, int, ushort , ushort, ulong );
int  i2s_write ( uchar *, int, ushort, ushort, ulong, ulong, ulong );
int  i2s_read_file ( uchar *, ulong, ulong );
void i2s_init_txInterrupt ( void );
void i2s_init_rxInterrupt ( void );


/* 
 * wavefmt_info: display waveform file format. 
 */

static void wavefmt_info 
(
    FORMAT_CHUNK *fmtChunk
)
{
    printf ("\n" );
    printf ("AudioFormat  : %d \n", fmtChunk->wCompressionCode );
    printf ("NumChannels  : %d \n", fmtChunk->wChannels );
    printf ("BlockAlign   : %d \n", fmtChunk->wBlockalign );
    printf ("BitsPerSample: %d \n", fmtChunk->wBitsPerSample );
    printf ("\n" );
}

/* 
 * GetTxCounter: Get transmit buffer valid counter from I2C (I2S_TSR). 
 */
static __inline__ int GetTxCounter 
(
    int   *txValidCounter
)
{
    ulong  status;
    int    validCounter, i=10;


    status = ksI2s->i2s_tsr ;
    DPRINT(I2S_DBG_WRITE,
           ("  %s: i2s_tsr=%08X:%08x \n", __FUNCTION__, &ksI2s->i2s_tsr, status ));
   
    if ( status & I2S_TX_OVERRUN)
    {
        DPRINT(I2S_DBG_ERROR,
              ("%s: ERROR, transmit buffer overrun,  i2s_tsr=%08x.\n",  
                 __FUNCTION__, status  ));
        return (I2S_NOK);
    }

    /* read the tx valid counter */
    *txValidCounter = 0;
    while ( (i-- > 0) && ((status & I2S_TX_VALID_MASK) <= 0) )
    {
        udelay (1);
        status = ksI2s->i2s_tsr ;
    }

    validCounter = (status & I2S_TX_VALID_MASK);   
    if ( validCounter <= 0 )
    {
        DPRINT(I2S_DBG_ERROR,
              ("%s: transmit valid counter is 0,  i2s_tsr=%08x.\n",  
                 __FUNCTION__, status  ));
        return (I2S_NOK);
    }
    *txValidCounter = validCounter;

    return ( I2S_OK );
}

/* 
 * CheckRxCounter: Get receive buffer counter from I2C (I2S_RSR). 
 */
static __inline__ int CheckRxCounter 
(
    int   *rxCounter
)
{
    ulong  status;
    int    validCounter, i=10;


    status = ksI2s->i2s_rsr ;
    DPRINT(I2S_DBG_READ,
           ("  %s: i2s_rsr=%08X:%08x \n", __FUNCTION__, &ksI2s->i2s_rsr, status ));
   
    if ( status & I2S_RX_UNDERRUN)
    {
        DPRINT(I2S_DBG_ERROR,
              ("%s: ERROR, receive buffer underrun,  i2s_rsr=%08x.\n",  
                 __FUNCTION__, status  ));
        return (I2S_NOK);
    }

    /* read the rx counter */
    *rxCounter = 0;
    while ( (i-- > 0) && ((status & I2S_RX_VALID_MASK) <= 0) )
    {
        udelay (1);
        status = ksI2s->i2s_rsr ;
    }

    validCounter = (status & I2S_RX_VALID_MASK);   
    if ( validCounter <= 0 )
    {
        DPRINT(I2S_DBG_ERROR,
              ("%s: receive counter is 0,  i2s_rsr=%08x.\n",  
                 __FUNCTION__, status  ));
        return (I2S_NOK);
    }

    *rxCounter = validCounter;

    return ( I2S_OK );
}

/* 
 * set_i2s_data: Write data to Write Data Address register (I2C_WDR)
 *               Starting from I2C_WDR, byte_2.
 */
static __inline__ void set_i2s_data
(
    ushort wChannels,
    char  *writeBuf, 
    int    numSample,
    ulong  byteAlign
)
{
    int     i;
    ulong   regData;
    ulong   offset=4-byteAlign;



    DPRINT(I2S_DBG_WRITE,
           ("  %s: writeBuf=%08x, numSample=%x, byteAlign=%x \n", 
               __FUNCTION__, writeBuf, numSample, byteAlign ));

    while (numSample > 0)
    {
        /* write to left channel */
        regData = 0;
        for (i=0; i<byteAlign; i++)
            regData |= ( *writeBuf++ << (i*8) ) << (offset*8);
        ksI2s->i2s_dr = regData; 
        DPRINT(I2S_DBG_STEP_WRITE,
               ("  %s: i2s_dr=%08X:%08x \n", __FUNCTION__, &ksI2s->i2s_dr, regData )); 
        numSample--;
          
        /* write to right channel */
        regData = 0;                    /* fill '0' to right channel if it is mono signal */ 
        if (wChannels == STEREO_SIGNAL_CHANNEL)
        {
            numSample --;
            for (i=0; i<byteAlign; i++)
                regData |= ( *writeBuf++ << (i*8) ) << (offset*8);
        }
        ksI2s->i2s_dr = regData; 
        DPRINT(I2S_DBG_STEP_WRITE,
              ("  %s: i2s_dr=%08X:%08x \n", __FUNCTION__, &ksI2s->i2s_dr, regData )); 
    }
}


/* 
 * get_i2s_data: Read Data from Read Data register (I2C_RDR)
 */
static __inline__ void get_i2s_data
(
    ushort wChannels,
    char  *readBuf, 
    int    numSample,
    ulong  byteAlign
)
{
    int    i;
    ulong  regData;
    ulong  offset=4-byteAlign;



    DPRINT(I2S_DBG_READ,
           ("  %s: readBuf=%08x, numSample=%02x, byteAlign=%x\n", 
               __FUNCTION__, readBuf, numSample, byteAlign ));
       
    while (numSample > 0)
    {
        /* read from left channel */
        regData = ksI2s->i2s_dr;
        DPRINT(I2S_DBG_STEP_READ,
              ("  %s: i2s_dr=%08X:%08x \n", __FUNCTION__, &ksI2s->i2s_dr, regData ));
        for (i=0; i<byteAlign; i++)
            *readBuf++ =  regData >> ((offset+i)*8);
        numSample--;
            
        /* read from right channel */
        regData = ksI2s->i2s_dr;        /* dummy read right channel if it is mono signal */ 
        DPRINT(I2S_DBG_STEP_READ,
              ("  %s: i2s_dr=%08X:%08x \n", __FUNCTION__, &ksI2s->i2s_dr, regData ));
        if (wChannels == STEREO_SIGNAL_CHANNEL)
        {
            for (i=0; i<byteAlign; i++)
                *readBuf++ =  regData >> ((offset+i)*8);
            numSample--;
        }
    }

}


/*
 * i2s_write_data()
 *
 * Write data to I2S device from data buffer 'pData' by length 'data_len' 
 * through KS8692 I2C controller.
 */
static int i2s_write_data 
(   
    uchar  *pData, 
    ulong   numSample,
    ushort  wChannels,
    ulong   byteAlign
)
{
    ulong  status;
    int    txValidCounter;
    ulong  writeSampleCount=0;



    DPRINT(I2S_DBG_WRITE,
           (" %s: data=%08x, numSample=%x \n", __FUNCTION__, pData, numSample ));

    if (pData == 0 || numSample == 0) 
    {
        /* Don't support data transfer of no length or to address 0 */
        DPRINT(I2S_DBG_ERROR,
               ("%s: bad call\n", __FUNCTION__));
        return I2S_NOK;
    }

    /* Get transmit valid buffer counter in I2S_TSR */
    if ( GetTxCounter ( &txValidCounter ) != I2S_OK )
    {
        /* tx buffer is not valid yet this time, try again later */
        DPRINT(I2S_DBG_ERROR,
               ("%s: tx buffer is not valid\n", __FUNCTION__));
        return I2S_OK;  
    }

    /* Write data to I2S_DR */
    writeSampleCount = ( txValidCounter > numSample) ? numSample : txValidCounter;
    set_i2s_data ( wChannels, (void *)pData, writeSampleCount, byteAlign );

    /* Clear Tx status register I2S_TSR */
    status = ksI2s->i2s_tsr;
    DPRINT(I2S_DBG_WRITE,
           (" %s: i2s_tsr=%08X:%08x, writeSampleCount=%x \n", __FUNCTION__, &ksI2s->i2s_tsr, status, writeSampleCount ));

    return ( writeSampleCount );

}

/*
 * i2s_read_data()
 *
 * Read data from I2S device to data buffer 'dataBuf' by number of sample 'numSample' 
 * through KS8692 I2C controller.
 */
static int i2s_read_data 
(   
    uchar  *dataBuf, 
    int     numSample,
    ushort  wChannels,
    ulong   byteAlign
)
{
    ulong  status;
    int    rxCounter;
    ulong  readSampleCount;




    DPRINT(I2S_DBG_READ,
           (" %s: dataBuf=%08x, numSample=%x \n", __FUNCTION__, dataBuf, numSample ));

    if (dataBuf == 0 || numSample == 0) 
    {
        /* Don't support data transfer of no length or to address 0 */
        DPRINT(I2S_DBG_ERROR,
               ("%s: bad call\n", __FUNCTION__));
        return I2S_NOK;
    }

    /* Get receive buffer counter in I2S_RSR */
    if ( CheckRxCounter ( &rxCounter ) != I2S_OK )
    {
        /* tx buffer is not valid yet this time, try again later */
        return I2S_OK;  
    }

    /* Read data from I2S_DR. */
    readSampleCount = ( rxCounter > numSample) ? numSample : rxCounter;
    get_i2s_data ( wChannels, dataBuf, readSampleCount, byteAlign);  

    /* Clear Tx status register I2S_TSR */
    status = ksI2s->i2s_rsr;
    DPRINT(I2S_DBG_READ,
           (" %s: i2s_rsr=%08X:%08x, readSampleCount=%x \n", __FUNCTION__, &ksI2s->i2s_rsr, status, readSampleCount ));

    return (readSampleCount);
}


/*----------------------Extern Function --------------------------------*/

/* 
 * Init I2S Control register and enable it.
 */
void i2s_init 
(
    char   rd_flag,
    ulong  fmt,            /* 0:I2S timing; 1:left justified timing */
    ushort wBitsPerSample
)
{
    unsigned long  regData=0;



    /* disable Tx/Rx before change setting */
    ksI2s->i2s_cr &= ~(I2S_TX_ENABLE | I2S_RX_ENABLE);

    if (wBitsPerSample == BITS24_PER_SAMPLE)
        regData = I2S_DATA_24BIT;
    else if (wBitsPerSample == BITS20_PER_SAMPLE)
        regData = I2S_DATA_20BIT;
    else if (wBitsPerSample == BITS18_PER_SAMPLE)
        regData = I2S_DATA_18BIT;
    else 
        regData = I2S_DATA_16BIT;

    if ( fmt==0 )
        regData &= ~I2S_LEFT_JUSTIFIED ;
    else
        regData |= I2S_LEFT_JUSTIFIED ;

    regData |= (3 << 16) | I2S_RX_INT_THRESHOLD2 | I2S_TX_INT_THRESHOLD2 ;

    if ( rd_flag )
        regData |= I2S_RX_ENABLE;
    else
        regData |= I2S_TX_ENABLE;
 

    ksI2s->i2s_cr = regData;
    DPRINT(I2S_DBG_INIT,
           ("%s: i2s_cr=%08X:%08x \n", __FUNCTION__, &ksI2s->i2s_cr, regData ));

    /* Clear Tx status register I2S_TSR */
    if ( rd_flag )
    {
        regData = ksI2s->i2s_rsr;
        DPRINT(I2S_DBG_INIT,
               ("%s: i2s_rsr=%08X:%08x \n", __FUNCTION__, &ksI2s->i2s_rsr, regData ));
    }
    else
    {
        regData = ksI2s->i2s_tsr;
        DPRINT(I2S_DBG_INIT,
               ("%s: i2s_tsr=%08X:%08x \n", __FUNCTION__, &ksI2s->i2s_tsr, regData ));
    }
}

/* 
 * Disable I2S Control.
 */
void i2s_disable 
(
    char   rd_flag
)
{
    unsigned long  regData=0;


    regData = ksI2s->i2s_cr;

    if ( rd_flag )
        regData &= ~I2S_RX_ENABLE;
    else
        regData &= ~I2S_TX_ENABLE;
 
    ksI2s->i2s_cr = regData;
    DPRINT(I2S_DBG_INIT,
           ("%s: i2s_cr=%08X:%08x \n", __FUNCTION__, &ksI2s->i2s_cr, regData ));

}


/*
 * i2s_read()
 *
 * Receive Wave data from I2S controller.  
 */
int i2s_read 
(
    uchar  *dataBuf,
    int     numSample,             /* total data sample */
    ushort	wBitsPerSample,        /* 16, 18, 20, or 24bits per sample */
    ushort	wChannels,             /* 0 (mono) or 1 (stereo) channel */
    ulong	fmt                    /* 0:I2S timing; 1:left justified timing */
)
{
    int    actuallyReadSample=0;
    ulong  byteAlign;


    DPRINT(I2S_DBG_READ,
           ("%s: dataBuf=0x%0x, numSample=%x, wBitsPerSample=%x, wChannels=%x \n", __FUNCTION__, 
                 dataBuf, numSample, wBitsPerSample, wChannels ));

    /* We only support stereo or mono signal */
    if ( ( wChannels != STEREO_SIGNAL_CHANNEL) &&
         ( wChannels != MONO_SIGNAL_CHANNEL) 
       )
    {
        DPRINT(I2S_DBG_ERROR,
               ("%s: ERROR, we don't support this channel number. Channel number is %x\n", 
                     __FUNCTION__, wChannels));
        return 1;
    }

    /* We only support 16, 18, 20, 24 bits per sample */
    if ( ( wBitsPerSample != BITS16_PER_SAMPLE) && 
         ( wBitsPerSample != BITS18_PER_SAMPLE) &&
         ( wBitsPerSample != BITS20_PER_SAMPLE) &&
         ( wBitsPerSample != BITS24_PER_SAMPLE) )
    {
        DPRINT(I2S_DBG_ERROR,
               ("%s: ERROR, we don't support this number of bits per sample. Bits per sample is %x\n", 
                     __FUNCTION__, wBitsPerSample));
        return 1;
    }

    if (wBitsPerSample == BITS16_PER_SAMPLE)
        byteAlign = 2;
    else
        byteAlign = 3;

    memset ((char *)dataBuf, 0, 0x1000000);

    /*
     * Init ks8692 I2S controller according to Wave file format parameters  
     */

    i2s_init(I2S_READ_FLAG, fmt, wBitsPerSample );


    /*
     * Receive Waveform data from ks8692 I2S controller  
     */

    printf("receiving waveform data (%x samples) to memory 0x%08x...", numSample, dataBuf);
    while ( numSample > 0 )
    {
        if ( (actuallyReadSample = i2s_read_data ( dataBuf, 
                                                   numSample, 
                                                   wChannels, 
                                                   byteAlign )) <= I2S_OK ) 
        {
            DPRINT(I2S_DBG_ERROR,
                   ("%s: ERROR, failed %x\n", __FUNCTION__, actuallyReadSample));
            return 1;
        }
        dataBuf += (actuallyReadSample * byteAlign);
        numSample -= (actuallyReadSample);
    }

    /* Disbale Receive */
    i2s_disable (I2S_READ_FLAG ); 

    return 0;
}


/*
 * i2s_write()
 *
 * Write Waveform data 'pWaveData' to I2S controller by 
 * total number of sample 'dwNumSample'.  
 */
int i2s_write 
(
    uchar  *pWaveData,
    int     dwNumSample,           /* total data sample */
    ushort	wBitsPerSample,        /* 16, 18, 20, or 24bits per sample */
    ushort	wChannels,             /* 0 (mono) or 1 (stereo) channel */
    ulong   byteAlign,             /* how many bytes per sample */
    ulong   fmt,                   /* 0:I2S timing; 1:left justified timing */
    ulong   repeatCount
)
{
    char   *dataBuf;
    int     numSample;
    int     actuallyWrittenSample=0;


    DPRINT(I2S_DBG_WRITE,
          ("%s: pWaveData=0x%0x, numSample=%x, wBitsPerSample=%x, wChannels=%x, byteAlign=%x, fmt=%x \n", __FUNCTION__, 
                 pWaveData, dwNumSample, wBitsPerSample, wChannels, byteAlign, fmt ));

    /* We only support stereo or mono signal */
    if ( ( wChannels != STEREO_SIGNAL_CHANNEL) &&
         ( wChannels != MONO_SIGNAL_CHANNEL) 
       )
    {
        DPRINT(I2S_DBG_ERROR,
               ("%s: ERROR, we don't support this channel number. Channel number is %x\n", 
                     __FUNCTION__, wChannels));
        return 1;
    }

    /* We only support 16, 18, 20, 24 bits per sample */
    if ( ( wBitsPerSample != BITS16_PER_SAMPLE) && 
         ( wBitsPerSample != BITS18_PER_SAMPLE) &&
         ( wBitsPerSample != BITS20_PER_SAMPLE) &&
         ( wBitsPerSample != BITS24_PER_SAMPLE) )
    {
        DPRINT(I2S_DBG_ERROR,
               ("%s: ERROR, we don't support this number of bits per sample. Bits per sample is %x\n", 
                     __FUNCTION__, wBitsPerSample));
        return 1;
    }

    /*
     * Init ks8692 I2S controller according to Wave file format parameters  
     */

    i2s_init(I2S_WRITE_FLAG, fmt, wBitsPerSample );

    /*
     * Transmit Waveform data to ks8692 I2S controller  
     */

    printf("sending waveform data (%x samples) from memory 0x%08x...", dwNumSample, pWaveData);
    while ( repeatCount-- > 0 )
    {
        dataBuf = (char *)pWaveData;        /* Get the data pointer */
        numSample = dwNumSample;
        while ( numSample > 0 )
        {
            DPRINT(I2S_DBG_WRITE,
                   ("%s: dataBuf=%08x, numSample=%x \n", __FUNCTION__,dataBuf, numSample ));
            if ( (actuallyWrittenSample = i2s_write_data ( dataBuf, 
                                                           numSample, 
                                                           wChannels, 
                                                           byteAlign )) < I2S_OK )
            {
                DPRINT(I2S_DBG_ERROR,
                       ("%s: ERROR, failed %x\n", __FUNCTION__, actuallyWrittenSample));
                return 1;
            }
            dataBuf   += (actuallyWrittenSample * byteAlign );
            numSample -= actuallyWrittenSample;
            DPRINT(I2S_DBG_WRITE,
                   ("%s: actuallyWrittenSample=%08x, numSample=%x \n", __FUNCTION__,actuallyWrittenSample, numSample ));
        }
    }

    /* Disbale Transmit */
    i2s_disable (I2S_WRITE_FLAG ); 

    return 0;
}

/*
 * i2s_read_file()
 *
 * Using Wave file to verify I2S controller.  
 */
int i2s_read_file 
(
    uchar  *waveFileBuf,
    ulong	fmt,            /* 0:I2S timing; 1:left justified timing */
    ulong	repeatCount
)
{
    RIFF_CHUNK   *riffChunk=(RIFF_CHUNK *)waveFileBuf;
    FORMAT_CHUNK *fmtChunk =(FORMAT_CHUNK *)&riffChunk->pFmt;
    DATA_CHUNK   *dataChunk;
    uchar  *pData;
    uchar   nameStr[5];
    int     dataLen;
    int     numSample;           /* total data sample = data Length /  wBlockalign / wChannels */
    ulong   byteAlign;           /* how many bytes per sample */



    memset(nameStr, 0, sizeof(nameStr));
    memcpy((char *)&nameStr[0], (char *)&riffChunk->chunkId, 4);
    DPRINT(I2S_DBG_READ,
           ("%s: waveFileBuf=%08X:%s, fmt=%x, repeatCount=%x\n", __FUNCTION__, 
             waveFileBuf, nameStr, fmt, repeatCount ));

    /* display wave file information */
    wavefmt_info ( fmtChunk ); 

    /*
     * Verify Wave file format 
     */

    /* We only verify Wave file data */
    if ( (riffChunk->chunkId != RIFF_ID) && ( riffChunk->riffType != WAVE_FORM) )
    {
        DPRINT(I2S_DBG_ERROR,
               ("%s: ERROR, this is not RIFF file. Chunk id is %s\n", 
                     __FUNCTION__, nameStr));
        return 1;
    }
    if ( fmtChunk->chunkId != FMT_ID)
    {
        memset(nameStr, 0, sizeof(nameStr));
        memcpy(nameStr, (char *)&fmtChunk->chunkId, 4);
        DPRINT(I2S_DBG_ERROR,
               ("%s: ERROR, this is not Format Chunk. Chunk id is %s\n", 
                     __FUNCTION__, nameStr));
        return 1;
    }

    /* We only support uncompressed waveform data */
    if ( fmtChunk->wCompressionCode != UNCOMPRESSED_DATA)
    {
        DPRINT(I2S_DBG_ERROR,
               ("%s: ERROR, this is not uncompressed waveform data. Compression code is %x\n", 
                     __FUNCTION__, fmtChunk->wCompressionCode));
        return 1;
    }

    /* We only stereo or mono signal */
    if ( ( fmtChunk->wChannels != STEREO_SIGNAL_CHANNEL) &&
         ( fmtChunk->wChannels != MONO_SIGNAL_CHANNEL) 
       )
    {
        DPRINT(I2S_DBG_ERROR,
               ("%s: ERROR, we don't support this channel number. Channel number is %x\n", 
                     __FUNCTION__, fmtChunk->wChannels));
        return 1;
    }

    /* We only support 16, 18, 20, 24 bits per sample */
    if ( ( fmtChunk->wBitsPerSample != BITS16_PER_SAMPLE) && 
         ( fmtChunk->wBitsPerSample != BITS18_PER_SAMPLE) &&
         ( fmtChunk->wBitsPerSample != BITS20_PER_SAMPLE) &&
         ( fmtChunk->wBitsPerSample != BITS24_PER_SAMPLE) )
    {
        DPRINT(I2S_DBG_ERROR,
               ("%s: ERROR, we don't support this number of bits per sample. Bits per sample is %x\n", 
                     __FUNCTION__, fmtChunk->wBitsPerSample));
        return 1;
    }

    /* Verify Data Chunk pointer */
    pData =  (uchar *)( (uchar *)&fmtChunk->wCompressionCode + fmtChunk->chunkSize );
    dataChunk = (DATA_CHUNK *)pData; 
    dataLen = dataChunk->chunkSize ;                   /* Get the data size */

    if ( dataChunk->chunkId != DATA_ID)
    {
        memset(nameStr, 0, sizeof(nameStr));
        memcpy(nameStr, (char *)&dataChunk->chunkId, 4);
        DPRINT(I2S_DBG_ERROR,
               ("%s: ERROR, this is not Data Chunk. Chunk id is %s\n", 
                     __FUNCTION__, nameStr));
        return 1;
    }
  
    DPRINT(I2S_DBG_READ,
          ("%s: data chunk addr=0x%0x, wave data addr =0x%08x, size= 0x%x, wBlockalign=%x \n \n", __FUNCTION__, 
                 dataChunk, &dataChunk->waveformData, dataChunk->chunkSize, fmtChunk->wBlockalign ));

    DPRINT(I2S_DBG_INT,
           ("%s: wCompressionCode=%d, wChannels=%d,  wBitsPerSample=%d, wBlockalign=%d \n", __FUNCTION__, 
             fmtChunk->wCompressionCode, fmtChunk->wChannels, fmtChunk->wBitsPerSample, fmtChunk->wBlockalign));


    byteAlign = fmtChunk->wBlockalign / fmtChunk->wChannels;
    numSample = dataLen / byteAlign;
  
#if (0)
    numSample = 0x40; 
#endif
    /*
     * Transmit Waveform data to ks8692 I2S controller  
     */

    return ( i2s_write ( (uchar *)&dataChunk->waveformData,
                         numSample,  
	                     fmtChunk->wBitsPerSample,
	                     fmtChunk->wChannels, 
                         byteAlign,  
                         fmt, 
                         repeatCount ) );

}

#ifdef CONFIG_USE_IRQ

/*
 * i2s_txIsr()
 *
 * ISR to test I2S Tx interrupt bits. 
 * Do nothing, but just clear I2S Tx interrupt. 
 */
void i2s_txIsr(void)
{
    unsigned long intStatus, intEnable;
    unsigned long status;


    intStatus = ks8692_read_dword(KS8692_INT_STATUS2);
    status = ksI2s->i2s_tsr ;

    DPRINT(I2S_DBG_INT,
           ("%s: i2s_tsr  =%08X:%08x \n", __FUNCTION__, &ksI2s->i2s_tsr, status ));
    DPRINT(I2S_DBG_INT,
           ("%s: intStatus=0x%08X:%08x \n", __FUNCTION__, KS8692_INT_STATUS2, intStatus));

    /* For test only: disable Tx interrupt */
    intEnable = ks8692_read_dword(KS8692_INT_ENABLE2);
    ks8692_write_dword(KS8692_INT_ENABLE2, (intEnable & ~INT_I2S_TX )); 

    /* Do nothing, but just clear interrupt */
    ks8692_write_dword(KS8692_INT_STATUS2, (intStatus & INT_I2S_TX)); 

}

/*
 * i2s_rxIsr()
 *
 * ISR to test I2S Rx interrupt bits. 
 * Do nothing, but just clear I2S Rx interrupt. 
 */
void i2s_rxIsr(void)
{
    unsigned long intStatus, intEnable;
    unsigned long status;

    intStatus = ks8692_read_dword(KS8692_INT_STATUS2);
    status = ksI2s->i2s_rsr ;

    DPRINT(I2S_DBG_INT,
           ("%s: intStatus=0x%08X:%08x \n", __FUNCTION__, KS8692_INT_STATUS2, intStatus));
    DPRINT(I2S_DBG_INT,
           ("%s: i2s_rsr=%08X:%08x \n", __FUNCTION__, &ksI2s->i2s_rsr, status ));

    /* For test only: disable Rx interrupt */
    intEnable = ks8692_read_dword(KS8692_INT_ENABLE2);
    ks8692_write_dword(KS8692_INT_ENABLE2, (intEnable & ~INT_I2S_RX )); 

    /* Do nothing, but just clear interrupt */
    ks8692_write_dword(KS8692_INT_STATUS2, (intStatus & INT_I2S_RX)); 
}


/*
 * i2s_init_txInterrupt()
 *
 * Hook up ISR and enable I2S interrupt to test I2S Tx interrupt bits. 
 * irq 42 is I2S Tx interrupt. 
 */
void i2s_init_txInterrupt ()
{    
    unsigned long intReg;

    /* initialize irq software vector table */
    interrupt_init();

    /* Clear I2S interrupt Tx bits */
    intReg = ks8692_read_dword(KS8692_INT_STATUS2);
    ks8692_write_dword(KS8692_INT_STATUS2, (intReg & INT_I2S_TX )); 

    /* install i2s_isr() */
	irq_install_handler((10+32), (interrupt_handler_t *)i2s_txIsr, NULL);


#ifdef TEST_KS_FIQ
    /* Configure I2S interrupt Tx for FIQ mode */
    intReg = ks8692_read_dword(KS8692_INT_CONTL2);
    ks8692_write_dword(KS8692_INT_CONTL2, (intReg | INT_I2S_TX )); 
#endif

    /* enable I2S interrupt Tx bits */
    intReg = ks8692_read_dword(KS8692_INT_ENABLE2);
    ks8692_write_dword(KS8692_INT_ENABLE2, (intReg | INT_I2S_TX )); 

    DPRINT(I2S_DBG_INT,
           ("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_STATUS2, ks8692_read_dword(KS8692_INT_STATUS2) ));
    DPRINT(I2S_DBG_INT,
           ("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_ENABLE2, ks8692_read_dword(KS8692_INT_ENABLE2) ));
    DPRINT(I2S_DBG_INT,
           ("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_CONTL2, ks8692_read_dword(KS8692_INT_CONTL2) ));
}

/*
 * i2s_init_rxInterrupt()
 *
 * Hook up ISR and enable I2S interrupt to test I2S Rx interrupt bits. 
 * irq 41 is I2S Rx interrupt. 
 */
void i2s_init_rxInterrupt ()
{    
    unsigned long intReg;

    /* initialize irq software vector table */
    interrupt_init();

    /* Clear I2S interrupt Rx bits */
    intReg = ks8692_read_dword(KS8692_INT_STATUS2);
    ks8692_write_dword(KS8692_INT_STATUS2, (intReg & INT_I2S_RX )); 

    /* install i2s_isr() */
	irq_install_handler((9+32), (interrupt_handler_t *)i2s_rxIsr, NULL);


    /* enable I2S interrupt Rx bits */
    intReg = ks8692_read_dword(KS8692_INT_ENABLE2);
    ks8692_write_dword(KS8692_INT_ENABLE2, (intReg | INT_I2S_RX )); 

    DPRINT(I2S_DBG_INT,
           ("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_STATUS2, ks8692_read_dword(KS8692_INT_STATUS2) ));
    DPRINT(I2S_DBG_INT,
           ("%s: %08X:%08x\n", __FUNCTION__, KS8692_INT_ENABLE2, ks8692_read_dword(KS8692_INT_ENABLE2) ));

}
#endif /* #ifdef CONFIG_USE_IRQ */

#endif /* #if (MICREL_CMD_I2S) */ 
