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

 * MII initialize\read\write throught Micrel KS8692 STA Controller.
 */

#include <config.h>
#include <common.h>
#include <malloc.h>
#include <asm/errno.h>
#include <asm/arch/ks8692Reg.h>
#include <asm/arch/platform.h>


#if (CONFIG_COMMANDS & CFG_CMD_MII)

/* Definitions  */

#define STA_DEBUG

#ifdef STA_DEBUG        /* if debugging driver */
#    define STA_DBG_OFF           0x000000
#    define STA_DBG_READ          0x000001
#    define STA_DBG_WRITE         0x000002
#    define STA_DBG_INIT          0x000004
#    define STA_DBG_TEMP          0x000008
#    define STA_DBG_ERROR         0x800000
#    define STA_DBG_ALL           0xffffff

static int ksDebug = ( STA_DBG_ERROR | STA_DBG_TEMP );

#define ks_debug(FLG,phy, X)     \
	    if ((ksDebug & FLG) && ((phy==CFG_PHY0_ADDR) | (phy==CFG_PHY1_ADDR)) ) printf X;

#else /* MMI_DEBUG */

#define ks_debug(FLG,phy,X)     \

#endif /* MMI_DEBUG */


/*
 * TO access KS8692 device.
 */
#define	ks_read(a)                ks8692_read_dword(a)    
#define	ks_read_w(a)              ks8692_read_word(a)	        
#define	ks_read_b(a)              ks8692_read_byte(a)	        

#define	ks_write(a,v)             ks8692_write_dword(a,v) 
#define	ks_write_w(a,v)           ks8692_write_word(a,v)       
#define	ks_write_b(a,v)           ks8692_write_byte(a,v)       



#define  F_MDC                1290   /* 1.29 MHz */
#define  TIMEOUT_COUNT          10


#define STA_WAIT_IDLE(timeOut, status)                         \
{                                                              \
   timeOut = 20 /*TIMEOUT_COUNT*/;                                    \
   status = ks_read( KS8692_STA_STATUS );                      \
   while (  (--timeOut > 0) && (( status & STA_STATUS_MASK ) != STA_IDLE) )  \
   {                                                           \
      udelay( 10 );                                            \
      status = ks_read( KS8692_STA_STATUS );                   \
   }                                                           \
}

#define STA_WAIT_RDONE(timeOut, status)                        \
{                                                              \
   timeOut = TIMEOUT_COUNT;                                    \
   status = ks_read( KS8692_STA_STATUS );                      \
   while ( (--timeOut > 0) && ( (status & STA_READ_DONE) != STA_READ_DONE ) )    \
   {                                                           \
      udelay( 10 );                                            \
      status = ks_read( KS8692_STA_STATUS );                   \
   }                                                           \
}

#define STA_WAIT_WDONE(timeOut, status)                        \
{                                                              \
   timeOut = TIMEOUT_COUNT;                                    \
   status = ks_read( KS8692_STA_STATUS );                      \
   while ( (--timeOut > 0) && ( (status & STA_WRITE_DONE) != STA_WRITE_DONE ) )    \
   {                                                           \
      udelay( 10 );                                            \
      status = ks_read( KS8692_STA_STATUS );                   \
   }                                                           \
}

/* ks_debug(STA_DBG_TEMP, CFG_PHY0_ADDR, ("\n"));              \ PING_READY */

/* Functions Declarations  */

int  hwInitSTA (void);
int  hwReadSTA (u32, u32, u16 *);
int  hwWriteSTA (u32, u32, u16);
void mii_init (void);




/* -------------------------------------------------------------------------- */

/*
    hwInitSTA

    Description:
        This routine initialize Station Management Entity (STA) Controller (MDC/MDIO).
        The default MDC output frequency is fix at 1.29MHz

    Parameters:
        None

    Return (int):
        1 if success; otherwise, -1.
*/

int hwInitSTA (void)
{
    u32    F_apb, clk_dividend;
    u32    RegData, status=0;
    int    timeOut;


    ks_debug(STA_DBG_INIT, CFG_PHY0_ADDR, ("%s> \n", __FUNCTION__));

    /* Get APB system clock in Hz */
    RegData = ks_read( KS8692_SYSTEM_BUS_CLOCK );
    switch (RegData & SYSTEM_BUS_CLOCK_MASK)
    {
        case SYSTEM_BUS_CLOCK_200:
             F_apb = 200000;
             break;
        case SYSTEM_BUS_CLOCK_125:
             F_apb = 125000;
             break;
        case SYSTEM_BUS_CLOCK_50:
             F_apb = 50000;
             break;
        case SYSTEM_BUS_CLOCK_166:
             /* Nothing need to change if system clock is default value */
             return (1); 
        default:
             ks_debug(STA_DBG_ERROR, CFG_PHY0_ADDR, ("%s> system clock error\n", __FUNCTION__));
             return (-1);           
    }

    /* Calculate CLK_DIVIDEND value -- F_mdc = F_apb / (CLK_DIVIDEND * 2) */
    clk_dividend = F_apb / (F_MDC * 2);

    /* Wait for STA is IDLE */
    STA_WAIT_IDLE (timeOut, status);
    if ( ( status & STA_STATUS_MASK ) != STA_IDLE )
    {
       ks_debug(STA_DBG_ERROR, CFG_PHY0_ADDR,
               ("%s> sta wait IDLE timeout (%d), status=0x%x\n", __FUNCTION__, timeOut, status));
       return (-1);           /* Something wrong, STA nerve IDLE */
    }

    RegData = ks_read( KS8692_STA_CONF );

    /*  Disable STA first */
    RegData &= ~(STA_MDIO_ENABLE);
    ks_write( KS8692_STA_CONF, RegData );
    ks_debug(STA_DBG_INIT, CFG_PHY0_ADDR,
               ("%s> %08X:%08x\n", __FUNCTION__, KS8692_STA_CONF, ks_read(KS8692_STA_CONF)));

    /* Wait for STA is IDLE */
    STA_WAIT_IDLE (timeOut, status);
    if ( ( status & STA_STATUS_MASK ) != STA_IDLE )
    {
       ks_debug(STA_DBG_ERROR, CFG_PHY0_ADDR,
               ("%s> sta wait IDLE timeout (%d), status=0x%x\n", __FUNCTION__, timeOut, status));
       return (-1);           /* Something wrong, STA nerve IDLE */
    }

    /*  Set CLK_DIVIDEND value to STA Configuration Register, and enable it */
    RegData &= ~(STA_CLK_DIVIDEND_MASK);
    RegData |= (clk_dividend << 1) & STA_CLK_DIVIDEND_MASK;
    RegData |= STA_MDIO_ENABLE;
    ks_write( KS8692_STA_CONF, RegData );
    ks_debug(STA_DBG_INIT, CFG_PHY0_ADDR,
               ("%s> %08X:%08x\n", __FUNCTION__, KS8692_STA_CONF, ks_read(KS8692_STA_CONF)));

    return (1); 

}  /* hwInitSTA */


/*
    hwReadSTA

    Description:
        This routine is used to read data from the specified PHY register
        through the STA (MDC/MDIO) interface.

    Parameters:
        u32 dwRegAddr
           Address of PHY register to read from.
  
        u32 dwPHYAddr
           Address of PHY to read from.

        u16 *readPHYData
           Address of WORD to contain data read from PHY register.
  
    Return (int):
        1 if success; otherwise, -1.
*/

int hwReadSTA 
(
   u32       dwRegAddr,
   u32       dwPHYAddr,
   u16      *readPHYData
)
{
    u32    RegData, status=0;
    int    timeOut;

    ks_debug(STA_DBG_READ, dwPHYAddr,
               ("%s> RegAddr=0x%x, PHYAddr=0x%x\n", __FUNCTION__, dwRegAddr, dwPHYAddr));

    /* Wait for STA becomes IDLE */
    STA_WAIT_IDLE (timeOut, status);
    if ( ( status & STA_STATUS_MASK ) != STA_IDLE )
    {
        ks_debug(STA_DBG_ERROR, dwPHYAddr,
               ("%s> sta wait IDLE timeout (%d), status=0x%x, RegAddr=0x%x, PHYAddr=0x%x\n", 
                 __FUNCTION__, timeOut, status, dwRegAddr, dwPHYAddr));
        return (-1);           /* Something wrong, STA nerve IDLE */
    }

    /* Read the specified PHY register */
    RegData = (STA_BURST_SIZE1) | 
              ((dwRegAddr << 6) & STA_PHY_REG_ADDR_MASK) |
              ((dwPHYAddr << 1) & STA_PHY_ADDR_MASK)  |
              (STA_READ) ;
    ks_write( KS8692_STA_COMM, RegData );
    ks_debug(STA_DBG_READ, dwPHYAddr,
               ("%s> %08X:%08x\n", __FUNCTION__, KS8692_STA_COMM, ks_read(KS8692_STA_COMM)));

    /* Start STA Read transaction */
    ks_write( KS8692_STA_CTRL, STA_START );

    /* Wait for STA read transaction finished */
    STA_WAIT_RDONE (timeOut, status);
    if ( ( status & STA_READ_DONE ) != STA_READ_DONE )
    {
        ks_debug(STA_DBG_ERROR, dwPHYAddr,
               ("%s> sta wait RDONE timeout (%d), status=0x%08x, RegAddr=0x%x, PHYAddr=0x%x\n", 
                __FUNCTION__, timeOut, status, dwRegAddr, dwPHYAddr));
        return (-1);           /* Something wrong, STA nerve IDLE */
    }

    /* Read PHY register value */
    RegData = ks_read( KS8692_STA_DATA0 );  
    /* ks_debug(STA_DBG_TEMP, dwPHYAddr, ("  ")); PING_READY */
    RegData = ks_read( KS8692_STA_DATA0 );

    *readPHYData = (u16)RegData;

    return (0); 

}  /* hwReadSTA */


/*
    hwWriteSTA

    Description:
        This routine is used to write data to the specified PHY register
        through the STA (MDC/MDIO) interface.

    Parameters:
        u32 dwRegAddr
           Address of PHY register to read from.
  
        u32 dwPHYAddr
           Address of PHY to read from.

        u16 writePHYData
           Data to write to PHY register.
  
    Return (int):
        1 if success; otherwise, -1.
*/

int hwWriteSTA 
(
   u32       dwRegAddr,
   u32       dwPHYAddr,
   u16       writePHYData
)
{
    u32    RegData, status=0;
    int    timeOut;
    u32    dwReadRegAddr=28;
    u16    readData;


    ks_debug(STA_DBG_WRITE, dwPHYAddr,
               ("%s> RegAddr=0x%x, PHYAddr=0x%x, data=0x%x \n", __FUNCTION__, dwRegAddr, dwPHYAddr, writePHYData));

    /* 
     * Enable write to Phy 
     */

    hwReadSTA( dwReadRegAddr, dwPHYAddr, &readData );
    readData |= 0x0004;
 
    /* Wait for STA becomes IDLE */
    STA_WAIT_IDLE (timeOut, status);
    if ( ( status & STA_STATUS_MASK ) != STA_IDLE )
    {
        ks_debug(STA_DBG_ERROR, dwPHYAddr,
               ("%s> sta wait IDLE timeout (%d), status=0x%x, RegAddr=0x%x, PHYAddr=0x%x\n", 
                 __FUNCTION__, timeOut, status, dwRegAddr, dwPHYAddr));
        return (-1);           /* Something wrong, STA nerve IDLE */
    }

    /* Write to the specified PHY register */
    RegData = (STA_BURST_SIZE1) | 
              ((dwReadRegAddr << 6) & STA_PHY_REG_ADDR_MASK) |
              ((dwPHYAddr << 1) & STA_PHY_ADDR_MASK)  |
              (STA_WRITE) ;
    ks_write( KS8692_STA_COMM, RegData );
    ks_debug(STA_DBG_WRITE, dwPHYAddr,
               ("%s> %08X:%08x\n", __FUNCTION__, KS8692_STA_COMM, ks_read(KS8692_STA_COMM)));

    /* Write data to PHY register */
    ks_write_w( KS8692_STA_DATA0, (u16)readData );

    /* Start STA Write transaction */
    ks_write( KS8692_STA_CTRL, STA_START );

    /* Wait for STA write transaction has been finished */
    STA_WAIT_WDONE (timeOut, status);
    if ( (status & STA_WRITE_DONE ) != STA_WRITE_DONE )
    {
        ks_debug(STA_DBG_ERROR, dwPHYAddr,
               ("%s> sta wait WDONE timeout (%d), status=0x%x, RegAddr=0x%x, PHYAddr=0x%x\n", 
                __FUNCTION__, timeOut, status, dwReadRegAddr, dwPHYAddr));
        return (-1);           /* Something wrong, STA nerve IDLE */
    }

    /* 
     * Now, write value to Phy 
     */

    /* Wait for STA becomes IDLE */
    STA_WAIT_IDLE (timeOut, status);
    if ( ( status & STA_STATUS_MASK ) != STA_IDLE )
    {
        ks_debug(STA_DBG_ERROR, dwPHYAddr,
               ("%s> sta wait IDLE timeout (%d), status=0x%x, RegAddr=0x%x, PHYAddr=0x%x\n", 
                 __FUNCTION__, timeOut, status, dwRegAddr, dwPHYAddr));
        return (-1);           /* Something wrong, STA nerve IDLE */
    }

    /* Write to the specified PHY register */
    RegData = (STA_BURST_SIZE1) | 
              ((dwRegAddr << 6) & STA_PHY_REG_ADDR_MASK) |
              ((dwPHYAddr << 1) & STA_PHY_ADDR_MASK)  |
              (STA_WRITE) ;
    ks_write( KS8692_STA_COMM, RegData );
    ks_debug(STA_DBG_WRITE, dwPHYAddr,
               ("%s> %08X:%08x\n", __FUNCTION__, KS8692_STA_COMM, ks_read(KS8692_STA_COMM)));

    /* Write data to PHY register */
    ks_write_w( KS8692_STA_DATA0, (u16)writePHYData );

    /* Start STA Write transaction */
    ks_write( KS8692_STA_CTRL, STA_START );

    /* Wait for STA write transaction has been finished */
    STA_WAIT_WDONE (timeOut, status);
    if ( (status & STA_WRITE_DONE ) != STA_WRITE_DONE )
    {
        ks_debug(STA_DBG_ERROR, dwPHYAddr,
               ("%s> sta wait WDONE timeout (%d), status=0x%x, RegAddr=0x%x, PHYAddr=0x%x\n", 
                __FUNCTION__, timeOut, status, dwRegAddr, dwPHYAddr));
        return (-1);           /* Something wrong, STA nerve IDLE */
    }

    return (0); 

}  /* hwWriteSTA */


static int mii_init_done = 0;

/****************************************************************************
 * mii_init -- Initialize the MII for MII command without ethernet
 * This function is a subset of eth_init
 ****************************************************************************
 */
void mii_init (void)
{

    ks_debug(STA_DBG_INIT, CFG_PHY0_ADDR, ("%s> \n", __FUNCTION__));

    if (mii_init_done != 0) 
       return;

    hwInitSTA();
    mii_init_done = 1;
}

#endif	/* CONFIG_DRIVER_KS8695ETH */



