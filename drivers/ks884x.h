/*------------------------------------------------------------------------
 . ks884x.h - Micrel KS884X Ethernet driver header
 .
 . (C) Copyright 2006
 . Micrel, Inc.
 .
 . (C) Copyright 2002
 . Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 . Rolf Offermanns <rof@sysgo.de>
 .
 . This program is free software; you can redistribute it and/or modify
 . it under the terms of the GNU General Public License as published by
 . the Free Software Foundation; either version 2 of the License, or
 . (at your option) any later version.
 .
 . This program is distributed in the hope that it will be useful,
 . but WITHOUT ANY WARRANTY; without even the implied warranty of
 . MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 . GNU General Public License for more details.
 .
 . You should have received a copy of the GNU General Public License
 . along with this program; if not, write to the Free Software
 . Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 .
 ---------------------------------------------------------------------------*/
#ifndef _KS884X_H_
#define _KS884X_H_

#include <asm/types.h>
#include <config.h>


#ifdef KS_ISA_BUS

/* Only support generic bus for now. */
#if 1
#define SH_BUS
#endif

/* SuperH needs to be aligned at 32-bit for memory i/o. */
#if 1
#define SH_32BIT_ALIGNED
#endif

#ifndef ADDR_SHIFT
#define ADDR_SHIFT  0
#endif
#ifndef ADDR_BASE
#define ADDR_BASE   0
#endif

/* Some hardware platforms can only do 32-bit access. */
/* Temporary workaround.  Need to rewrite driver for better performance. */
#if 0
#define SH_32BIT_ACCESS_ONLY
#endif

/* Some hardware platforms cannot do byte write successfully. */
#if 0
#define SH_16BIT_WRITE
#endif
#endif


typedef unsigned char   BYTE;
typedef unsigned char   UCHAR;
typedef unsigned char   UINT8;
typedef unsigned char*  PUCHAR;
typedef unsigned short  WORD;
typedef unsigned short  USHORT;
typedef unsigned short* PUSHORT;
typedef unsigned long   DWORD;
typedef unsigned long   ULONG;
typedef unsigned long   UINT32;
typedef unsigned long*  PULONG;
typedef void*           PVOID;

typedef int             BOOLEAN;
typedef int*            PBOOLEAN;


typedef unsigned long   ULONGLONG;
typedef unsigned long*  PULONGLONG;


#define FAR

#define FALSE  0
#define TRUE   1


#define NEWLINE    "\n"
#define DBG_PRINT  printf


#define ASSERT( x )

#define MOVE_MEM( dest, src, len )                                          \
    memcpy( dest, src, len )

#define DelayMillisec( ms )  udelay( ms * 1000 )


#define MIO_DWORD( x )  *(( volatile unsigned long* )( x ))
#define MIO_WORD( x )   *(( volatile unsigned short* )( x ))
#define MIO_BYTE( x )   *(( volatile unsigned char* )( x ))


#ifdef CONFIG_KS884X_LOAD_BOARD

#ifdef SH_16BIT_ACCESS_ONLY
/* 
 *  Micrel KS884X Load Board
 *  WORD Access Only 
 *  for KS884X-16 
 */

#define HW_READ_BYTE( phwi, addr, data )                                    \
{                                                                           \
    ULONG dwDataRead;                                                       \
    int   shiftBit = (( addr ) & 0x01 ) << 3;                               \
                                                                            \
    HW_READ_WORD( phwi, (( addr ) & ~0x1 ), &dwDataRead );                  \
    *( data ) = ( UCHAR )( dwDataRead >> shiftBit );                        \
}

#define HW_WRITE_BYTE( phwi, addr, data )                                   \
{                                                                           \
    ULONG addrByDwordAligned = ( addr ) & ~0x1;                             \
    USHORT wDataMask = 0xFF;                                                \
    USHORT wDataRead;                                                       \
    USHORT wDataWrite;                                                      \
    int   shiftBit = (( addr ) & 0x01 ) << 3;                               \
                                                                            \
    wDataMask <<= shiftBit;                                                 \
    HW_READ_WORD( phwi, addrByDwordAligned, &wDataRead );                   \
    wDataRead &= ~wDataMask;                                                \
    wDataWrite = ((( data ) << shiftBit ) | wDataRead );                    \
    HW_WRITE_WORD( phwi, addrByDwordAligned, wDataWrite );                  \
}

#define HW_READ_WORD( phwi, addr, data )                                    \
{                                                                           \
    *( data ) = MIO_WORD(( phwi )->m_pVirtualMemory +                       \
	            ADDR_BASE + (( addr ) << ADDR_SHIFT )) ;                    \
}

#define HW_WRITE_WORD( phwi, addr, data )                                   \
{                                                                           \
    MIO_WORD(( phwi )->m_pVirtualMemory +                                   \
        ADDR_BASE + (( addr ) << ADDR_SHIFT )) = ( ULONG )( data );         \
}

#define HW_READ_DWORD( phwi, addr, data )                                   \
{                                                                           \
    ULONG data1;                                                            \
                                                                            \
    HW_READ_WORD( phwi, (addr+2), &data1);                                  \
    *( data ) = (data1 << 16);                                              \                                              
    HW_READ_WORD( phwi, addr, &data1);                                      \
    *( data ) |= (data1);                                                   \                                                     
}

#define HW_WRITE_DWORD( phwi, addr, data )                                  \
{                                                                           \
    HW_WRITE_WORD( phwi, (addr),   data );                                  \
    HW_WRITE_WORD( phwi, (addr+2), (data >> 16) );                          \
}

#define HW_READ_BUFFER( phwi, addr, data, len )                             \
{                                                                           \
    PUSHORT pData = ( PUSHORT )( data );                                    \
    int     nRead = (( len ) + 3 ) >> 2;                                    \
    while ( nRead-- ) {                                                     \
        *pData++ = MIO_WORD(( phwi )->m_pVirtualMemory +                    \
                              ADDR_BASE + (( addr ) << ADDR_SHIFT ));       \
        *pData++ = MIO_WORD(( phwi )->m_pVirtualMemory +                    \
                              ADDR_BASE + (( addr + 2 ) << ADDR_SHIFT ));   \
    }                                                                       \
}

#define HW_WRITE_BUFFER( phwi, addr, data, len )                            \
{                                                                           \
    PUSHORT pData = ( PUSHORT )( data );                                    \
    int     nWrite = (( len ) + 3 ) >> 2;                                   \
    while ( nWrite-- ) {                                                    \
        MIO_WORD(( phwi )->m_pVirtualMemory +                               \
                   ADDR_BASE + (( addr ) << ADDR_SHIFT )) = *pData++;       \
        MIO_WORD(( phwi )->m_pVirtualMemory +                               \
                   ADDR_BASE + (( addr + 2) << ADDR_SHIFT )) = *pData++;    \
    }                                                                       \
}

#elif defined(SH_32BIT_ACCESS_ONLY)

/* 
 *  Micrel KS884X Load Board
 *  DWORD Access Only 
 *  for KS884X-32 
 */

#define HW_READ_BYTE( phwi, addr, data )                                    \
{                                                                           \
    ULONG dwDataRead;                                                       \
    int   shiftBit = (( addr ) & 0x03 ) << 3;                               \
                                                                            \
    HW_READ_DWORD( phwi, (( addr ) & 0x0C ), &dwDataRead );                 \
    *( data ) = ( UCHAR )( dwDataRead >> shiftBit );                        \
}

#define HW_WRITE_BYTE( phwi, addr, data )                                   \
{                                                                           \
    ULONG addrByDwordAligned = ( addr ) & 0x0C;                             \
    ULONG dwDataMask = 0xFF;                                                \
    ULONG dwDataRead;                                                       \
    ULONG dwDataWrite;                                                      \
    int   shiftBit = (( addr ) & 0x03 ) << 3;                               \
                                                                            \
    dwDataMask <<= shiftBit;                                                \
    HW_READ_DWORD( phwi, addrByDwordAligned, &dwDataRead );                 \
    dwDataRead &= ~dwDataMask;                                              \
    dwDataWrite = ((( data ) << shiftBit ) | dwDataRead );                  \
    HW_WRITE_DWORD( phwi, addrByDwordAligned, dwDataWrite );                \
}

#define HW_READ_WORD( phwi, addr, data )                                    \
{                                                                           \
    ULONG dwDataRead;                                                       \
    int   shiftBit = (( addr ) & 0x03 ) << 3;                               \
                                                                            \
    HW_READ_DWORD( phwi, (( addr ) & 0x0C ), &dwDataRead );                 \
    *( data ) = ( USHORT )( dwDataRead >> shiftBit );                       \
}

#define HW_WRITE_WORD( phwi, addr, data )                                   \
{                                                                           \
    ULONG addrByDwordAligned = ( addr ) & 0x0C;                             \
    ULONG dwDataMask = 0xFFFF;                                              \
    ULONG dwDataRead;                                                       \
    ULONG dwDataWrite;                                                      \
    int   shiftBit = (( addr ) & 0x03 ) << 3;                               \
                                                                            \
    dwDataMask <<= shiftBit;                                                \
    HW_READ_DWORD( phwi, addrByDwordAligned, &dwDataRead );                 \
    dwDataRead &= ~dwDataMask;                                              \
    dwDataWrite = ((( data ) << shiftBit ) | dwDataRead );                  \
    HW_WRITE_DWORD( phwi, addrByDwordAligned, dwDataWrite );                \
}

#define HW_READ_DWORD( phwi, addr, data )                                   \
{                                                                           \
    *( data ) = MIO_DWORD(( phwi )->m_pVirtualMemory +                      \
	ADDR_BASE + (( addr ) << ADDR_SHIFT )) ;                                \
}


#define HW_WRITE_DWORD( phwi, addr, data )                                  \
{                                                                           \
    MIO_DWORD(( phwi )->m_pVirtualMemory +                                  \
        ADDR_BASE + (( addr ) << ADDR_SHIFT )) = ( ULONG )( data );         \
}

#define HW_READ_BUFFER( phwi, addr, data, len )                             \
{                                                                           \
    PULONG pData = ( PULONG )( data );                                      \
    int    nRead = (( len ) + 3 ) >> 2;                                     \
    while ( nRead-- ) {                                                     \
        *pData++ = MIO_DWORD(( phwi )->m_pVirtualMemory +                   \
	    ADDR_BASE + (( addr ) << ADDR_SHIFT ));                         \
    }                                                                       \
}

#define HW_WRITE_BUFFER( phwi, addr, data, len )                            \
{                                                                           \
    PULONG pData = ( PULONG )( data );                                      \
    int    nWrite = (( len ) + 3 ) >> 2;                                    \
    while ( nWrite-- ) {                                                    \
        MIO_DWORD(( phwi )->m_pVirtualMemory +                              \
	    ADDR_BASE + (( addr ) << ADDR_SHIFT )) = *pData++;              \
    }                                                                       \
}

#else
  #error unknown device ...

#endif /* #ifdef SH_16BIT_ACCESS_ONLY */

#else /* #ifdef CONFIG_KS884X_LOAD_BOARD */

/* 
 *  Other Host CPU Platform
 *  BYTE/WORD/DWORD Access Capability 
 */

#define HW_READ_BYTE( phwi, addr, data )                                    \
    *( data ) = MIO_BYTE(( phwi )->m_pVirtualMemory + ( addr ))

#define HW_WRITE_BYTE( phwi, addr, data )                                   \
    MIO_BYTE(( phwi )->m_pVirtualMemory + ( addr )) = ( UCHAR )( data )

#define HW_READ_WORD( phwi, addr, data )                                    \
    *( data ) = MIO_WORD(( phwi )->m_pVirtualMemory + ( addr ))

#define HW_WRITE_WORD( phwi, addr, data )                                   \
    MIO_WORD(( phwi )->m_pVirtualMemory + ( addr )) = ( USHORT )( data )

#define HW_READ_DWORD( phwi, addr, data )                                   \
{                                                                           \
    *( data ) = MIO_DWORD(( phwi )->m_pVirtualMemory +                      \
	ADDR_BASE + (( addr ) << ADDR_SHIFT )) ;                                \
}

#define HW_WRITE_DWORD( phwi, addr, data )                                  \
{                                                                           \
    MIO_DWORD(( phwi )->m_pVirtualMemory +                                  \
        ADDR_BASE + (( addr ) << ADDR_SHIFT )) = ( ULONG )( data );         \
}

#ifdef SH_32BIT_ALIGNED
#define HW_READ_BUFFER( phwi, addr, data, len )                             \
{                                                                           \
    PULONG pData = ( PULONG )( data );                                      \
    int    nRead = (( len ) + 3 ) >> 2;                                     \
    while ( nRead-- ) {                                                     \
        *pData++ = MIO_DWORD(( phwi )->m_pVirtualMemory +                   \
	    ADDR_BASE + (( addr ) << ADDR_SHIFT ));                         \
    }                                                                       \
}

#define HW_WRITE_BUFFER( phwi, addr, data, len )                            \
{                                                                           \
    PULONG pData = ( PULONG )( data );                                      \
    int    nWrite = (( len ) + 3 ) >> 2;                                    \
    while ( nWrite-- ) {                                                    \
        MIO_DWORD(( phwi )->m_pVirtualMemory +                              \
	    ADDR_BASE + (( addr ) << ADDR_SHIFT )) = *pData++;              \
    }                                                                       \
}

#else
#define HW_READ_BUFFER( phwi, addr, data, len )                             \
{                                                                           \
    PUSHORT pData = ( PUSHORT )( data );                                    \
    int     nRead = (( len ) + 3 ) >> 2;                                    \
    while ( nRead-- ) {                                                     \
        *pData++ = MIO_WORD(( phwi )->m_pVirtualMemory + ( addr ));         \
        *pData++ = MIO_WORD(( phwi )->m_pVirtualMemory + ( addr ) + 2 );    \
    }                                                                       \
}

#define HW_WRITE_BUFFER( phwi, addr, data, len )                            \
{                                                                           \
    PUSHORT pData = ( PUSHORT )( data );                                    \
    int     nWrite = (( len ) + 3 ) >> 2;                                   \
    while ( nWrite-- ) {                                                    \
        MIO_WORD(( phwi )->m_pVirtualMemory + ( addr )) = *pData++;         \
        MIO_WORD(( phwi )->m_pVirtualMemory + ( addr ) + 2 ) = *pData++;    \
    }                                                                       \
}
#endif

#endif /* #ifdef CONFIG_KS884X_LOAD_BOARD */




/* -------------------------------------------------------------------------- */

#define MAC_ADDRESS_LENGTH  6

#define TO_LO_BYTE( x )  (( UCHAR )(( x ) >> 8 ))
#define TO_HI_BYTE( x )  (( USHORT )( x ) << 8 )


/* KS884X byte registers */

#define REG_FAMILY_ID               0x88

/* SIDER */
#define REG_CHIP_ID_41              0x8810
#define REG_CHIP_ID_42              0x8800

#define SWITCH_CHIP_ID_MASK_41      0xFF10
#define SWITCH_CHIP_ID_MASK         0xFFF0
#define SWITCH_CHIP_ID_SHIFT        4
#define SWITCH_REVISION_MASK        0x000E
#define SWITCH_START                0x01

/* SGCR1 */
#define REG_SWITCH_CTRL_1           0x02

#define SWITCH_NEW_BACKOFF          0x80
#define SWITCH_802_1P_MASK          0x70
#define SWITCH_802_1P_BASE          7
#define SWITCH_802_1P_SHIFT         4
#define SWITCH_PASS_PAUSE           0x08
#define SWITCH_BUFFER_SHARE         0x04
#define SWITCH_RECEIVE_PAUSE        0x02
#define SWITCH_LINK_AGE             0x01

#define REG_SWITCH_CTRL_1_HI        0x03

#define SWITCH_PASS_ALL             0x80
#define SWITCH_TX_FLOW_CTRL         0x20
#define SWITCH_RX_FLOW_CTRL         0x10
#define SWITCH_CHECK_LENGTH         0x08
#define SWITCH_AGING_ENABLE         0x04
#define SWITCH_FAST_AGE             0x02
#define SWITCH_AGGR_BACKOFF         0x01

/* SGCR2 */
#define REG_SWITCH_CTRL_2           0x04

#define UNICAST_VLAN_BOUNDARY       0x80
#define MULTICAST_STORM_DISABLE     0x40
#define SWITCH_BACK_PRESSURE        0x20
#define FAIR_FLOW_CTRL              0x10
#define NO_EXC_COLLISION_DROP       0x08
#define SWITCH_HUGE_PACKET          0x04
#define SWITCH_LEGAL_PACKET         0x02
#define SWITCH_BUF_RESERVE          0x01

#define REG_SWITCH_CTRL_2_HI        0x05

#define SWITCH_VLAN_ENABLE          0x80
#define SWITCH_IGMP_SNOOP           0x40
#define PRIORITY_SCHEME_SELECT      0x0C
#define PRIORITY_RATIO_HIGH         0x00
#define PRIORITY_RATIO_10           0x04
#define PRIORITY_RATIO_5            0x08
#define PRIORITY_RATIO_2            0x0C
#define SWITCH_MIRROR_RX_TX         0x01

/* SGCR3 */
#define REG_SWITCH_CTRL_3           0x06

#define SWITCH_REPEATER             0x80
#define SWITCH_HALF_DUPLEX          0x40
#define SWITCH_FLOW_CTRL            0x20
#define SWITCH_10_MBIT              0x10
#define SWITCH_REPLACE_VID          0x08
#define BROADCAST_STORM_RATE_HI     0x07

#define REG_SWITCH_CTRL_3_HI        0x07

#define BROADCAST_STORM_RATE_LO     0xFF
#define BROADCAST_STORM_RATE        0x07FF

/* SGCR4 */
#define REG_SWITCH_CTRL_4           0x08

/* SGCR5 */
#define REG_SWITCH_CTRL_5           0x0A

#define PHY_POWER_SAVE              0x40
#define CRC_DROP                    0x20
#define LED_MODE                    0x02
#define TPID_MODE_ENABLE            0x01

/* SGCR6 */
#define REG_SWITCH_CTRL_6           0x0C

#define SWITCH_802_1P_MAP_MASK      3
#define SWITCH_802_1P_MAP_SHIFT     2

/* SGCR7 */
#define REG_SWITCH_CTRL_7           0x0E


/* P1CR1 */
#define REG_PORT_1_CTRL_1           0x10
/* P2CR1 */
#define REG_PORT_2_CTRL_1           0x20
/* P3CR1 */
#define REG_PORT_3_CTRL_1           0x30

#define PORT_BROADCAST_STORM        0x80
#define PORT_DIFFSERV_ENABLE        0x40
#define PORT_802_1P_ENABLE          0x20
#define PORT_BASED_PRIORITY_MASK    0x18
#define PORT_BASED_PRIORITY_BASE    0x03
#define PORT_BASED_PRIORITY_SHIFT   3
#define PORT_PORT_PRIORITY_0        0x00
#define PORT_PORT_PRIORITY_1        0x08
#define PORT_PORT_PRIORITY_2        0x10
#define PORT_PORT_PRIORITY_3        0x18
#define PORT_INSERT_TAG             0x04
#define PORT_REMOVE_TAG             0x02
#define PORT_PRIORITY_ENABLE        0x01

/* P1CR2 */
#define REG_PORT_1_CTRL_2           0x11
/* P2CR2 */
#define REG_PORT_2_CTRL_2           0x21
/* P3CR2 */
#define REG_PORT_3_CTRL_2           0x31

#define PORT_MIRROR_SNIFFER         0x80
#define PORT_MIRROR_RX              0x40
#define PORT_MIRROR_TX              0x20
#define PORT_DOUBLE_TAG             0x10
#define PORT_802_1P_REMAPPING       0x08
#define PORT_VLAN_MEMBERSHIP        0x07

#define REG_PORT_1_CTRL_2_HI        0x12
#define REG_PORT_2_CTRL_2_HI        0x22
#define REG_PORT_3_CTRL_2_HI        0x32

#define PORT_REMOTE_LOOPBACK        0x80
#define PORT_INGRESS_FILTER         0x40
#define PORT_DISCARD_NON_VID        0x20
#define PORT_FORCE_FLOW_CTRL        0x10
#define PORT_BACK_PRESSURE          0x08
#define PORT_TX_ENABLE              0x04
#define PORT_RX_ENABLE              0x02
#define PORT_LEARN_DISABLE          0x01

/* P1VIDCR */
#define REG_PORT_1_CTRL_VID         0x13
/* P2VIDCR */
#define REG_PORT_2_CTRL_VID         0x23
/* P3VIDCR */
#define REG_PORT_3_CTRL_VID         0x33

#define PORT_DEFAULT_VID            0xFFFF

/* P1CR3 */
#define REG_PORT_1_CTRL_3           0x15
/* P2CR3 */
#define REG_PORT_2_CTRL_3           0x25
/* P3CR3 */
#define REG_PORT_3_CTRL_3           0x35

#define PORT_USER_PRIORITY_CEILING  0x10
#define PORT_INGRESS_LIMIT_MODE     0x0C
#define PORT_INGRESS_ALL            0x00
#define PORT_INGRESS_UNICAST        0x04
#define PORT_INGRESS_MULTICAST      0x08
#define PORT_INGRESS_BROADCAST      0x0C
#define PORT_COUNT_IFG              0x02
#define PORT_COUNT_PREAMBLE         0x01

/* P1IRCR */
#define REG_PORT_1_IN_RATE          0x16
/* P1ERCR */
#define REG_PORT_1_OUT_RATE         0x18
/* P2IRCR */
#define REG_PORT_2_IN_RATE          0x26
/* P2ERCR */
#define REG_PORT_2_OUT_RATE         0x28
/* P3IRCR */
#define REG_PORT_3_IN_RATE          0x36
/* P3ERCR */
#define REG_PORT_3_OUT_RATE         0x38

#define PORT_PRIORITY_RATE          0x0F
#define PORT_PRIORITY_RATE_SHIFT    4


#define REG_PORT_1_LINK_MD_CTRL     0x1A
/* P1SCSLMD */
#define REG_PORT_1_LINK_MD_RESULT   0x1B
#define REG_PORT_2_LINK_MD_CTRL     0x2A
/* P2SCSLMD */
#define REG_PORT_2_LINK_MD_RESULT   0x2B

#define PORT_CABLE_DIAG_RESULT      0x60
#define PORT_CABLE_STAT_NORMAL      0x00
#define PORT_CABLE_STAT_OPEN        0x20
#define PORT_CABLE_STAT_SHORT       0x40
#define PORT_CABLE_STAT_FAILED      0x60
#define PORT_START_CABLE_DIAG       0x10
#define PORT_FORCE_LINK             0x08
#define PORT_POWER_SAVING           0x04
#define PORT_PHY_REMOTE_LOOPBACK    0x02
#define PORT_CABLE_FAULT_COUNTER_H  0x01

#define PORT_CABLE_FAULT_COUNTER_L  0xFF
#define PORT_CABLE_FAULT_COUNTER    0x1FF

/* P1CR4 */
#define REG_PORT_1_CTRL_4           0x1C
/* P2CR4 */
#define REG_PORT_2_CTRL_4           0x2C

#define PORT_AUTO_NEG_ENABLE        0x80
#define PORT_FORCE_100_MBIT         0x40
#define PORT_FORCE_FULL_DUPLEX      0x20
#define PORT_AUTO_NEG_SYM_PAUSE     0x10
#define PORT_AUTO_NEG_100BTX_FD     0x08
#define PORT_AUTO_NEG_100BTX        0x04
#define PORT_AUTO_NEG_10BT_FD       0x02
#define PORT_AUTO_NEG_10BT          0x01

#define REG_PORT_1_CTRL_4_HI        0x1D
#define REG_PORT_2_CTRL_4_HI        0x2D

#define PORT_LED_OFF                0x80
#define PORT_TX_DISABLE             0x40
#define PORT_AUTO_NEG_RESTART       0x20
#define PORT_REMOTE_FAULT_DISABLE   0x10
#define PORT_POWER_DOWN             0x08
#define PORT_AUTO_MDIX_DISABLE      0x04
#define PORT_FORCE_MDIX             0x02
#define PORT_LOOPBACK               0x01

/* P1SR */
#define REG_PORT_1_STATUS           0x1E
/* P2SR */
#define REG_PORT_2_STATUS           0x2E

#define PORT_MDIX_STATUS            0x80
#define PORT_AUTO_NEG_COMPLETE      0x40
#define PORT_STATUS_LINK_GOOD       0x20
#define PORT_REMOTE_SYM_PAUSE       0x10
#define PORT_REMOTE_100BTX_FD       0x08
#define PORT_REMOTE_100BTX          0x04
#define PORT_REMOTE_10BT_FD         0x02
#define PORT_REMOTE_10BT            0x01

#define REG_PORT_1_STATUS_HI        0x1F
#define REG_PORT_2_STATUS_HI        0x2F
#define REG_PORT_3_STATUS_HI        0x3F

#define PORT_HP_MDIX                0x80
#define PORT_REVERSED_POLARITY      0x20
#define PORT_RX_FLOW_CTRL           0x10
#define PORT_TX_FLOW_CTRL           0x08
#define PORT_STAT_SPEED_100MBIT     0x04
#define PORT_STAT_FULL_DUPLEX       0x02
#define PORT_REMOTE_FAULT           0x01


#define REG_MEDIA_CONV_PHY_ADDR     0x40


/* TOSR1 */
#define REG_TOS_PRIORITY_CTRL_1     0x60
/* TOSR2 */
#define REG_TOS_PRIORITY_CTRL_2     0x62
/* TOSR3 */
#define REG_TOS_PRIORITY_CTRL_3     0x64
/* TOSR4 */
#define REG_TOS_PRIORITY_CTRL_4     0x66
/* TOSR5 */
#define REG_TOS_PRIORITY_CTRL_5     0x68
/* TOSR6 */
#define REG_TOS_PRIORITY_CTRL_6     0x6A
/* TOSR7 */
#define REG_TOS_PRIORITY_CTRL_7     0x6C
/* TOSR8 */
#define REG_TOS_PRIORITY_CTRL_8     0x6E

#define TOS_PRIORITY_MASK           3
#define TOS_PRIORITY_SHIFT          2


/* MACAR1 */
#define REG_MAC_ADDR_0              0x70
#define REG_MAC_ADDR_1              0x71
/* MACAR2 */
#define REG_MAC_ADDR_2              0x72
#define REG_MAC_ADDR_3              0x73
/* MACAR3 */
#define REG_MAC_ADDR_4              0x74
#define REG_MAC_ADDR_5              0x75


#define REG_USER_DEFINED_0          0x76
#define REG_USER_DEFINED_1          0x77
#define REG_USER_DEFINED_2          0x78


/* IACR */
#define REG_INDIRECT_ACCESS_CTRL_0  0x79
#define REG_INDIRECT_ACCESS_CTRL_1  0x7A

/* IADR1 */
#define REG_INDIRECT_DATA_8         0x7B
/* IADR3 */
#define REG_INDIRECT_DATA_7         0x7C
#define REG_INDIRECT_DATA_6         0x7D
/* IADR2 */
#define REG_INDIRECT_DATA_5         0x7E
#define REG_INDIRECT_DATA_4         0x7F
/* IADR5 */
#define REG_INDIRECT_DATA_3         0x80
#define REG_INDIRECT_DATA_2         0x81
/* IADR4 */
#define REG_INDIRECT_DATA_1         0x82
#define REG_INDIRECT_DATA_0         0x83


/* PHAR */


/* P1MBCR */
/* P2MBCR */
#define PHY_REG_CTRL                0

#define PHY_RESET                   0x8000
#define PHY_LOOPBACK                0x4000
#define PHY_SPEED_100MBIT           0x2000
#define PHY_AUTO_NEG_ENABLE         0x1000
#define PHY_POWER_DOWN              0x0800
#define PHY_MII_DISABLE             0x0400
#define PHY_AUTO_NEG_RESTART        0x0200
#define PHY_FULL_DUPLEX             0x0100
#define PHY_COLLISION_TEST          0x0080
#define PHY_HP_MDIX                 0x0020
#define PHY_FORCE_MDIX              0x0010
#define PHY_AUTO_MDIX_DISABLE       0x0008
#define PHY_REMOTE_FAULT_DISABLE    0x0004
#define PHY_TRANSMIT_DISABLE        0x0002
#define PHY_LED_DISABLE             0x0001

/* P1MBSR */
/* P2MBSR */
#define PHY_REG_STATUS              1

#define PHY_100BT4_CAPABLE          0x8000
#define PHY_100BTX_FD_CAPABLE       0x4000
#define PHY_100BTX_CAPABLE          0x2000
#define PHY_10BT_FD_CAPABLE         0x1000
#define PHY_10BT_CAPABLE            0x0800
#define PHY_MII_SUPPRESS_CAPABLE    0x0040
#define PHY_AUTO_NEG_ACKNOWLEDGE    0x0020
#define PHY_REMOTE_FAULT            0x0010
#define PHY_AUTO_NEG_CAPABLE        0x0008
#define PHY_LINK_STATUS             0x0004
#define PHY_JABBER_DETECT           0x0002
#define PHY_EXTENDED_CAPABILITY     0x0001

/* PHY1ILR */
/* PHY1IHR */
/* PHY2ILR */
/* PHY2IHR */
#define PHY_REG_ID_1                2
#define PHY_REG_ID_2                3

/* P1ANAR */
/* P2ANAR */
#define PHY_REG_AUTO_NEGOTIATION    4

#define PHY_AUTO_NEG_NEXT_PAGE      0x8000
#define PHY_AUTO_NEG_REMOTE_FAULT   0x2000
#if 0
#define PHY_AUTO_NEG_ASYM_PAUSE     0x0800
#endif
#define PHY_AUTO_NEG_SYM_PAUSE      0x0400
#define PHY_AUTO_NEG_100BT4         0x0200
#define PHY_AUTO_NEG_100BTX_FD      0x0100
#define PHY_AUTO_NEG_100BTX         0x0080
#define PHY_AUTO_NEG_10BT_FD        0x0040
#define PHY_AUTO_NEG_10BT           0x0020
#define PHY_AUTO_NEG_SELECTOR       0x001F
#define PHY_AUTO_NEG_802_3          0x0001

/* P1ANLPR */
/* P2ANLPR */
#define PHY_REG_REMOTE_CAPABILITY   5

#define PHY_REMOTE_NEXT_PAGE        0x8000
#define PHY_REMOTE_ACKNOWLEDGE      0x4000
#define PHY_REMOTE_REMOTE_FAULT     0x2000
#define PHY_REMOTE_SYM_PAUSE        0x0400
#define PHY_REMOTE_100BTX_FD        0x0100
#define PHY_REMOTE_100BTX           0x0080
#define PHY_REMOTE_10BT_FD          0x0040
#define PHY_REMOTE_10BT             0x0020

/* P1VCT */
/* P2VCT */
#define PHY_REG_LINK_MD             29

#define PHY_START_CABLE_DIAG        0x8000
#define PHY_CABLE_DIAG_RESULT       0x6000
#define PHY_CABLE_STAT_NORMAL       0x0000
#define PHY_CABLE_STAT_OPEN         0x2000
#define PHY_CABLE_STAT_SHORT        0x4000
#define PHY_CABLE_STAT_FAILED       0x6000
#define PHY_CABLE_10M_SHORT         0x1000
#define PHY_CABLE_FAULT_COUNTER     0x01FF

/* P1PHYCTRL */
/* P2PHYCTRL */
#define PHY_REG_PHY_CTRL            30

#define PHY_STAT_REVERSED_POLARITY  0x0020
#define PHY_STAT_MDIX               0x0010
#define PHY_FORCE_LINK              0x0008
#define PHY_POWER_SAVING            0x0004
#define PHY_REMOTE_LOOPBACK         0x0002

/* -------------------------------------------------------------------------- */

/* KS884X ISA registers */

#ifdef KS_ISA_BUS
#define REG_SWITCH_CTRL_BANK        32

#define REG_SIDER_BANK              REG_SWITCH_CTRL_BANK
#define REG_SIDER_OFFSET            0x00

#define REG_CHIP_ID_OFFSET          REG_SIDER_OFFSET
#define REG_FAMILY_ID_OFFSET        ( REG_CHIP_ID_OFFSET + 1 )

#define REG_SGCR1_BANK              REG_SWITCH_CTRL_BANK
#define REG_SGCR1_OFFSET            0x02

#define REG_SWITCH_CTRL_1_OFFSET    REG_SGCR1_OFFSET
#define REG_SWITCH_CTRL_1_HI_OFFSET ( REG_SWITCH_CTRL_1_OFFSET + 1 )

#define REG_SGCR2_BANK              REG_SWTICH_CTRL_BANK
#define REG_SGCR2_OFFSET            0x04

#define REG_SWITCH_CTRL_2_OFFSET    REG_SGCR2_OFFSET
#define REG_SWITCH_CTRL_2_HI_OFFSET ( REG_SWITCH_CTRL_2_OFFSET + 1 )

#define REG_SGCR3_BANK              REG_SWITCH_CTRL_BANK
#define REG_SGCR3_OFFSET            0x06

#define REG_SWITCH_CTRL_3_OFFSET    REG_SGCR3_OFFSET
#define REG_SWITCH_CTRL_3_HI_OFFSET ( REG_SWITCH_CTRL_3_OFFSET + 1 )

#define REG_SGCR4_BANK              REG_SWITCH_CTRL_BANK
#define REG_SGCR4_OFFSET            0x08

#define REG_SWITCH_CTRL_4_OFFSET    REG_SGCR4_OFFSET

#define REG_SGCR5_BANK              REG_SWITCH_CTRL_BANK
#define REG_SGCR5_OFFSET            0x0A

#define REG_SWITCH_CTRL_5_OFFSET    REG_SGCR5_OFFSET


#define REG_SWITCH_802_1P_BANK      33

#define REG_SGCR6_BANK              REG_SWITCH_802_1P_BANK
#define REG_SGCR6_OFFSET            0x00

#define REG_SWITCH_CTRL_6_OFFSET    REG_SGCR6_OFFSET


#define REG_PHAR_BANK               34
#define REG_PHAR_OFFSET             0x00

#define REG_LBS21R_OFFSET           0x04

#define REG_LBRCTCER_OFFSET         0x08

#define REG_LBRCGR_OFFSET           0x0A


#define REG_CSCR_BANK               35
#define REG_CSCR_OFFSET             0x00

#define REG_PSWIR_OFFSET            0x02

#define REG_PC21R_OFFSET            0x04

#define REG_PC3R_OFFSET             0x06

#define REG_VMCRTCR_OFFSET          0x08


#define REG_S58R_BANK               36
#define REG_S58R_OFFSET             0x00

#define REG_MVI21R_OFFSET           0x04

#define REG_MM1V31R_OFFSET          0x06

#define REG_MMI32R_OFFSET           0x08


#define REG_LPVI21R_BANK            37
#define REG_LPVI21R_OFFSET          0x00

#define REG_LPM1V31R_OFFSET         0x04


#define REG_CSSR_BANK               38
#define REG_CSSR_OFFSET             0x00

#define REG_ASCTR_OFFSET            0x04

#define REG_MS21R_OFFSET            0x08

#define REG_LPS21R_OFFSET           0x0A


#define REG_MAC_ADDR_BANK           39

#define REG_MACAR1_BANK             REG_MAC_ADDR_BANK
#define REG_MACAR1_OFFSET           0x00
#define REG_MACAR2_OFFSET           0x02
#define REG_MACAR3_OFFSET           0x04

#define REG_MAC_ADDR_0_OFFSET       REG_MACAR1_OFFSET
#define REG_MAC_ADDR_1_OFFSET       ( REG_MAC_ADDR_0_OFFSET + 1 )
#define REG_MAC_ADDR_2_OFFSET       REG_MACAR2_OFFSET
#define REG_MAC_ADDR_3_OFFSET       ( REG_MAC_ADDR_2_OFFSET + 1 )
#define REG_MAC_ADDR_4_OFFSET       REG_MACAR3_OFFSET
#define REG_MAC_ADDR_5_OFFSET       ( REG_MAC_ADDR_4_OFFSET + 1 )


#define REG_TOS_PRIORITY_BANK       40
#define REG_TOS_PRIORITY_2_BANK     ( REG_TOS_PRIORITY_BANK + 1 )

#define REG_TOSR1_BANK              REG_TOS_PRIORITY_BANK
#define REG_TOSR1_OFFSET            0x00
#define REG_TOSR2_OFFSET            0x02
#define REG_TOSR3_OFFSET            0x04
#define REG_TOSR4_OFFSET            0x06
#define REG_TOSR5_OFFSET            0x08
#define REG_TOSR6_OFFSET            0x0A

#define REG_TOSR7_BANK              REG_TOS_PRIORITY_2_BANK
#define REG_TOSR7_OFFSET            0x00
#define REG_TOSR8_OFFSET            0x02

#define REG_TOS_1_OFFSET            REG_TOSR1_OFFSET
#define REG_TOS_2_OFFSET            REG_TOSR2_OFFSET
#define REG_TOS_3_OFFSET            REG_TOSR3_OFFSET
#define REG_TOS_4_OFFSET            REG_TOSR4_OFFSET
#define REG_TOS_5_OFFSET            REG_TOSR5_OFFSET
#define REG_TOS_6_OFFSET            REG_TOSR6_OFFSET

#define REG_TOS_7_OFFSET            REG_TOSR7_OFFSET
#define REG_TOS_8_OFFSET            REG_TOSR8_OFFSET


#define REG_IND_ACC_CTRL_BANK       42

#define REG_IACR_BANK               REG_IND_ACC_CTRL_BANK
#define REG_IACR_OFFSET             0x00

#define REG_IADR1_BANK              REG_IND_ACC_CTRL_BANK
#define REG_IADR1_OFFSET            0x02

#define REG_IADR2_BANK              REG_IND_ACC_CTRL_BANK
#define REG_IADR2_OFFSET            0x04

#define REG_IADR3_BANK              REG_IND_ACC_CTRL_BANK
#define REG_IADR3_OFFSET            0x06

#define REG_IADR4_BANK              REG_IND_ACC_CTRL_BANK
#define REG_IADR4_OFFSET            0x08

#define REG_IADR5_BANK              REG_IND_ACC_CTRL_BANK
#define REG_IADR5_OFFSET            0x0A

#define REG_ACC_CTRL_INDEX_OFFSET   REG_IACR_OFFSET
#define REG_ACC_CTRL_SEL_OFFSET     ( REG_ACC_CTRL_INDEX_OFFSET + 1 )

#define REG_ACC_DATA_0_OFFSET       REG_IADR4_OFFSET
#define REG_ACC_DATA_1_OFFSET       ( REG_ACC_DATA_0_OFFSET + 1 )
#define REG_ACC_DATA_2_OFFSET       REG_IADR5_OFFSET
#define REG_ACC_DATA_3_OFFSET       ( REG_ACC_DATA_2_OFFSET + 1 )
#define REG_ACC_DATA_4_OFFSET       REG_IADR2_OFFSET
#define REG_ACC_DATA_5_OFFSET       ( REG_ACC_DATA_4_OFFSET + 1 )
#define REG_ACC_DATA_6_OFFSET       REG_IADR3_OFFSET
#define REG_ACC_DATA_7_OFFSET       ( REG_ACC_DATA_6_OFFSET + 1 )
#define REG_ACC_DATA_8_OFFSET       REG_IADR1_OFFSET


#define REG_PHY_1_CTRL_BANK         45
#define REG_PHY_2_CTRL_BANK         46

#define PHY_BANK_INTERVAL           \
    ( REG_PHY_2_CTRL_BANK - REG_PHY_1_CTRL_BANK )

#define REG_P1MBCR_BANK             REG_PHY_1_CTRL_BANK
#define REG_P1MBCR_OFFSET           0x00

#define REG_P1MBSR_BANK             REG_PHY_1_CTRL_BANK
#define REG_P1MBSR_OFFSET           0x02

#define REG_PHY1ILR_BANK            REG_PHY_1_CTRL_BANK
#define REG_PHY1ILR_OFFSET          0x04

#define REG_PHY1LHR_BANK            REG_PHY_1_CTRL_BANK
#define REG_PHY1LHR_OFFSET          0x06

#define REG_P1ANAR_BANK             REG_PHY_1_CTRL_BANK
#define REG_P1ANAR_OFFSET           0x08

#define REG_P1ANLPR_BANK            REG_PHY_1_CTRL_BANK
#define REG_P1ANLPR_OFFSET          0x0A

#define REG_P2MBCR_BANK             REG_PHY_2_CTRL_BANK
#define REG_P2MBCR_OFFSET           0x00

#define REG_P2MBSR_BANK             REG_PHY_2_CTRL_BANK
#define REG_P2MBSR_OFFSET           0x02

#define REG_PHY2ILR_BANK            REG_PHY_2_CTRL_BANK
#define REG_PHY2ILR_OFFSET          0x04

#define REG_PHY2LHR_BANK            REG_PHY_2_CTRL_BANK
#define REG_PHY2LHR_OFFSET          0x06

#define REG_P2ANAR_BANK             REG_PHY_2_CTRL_BANK
#define REG_P2ANAR_OFFSET           0x08

#define REG_P2ANLPR_BANK            REG_PHY_2_CTRL_BANK
#define REG_P2ANLPR_OFFSET          0x0A


#define REG_PHY_SPECIAL_BANK        47

#define REG_P1VCT_BANK              REG_PHY_SPECIAL_BANK
#define REG_P1VCT_OFFSET            0x00

#define REG_P1PHYCTRL_BANK          REG_PHY_SPECIAL_BANK
#define REG_P1PHYCTRL_OFFSET        0x02

#define REG_P2VCT_BANK              REG_PHY_SPECIAL_BANK
#define REG_P2VCT_OFFSET            0x04

#define REG_P2PHYCTRL_BANK          REG_PHY_SPECIAL_BANK
#define REG_P2PHYCTRL_OFFSET        0x06


#define REG_PHY_CTRL_OFFSET         0x00
#define REG_PHY_STATUS_OFFSET       0x02
#define REG_PHY_ID_1_OFFSET         0x04
#define REG_PHY_ID_2_OFFSET         0x06
#define REG_PHY_AUTO_NEG_OFFSET     0x08
#define REG_PHY_REMOTE_CAP_OFFSET   0x0A

#define REG_PHY_LINK_MD_1_OFFSET    0x00
#define REG_PHY_PHY_CTRL_1_OFFSET   0x02
#define REG_PHY_LINK_MD_2_OFFSET    0x04
#define REG_PHY_PHY_CTRL_2_OFFSET   0x06

#define PHY_SPECIAL_INTERVAL        \
    ( REG_PHY_LINK_MD_2_OFFSET - REG_PHY_LINK_MD_1_OFFSET )


#define REG_PORT_1_CTRL_BANK        48
#define REG_PORT_1_LINK_CTRL_BANK   49
#define REG_PORT_1_LINK_STATUS_BANK 49

#define REG_PORT_2_CTRL_BANK        50
#define REG_PORT_2_LINK_CTRL_BANK   51
#define REG_PORT_2_LINK_STATUS_BANK 51

#define REG_PORT_3_CTRL_BANK        52
#define REG_PORT_3_LINK_CTRL_BANK   53
#define REG_PORT_3_LINK_STATUS_BANK 53

/* Port# Control Register */
#define REG_PORT_CTRL_BANK          REG_PORT_1_CTRL_BANK

/* Port# Link Control Register */
#define REG_PORT_LINK_CTRL_BANK     REG_PORT_1_LINK_CTRL_BANK
#define REG_PORT_LINK_STATUS_BANK   REG_PORT_1_LINK_STATUS_BANK

#define PORT_BANK_INTERVAL          \
    ( REG_PORT_2_CTRL_BANK - REG_PORT_1_CTRL_BANK )

#define REG_P1CR1_BANK              REG_PORT_1_CTRL_BANK
#define REG_P1CR1_OFFSET            0x00

#define REG_P1CR2_BANK              REG_PORT_1_CTRL_BANK
#define REG_P1CR2_OFFSET            0x02

#define REG_P1VIDCR_BANK            REG_PORT_1_CTRL_BANK
#define REG_P1VIDCR_OFFSET          0x04

#define REG_P1CR3_BANK              REG_PORT_1_CTRL_BANK
#define REG_P1CR3_OFFSET            0x06

#define REG_P1IRCR_BANK             REG_PORT_1_CTRL_BANK
#define REG_P1IRCR_OFFSET           0x08

#define REG_P1ERCR_BANK             REG_PORT_1_CTRL_BANK
#define REG_P1ERCR_OFFSET           0x0A

#define REG_P1SCSLMD_BANK           REG_PORT_1_LINK_CTRL_BANK
#define REG_P1SCSLMD_OFFSET         0x00

#define REG_P1CR4_BANK              REG_PORT_1_LINK_CTRL_BANK
#define REG_P1CR4_OFFSET            0x02

#define REG_P1SR_BANK               REG_PORT_1_LINK_CTRL_BANK
#define REG_P1SR_OFFSET             0x04

#define REG_P2CR1_BANK              REG_PORT_2_CTRL_BANK
#define REG_P2CR1_OFFSET            0x00

#define REG_P2CR2_BANK              REG_PORT_2_CTRL_BANK
#define REG_P2CR2_OFFSET            0x02

#define REG_P2VIDCR_BANK            REG_PORT_2_CTRL_BANK
#define REG_P2VIDCR_OFFSET          0x04

#define REG_P2CR3_BANK              REG_PORT_2_CTRL_BANK
#define REG_P2CR3_OFFSET            0x06

#define REG_P2IRCR_BANK             REG_PORT_2_CTRL_BANK
#define REG_P2IRCR_OFFSET           0x08

#define REG_P2ERCR_BANK             REG_PORT_2_CTRL_BANK
#define REG_P2ERCR_OFFSET           0x0A

#define REG_P2SCSLMD_BANK           REG_PORT_2_LINK_CTRL_BANK
#define REG_P2SCSLMD_OFFSET         0x00

#define REG_P2CR4_BANK              REG_PORT_2_LINK_CTRL_BANK
#define REG_P2CR4_OFFSET            0x02

#define REG_P2SR_BANK               REG_PORT_1_LINK_CTRL_BANK
#define REG_P2SR_OFFSET             0x04

#define REG_P3CR1_BANK              REG_PORT_3_CTRL_BANK
#define REG_P3CR1_OFFSET            0x02

#define REG_P3CR2_BANK              REG_PORT_3_CTRL_BANK
#define REG_P3CR2_OFFSET            0x02

#define REG_P3VIDCR_BANK            REG_PORT_3_CTRL_BANK
#define REG_P3VIDCR_OFFSET          0x04

#define REG_P3CR3_BANK              REG_PORT_3_CTRL_BANK
#define REG_P3CR3_OFFSET            0x06

#define REG_P3IRCR_BANK             REG_PORT_3_CTRL_BANK
#define REG_P3IRCR_OFFSET           0x08

#define REG_P3ERCR_BANK             REG_PORT_3_CTRL_BANK
#define REG_P3ERCR_OFFSET           0x0A

#define REG_P3SCSLMD_BANK           REG_PORT_3_LINK_CTRL_BANK
#define REG_P3SCSLMD_OFFSET         0x00

#define REG_P3CR4_BANK              REG_PORT_3_LINK_CTRL_BANK
#define REG_P3CR4_OFFSET            0x02

#define REG_P3SR_BANK               REG_PORT_1_LINK_CTRL_BANK
#define REG_P3SR_OFFSET             0x04

#define REG_PORT_CTRL_1_OFFSET      0x00

#define REG_PORT_CTRL_2_OFFSET      0x02
#define REG_PORT_CTRL_2_HI_OFFSET   0x03

#define REG_PORT_CTRL_VID_OFFSET    0x04

#define REG_PORT_CTRL_3_OFFSET      0x06

#define REG_PORT_IN_RATE_OFFSET     0x08
#define REG_PORT_OUT_RATE_OFFSET    0x0A

#define REG_PORT_LINK_MD_RESULT     0x00
#define REG_PORT_LINK_MD_CTRL       0x01

#define REG_PORT_CTRL_4_OFFSET      0x02

#define REG_PORT_STATUS_OFFSET      0x04
#define REG_PORT_STATUS_HI_OFFSET   0x05
#endif

/* -------------------------------------------------------------------------- */

/* KS884X PCI registers */

#ifdef KS_PCI_BUS
#define REG_SIDER_PCI               0x0400

#define REG_CHIP_ID_OFFSET          REG_SIDER_PCI
#define REG_FAMILY_ID_OFFSET        ( REG_CHIP_ID_OFFSET + 1 )

#define REG_SGCR1_PCI               0x0402

#define REG_SWITCH_CTRL_1_OFFSET    REG_SGCR1_PCI
#define REG_SWITCH_CTRL_1_HI_OFFSET ( REG_SWITCH_CTRL_1_OFFSET + 1 )

#define REG_SGCR2_PCI               0x0404

#define REG_SWITCH_CTRL_2_OFFSET    REG_SGCR2_PCI
#define REG_SWITCH_CTRL_2_HI_OFFSET ( REG_SWITCH_CTRL_2_OFFSET + 1 )

#define REG_SGCR3_PCI               0x0406
#define REG_SGCR3_OFFSET            REG_SGCR3_PCI

#define REG_SWITCH_CTRL_3_OFFSET    REG_SGCR3_PCI
#define REG_SWITCH_CTRL_3_HI_OFFSET ( REG_SWITCH_CTRL_3_OFFSET + 1 )

#define REG_SGCR4_PCI               0x0408
#define REG_SGCR5_PCI               0x040A

#define REG_SGCR6_PCI               0x0410

#define REG_SWITCH_CTRL_6_OFFSET    REG_SGCR6_PCI


#define REG_PHAR_PCI                0x0420
#define REG_LBS21R_PCI              0x0424
#define REG_LBRCTCER_PCI            0x0426


#define REG_MACAR1_PCI              0x0470
#define REG_MACAR2_PCI              0x0472
#define REG_MACAR3_PCI              0x0474

#define REG_MAC_ADDR_0_OFFSET       REG_MACAR1_PCI
#define REG_MAC_ADDR_1_OFFSET       ( REG_MAC_ADDR_0_OFFSET + 1 )
#define REG_MAC_ADDR_2_OFFSET       REG_MACAR2_PCI
#define REG_MAC_ADDR_3_OFFSET       ( REG_MAC_ADDR_2_OFFSET + 1 )
#define REG_MAC_ADDR_4_OFFSET       REG_MACAR3_PCI
#define REG_MAC_ADDR_5_OFFSET       ( REG_MAC_ADDR_4_OFFSET + 1 )


#define REG_TOSR1_PCI               0x0480
#define REG_TOSR2_PCI               0x0482
#define REG_TOSR3_PCI               0x0484
#define REG_TOSR4_PCI               0x0486
#define REG_TOSR5_PCI               0x0488
#define REG_TOSR6_PCI               0x048A
#define REG_TOSR7_PCI               0x0490
#define REG_TOSR8_PCI               0x0492

#define REG_TOS_1_OFFSET            REG_TOSR1_PCI
#define REG_TOS_2_OFFSET            REG_TOSR2_PCI
#define REG_TOS_3_OFFSET            REG_TOSR3_PCI
#define REG_TOS_4_OFFSET            REG_TOSR4_PCI
#define REG_TOS_5_OFFSET            REG_TOSR5_PCI
#define REG_TOS_6_OFFSET            REG_TOSR6_PCI

#define REG_TOS_7_OFFSET            REG_TOSR7_PCI
#define REG_TOS_8_OFFSET            REG_TOSR8_PCI


#define REG_IACR_PCI                0x04A0
#define REG_IACR_OFFSET             REG_IACR_PCI

#define REG_IADR1_PCI               0x04A2
#define REG_IADR2_PCI               0x04A4
#define REG_IADR3_PCI               0x04A6
#define REG_IADR4_PCI               0x04A8
#define REG_IADR5_PCI               0x04AA

#define REG_ACC_CTRL_SEL_OFFSET     REG_IACR_PCI
#define REG_ACC_CTRL_INDEX_OFFSET   ( REG_ACC_CTRL_SEL_OFFSET + 1 )

#define REG_ACC_DATA_0_OFFSET       REG_IADR4_PCI
#define REG_ACC_DATA_1_OFFSET       ( REG_ACC_DATA_0_OFFSET + 1 )
#define REG_ACC_DATA_2_OFFSET       REG_IADR5_PCI
#define REG_ACC_DATA_3_OFFSET       ( REG_ACC_DATA_2_OFFSET + 1 )
#define REG_ACC_DATA_4_OFFSET       REG_IADR2_PCI
#define REG_ACC_DATA_5_OFFSET       ( REG_ACC_DATA_4_OFFSET + 1 )
#define REG_ACC_DATA_6_OFFSET       REG_IADR3_PCI
#define REG_ACC_DATA_7_OFFSET       ( REG_ACC_DATA_6_OFFSET + 1 )
#define REG_ACC_DATA_8_OFFSET       REG_IADR1_PCI


#define REG_P1MBCR_PCI              0x04D0
#define REG_P1MBSR_PCI              0x04D2

#define REG_PHY1ILR_PCI             0x04D4
#define REG_PHY1IHR_PCI             0x04D6

#define REG_P1ANAR_PCI              0x04D8
#define REG_P1ANLPR_PCI             0x04DA

#define REG_P2MBCR_PCI              0x04E0
#define REG_P2MBSR_PCI              0x04E2

#define REG_PHY2ILR_PCI             0x04E4
#define REG_PHY2IHR_PCI             0x04E6

#define REG_P2ANAR_PCI              0x04E8
#define REG_P2ANLPR_PCI             0x04EA

#define REG_P1VCT_PCI               0x04F0
#define REG_P1PHYCTRL_PCI           0x04F2
#define REG_P2VCT_PCI               0x04F4
#define REG_P2PHYCTRL_PCI           0x04F6

#define REG_PHY_1_CTRL_OFFSET       REG_P1MBCR_PCI

#define REG_PHY_SPECIAL_OFFSET      REG_P1VCT_PCI

#define PHY_CTRL_INTERVAL           \
    ( REG_P2MBCR_PCI - REG_P1MBCR_PCI )

#define REG_PHY_CTRL_OFFSET         0x00
#define REG_PHY_STATUS_OFFSET       0x02
#define REG_PHY_ID_1_OFFSET         0x04
#define REG_PHY_ID_2_OFFSET         0x06
#define REG_PHY_AUTO_NEG_OFFSET     0x08
#define REG_PHY_REMOTE_CAP_OFFSET   0x0A

#define REG_PHY_LINK_MD_1_OFFSET    0x00
#define REG_PHY_PHY_CTRL_1_OFFSET   0x02
#define REG_PHY_LINK_MD_2_OFFSET    0x04
#define REG_PHY_PHY_CTRL_2_OFFSET   0x06

#define PHY_SPECIAL_INTERVAL        \
    ( REG_PHY_LINK_MD_2_OFFSET - REG_PHY_LINK_MD_1_OFFSET )

#define REG_P1CR1_PCI               0x0500
#define REG_P1CR2_PCI               0x0502
#define REG_P1VIDR_PCI              0x0504
#define REG_P1CR3_PCI               0x0506
#define REG_P1IRCR_PCI              0x0508
#define REG_P1ERCR_PCI              0x050A
#define REG_P1SCSLMD_PCI            0x0510
#define REG_P1CR4_PCI               0x0512
#define REG_P1SR_PCI                0x0514

#define REG_P2CR1_PCI               0x0520
#define REG_P2CR2_PCI               0x0522
#define REG_P2VIDR_PCI              0x0524
#define REG_P2CR3_PCI               0x0526
#define REG_P2IRCR_PCI              0x0528
#define REG_P2ERCR_PCI              0x052A
#define REG_P2SCSLMD_PCI            0x0530
#define REG_P2CR4_PCI               0x0532
#define REG_P2SR_PCI                0x0534

#define REG_P3CR1_PCI               0x0540
#define REG_P3CR2_PCI               0x0542
#define REG_P3VIDR_PCI              0x0544
#define REG_P3CR3_PCI               0x0546
#define REG_P3IRCR_PCI              0x0548
#define REG_P3ERCR_PCI              0x054A
#define REG_P3SR_PCI                0x0554

#define REG_PORT_1_CTRL_1_PCI       REG_P1CR1_PCI
#define REG_PORT_2_CTRL_1_PCI       REG_P2CR1_PCI
#define REG_PORT_3_CTRL_1_PCI       REG_P3CR1_PCI

#define REG_PORT_CTRL_1             REG_PORT_1_CTRL_1_PCI

#define PORT_CTRL_ADDR( port, addr )                                        \
    addr = REG_PORT_CTRL_1 + port *                                         \
        ( REG_PORT_2_CTRL_1_PCI - REG_PORT_1_CTRL_1_PCI )

#define REG_PORT_CTRL_1_OFFSET      0x00

#define REG_PORT_CTRL_2_OFFSET      0x02
#define REG_PORT_CTRL_2_HI_OFFSET   0x03

#define REG_PORT_CTRL_VID_OFFSET    0x04

#define REG_PORT_CTRL_3_OFFSET      0x06

#define REG_PORT_IN_RATE_OFFSET     0x08
#define REG_PORT_OUT_RATE_OFFSET    0x0A

#define REG_PORT_LINK_MD_RESULT     0x10
#define REG_PORT_LINK_MD_CTRL       0x11

#define REG_PORT_CTRL_4_OFFSET      0x12

#define REG_PORT_STATUS_OFFSET      0x14
#define REG_PORT_STATUS_HI_OFFSET   0x15
#endif

/* -------------------------------------------------------------------------- */

enum {
    TABLE_STATIC_MAC = 0,
    TABLE_VLAN,
    TABLE_DYNAMIC_MAC,
    TABLE_MIB
};

#define LEARNED_MAC_TABLE_ENTRIES  1024
#define STATIC_MAC_TABLE_ENTRIES   8

typedef struct {
    UCHAR  MacAddr[ MAC_ADDRESS_LENGTH ];
    USHORT wVID;
    UCHAR  bFID;
    UCHAR  bPorts;
    UCHAR  fOverride : 1;
    UCHAR  fUseFID : 1;
    UCHAR  fValid : 1;
} MAC_TABLE, *PMAC_TABLE;



#define VLAN_TABLE_ENTRIES  16

typedef struct {
    USHORT wVID;
    UCHAR  bFID;
    UCHAR  bMember;
} VLAN_TABLE, *PVLAN_TABLE;


#define SWITCH_PORT_NUM         2
#define TOTAL_PORT_NUM          ( SWITCH_PORT_NUM + 1 )

#define PORT_COUNTER_NUM        0x20
#define TOTAL_PORT_COUNTER_NUM  ( PORT_COUNTER_NUM + 2 )

#define MIB_COUNTER_RX_LO_PRIORITY       0x00
#define MIB_COUNTER_RX_HI_PRIORITY       0x01
#define MIB_COUNTER_RX_UNDERSIZE         0x02
#define MIB_COUNTER_RX_FRAGMENT          0x03
#define MIB_COUNTER_RX_OVERSIZE          0x04
#define MIB_COUNTER_RX_JABBER            0x05
#define MIB_COUNTER_RX_SYMBOL_ERR        0x06
#define MIB_COUNTER_RX_CRC_ERR           0x07
#define MIB_COUNTER_RX_ALIGNMENT_ERR     0x08
#define MIB_COUNTER_RX_CTRL_8808         0x09
#define MIB_COUNTER_RX_PAUSE             0x0A
#define MIB_COUNTER_RX_BROADCAST         0x0B
#define MIB_COUNTER_RX_MULTICAST         0x0C
#define MIB_COUNTER_RX_UNICAST           0x0D
#define MIB_COUNTER_RX_OCTET_64          0x0E
#define MIB_COUNTER_RX_OCTET_65_127      0x0F
#define MIB_COUNTER_RX_OCTET_128_255     0x10
#define MIB_COUNTER_RX_OCTET_256_511     0x11
#define MIB_COUNTER_RX_OCTET_512_1023    0x12
#define MIB_COUNTER_RX_OCTET_1024_1522   0x13
#define MIB_COUNTER_TX_LO_PRIORITY       0x14
#define MIB_COUNTER_TX_HI_PRIORITY       0x15
#define MIB_COUNTER_TX_LATE_COLLISION    0x16
#define MIB_COUNTER_TX_PAUSE             0x17
#define MIB_COUNTER_TX_BROADCAST         0x18
#define MIB_COUNTER_TX_MULTICAST         0x19
#define MIB_COUNTER_TX_UNICAST           0x1A
#define MIB_COUNTER_TX_DEFERRED          0x1B
#define MIB_COUNTER_TX_TOTAL_COLLISION   0x1C
#define MIB_COUNTER_TX_EXCESS_COLLISION  0x1D
#define MIB_COUNTER_TX_SINGLE_COLLISION  0x1E
#define MIB_COUNTER_TX_MULTI_COLLISION   0x1F

#define MIB_COUNTER_RX_DROPPED_PACKET    0x20
#define MIB_COUNTER_TX_DROPPED_PACKET    0x21


#define MIB_TABLE_ENTRIES  (TOTAL_PORT_COUNTER_NUM * 3 )

typedef struct {
    USHORT    wVID;
    UCHAR     bCurrentCounter;
    UCHAR     bMember;
    ULONG     dwRxRate[ 4 ];
    ULONG     dwTxRate[ 4 ];
    UCHAR     bPortPriority;
    UCHAR     bReserved1[ 3 ];

    ULONGLONG cnCounter[ TOTAL_PORT_COUNTER_NUM ];
    ULONG     cnDropped[ 2 ];
} PORT_CONFIG, *PPORT_CONFIG;


typedef struct {
    ULONG  ulHardwareState;
    ULONG  ulSpeed;
    UCHAR  bDuplex;
    UCHAR  bLinkPartner;
    UCHAR  bAdvertised;
    UCHAR  bReserved1;
    int    nSTP_State;
} PORT_INFO, *PPORT_INFO;


/* -------------------------------------------------------------------------- */

/* define max switch port */
#ifdef DEF_KS8842
#define MAX_SWITCH_PORT   2
#else
#define MAX_SWITCH_PORT   1
#endif

#define MAX_ETHERNET_BODY_SIZE  1500
#define ETHERNET_HEADER_SIZE    14

#define MAXIMUM_ETHERNET_PACKET_SIZE  \
    ( MAX_ETHERNET_BODY_SIZE + ETHERNET_HEADER_SIZE )

#define MAX_BUF_SIZE            2048

#define TX_BUF_SIZE             2000
#define RX_BUF_SIZE             2000

#define NDIS_MAX_LOOKAHEAD      ( RX_BUF_SIZE - ETHERNET_HEADER_SIZE )

#define MAX_MULTICAST_LIST      32

#define HW_MULTICAST_SIZE       8


#define MAC_ADDR_ORDER( i )  ( MAC_ADDRESS_LENGTH - 1 - ( i ))


#define MAIN_PORT   0
#define OTHER_PORT  1
#define HOST_PORT   2

#define PORT_1      1
#define PORT_2      2

#define DEV_TO_HW_PORT( port )  ( port + 1 )
#define HW_TO_DEV_PORT( port )  ( port - 1 )


typedef enum
{
    MediaStateConnected,
    MediaStateDisconnected
} MEDIA_STATE;


typedef enum
{
    OID_COUNTER_UNKOWN,

    OID_COUNTER_FIRST,
    OID_COUNTER_DIRECTED_BYTES_XMIT = OID_COUNTER_FIRST, /* total bytes transmitted  */
    OID_COUNTER_DIRECTED_FRAMES_XMIT,    /* total packets transmitted */

    OID_COUNTER_BROADCAST_BYTES_XMIT,
    OID_COUNTER_BROADCAST_FRAME_XMIT,

    OID_COUNTER_DIRECTED_BYTES_RCV,      /* total bytes received   */
    OID_COUNTER_DIRECTED_FRAMES_RCV,     /* total packets received */
    OID_COUNTER_BROADCAST_BYTES_RCV,
    OID_COUNTER_BROADCAST_FRAMES_RCV,    /* total broadcast packets received (RXSR: RXBF)                */
    OID_COUNTER_MULTICAST_FRAMES_RCV,    /* total multicast packets received (RXSR: RXMF) or (RDSE0: MF) */
    OID_COUNTER_UNICAST_FRAMES_RCV,      /* total unicast packets received   (RXSR: RXUF)                */

    OID_COUNTER_XMIT_ERROR,              /* total transmit errors */
    OID_COUNTER_XMIT_LATE_COLLISION,     /* transmit Late Collision (TXSR: TXLC) */
    OID_COUNTER_XMIT_MORE_COLLISIONS,    /* transmit Maximum Collision (TXSR: TXMC) */
    OID_COUNTER_XMIT_UNDERRUN,           /* transmit Underrun (TXSR: TXUR) */
    OID_COUNTER_XMIT_ALLOC_FAIL,         /* transmit fail because no enought memory in the Tx Packet Memory */
    OID_COUNTER_XMIT_DROPPED,            /* transmit packet drop because no buffer in the host memory */
    OID_COUNTER_XMIT_INT_UNDERRUN,       /* transmit underrun from interrupt status (ISR: TXUIS) */
    OID_COUNTER_XMIT_INT_STOP,           /* transmit DMA MAC process stop from interrupt status (ISR: TXPSIE) */
    OID_COUNTER_XMIT_INT,                /* transmit Tx interrupt status (ISR: TXIE) */

    OID_COUNTER_RCV_ERROR,               /* total receive errors */
    OID_COUNTER_RCV_ERROR_CRC,           /* receive packet with CRC error (RXSR: RXCE) or (RDSE0: CE) */
    OID_COUNTER_RCV_ERROR_MII,           /* receive MII error (RXSR: RXMR) or (RDSE0: RE) */
    OID_COUNTER_RCV_ERROR_TOOLONG,       /* receive frame too long error (RXSR: RXTL) or (RDSE0: TL)  */
    OID_COUNTER_RCV_ERROR_RUNT,          /* receive Runt frame error (RXSR: RXRF) or (RDSE0: RF)  */
    OID_COUNTER_RCV_INVALID_FRAME,       /* receive invalid frame (RXSR: RXFV) */
    OID_COUNTER_RCV_ERROR_IP,            /* receive frame with IP checksum error  (RDSE0: IPE) */
    OID_COUNTER_RCV_ERROR_TCP,           /* receive frame with TCP checksum error (RDSE0: TCPE) */
    OID_COUNTER_RCV_ERROR_UDP,           /* receive frame with UDP checksum error (RDSE0: UDPE) */
    OID_COUNTER_RCV_NO_BUFFER,           /* receive failed on memory allocation for the incoming frames from interrupt status (ISR: RXOIS). */
    OID_COUNTER_RCV_DROPPED,             /* receive packet drop because no buffer in the host memory */
    OID_COUNTER_RCV_INT_ERROR,           /* receive error from interrupt status (ISR: RXEFIE) */
    OID_COUNTER_RCV_INT_STOP,            /* receive DMA MAC process stop from interrupt status (ISR: RXPSIE) */
    OID_COUNTER_RCV_INT,                 /* receive Rx interrupt status (ISR: RXIE) */

    OID_COUNTER_XMIT_OK,
    OID_COUNTER_RCV_OK,

    OID_COUNTER_RCV_ERROR_LEN,

    OID_COUNTER_LAST
} EOidCounter;


enum
{
    COUNT_BAD_FIRST,
    COUNT_BAD_ALLOC = COUNT_BAD_FIRST,
    COUNT_BAD_CMD,
    COUNT_BAD_CMD_BUSY,
    COUNT_BAD_CMD_INITIALIZE,
    COUNT_BAD_CMD_MEM_ALLOC,
    COUNT_BAD_CMD_RESET,
    COUNT_BAD_CMD_WRONG_CHIP,
    COUNT_BAD_COPY_DOWN,
    COUNT_BAD_RCV_FRAME,
    COUNT_BAD_RCV_PACKET,
    COUNT_BAD_SEND,
    COUNT_BAD_SEND_DIFF,
    COUNT_BAD_SEND_PACKET,
    COUNT_BAD_SEND_ZERO,
    COUNT_BAD_XFER_ZERO,
    COUNT_BAD_LAST
};


enum
{
    COUNT_GOOD_FIRST,
    COUNT_GOOD_CMD_RESET = COUNT_GOOD_FIRST,
    COUNT_GOOD_CMD_RESET_MMU,
    COUNT_GOOD_COPY_DOWN_ODD,
    COUNT_GOOD_INT,
    COUNT_GOOD_INT_LOOP,
    COUNT_GOOD_INT_ALLOC,
    COUNT_GOOD_INT_RX,
    COUNT_GOOD_INT_RX_EARLY,
    COUNT_GOOD_INT_RX_OVERRUN,
    COUNT_GOOD_INT_TX,
    COUNT_GOOD_INT_TX_EMPTY,
    COUNT_GOOD_NEXT_PACKET,
    COUNT_GOOD_NO_NEXT_PACKET,
    COUNT_GOOD_RCV_COMPLETE,
    COUNT_GOOD_RCV_DISCARD,
    COUNT_GOOD_RCV_NOT_DISCARD,
    COUNT_GOOD_SEND_PACKET,
    COUNT_GOOD_SEND_QUEUE,
    COUNT_GOOD_SEND_ZERO,
    COUNT_GOOD_XFER_ZERO,
    COUNT_GOOD_LAST
};


enum
{
    WAIT_DELAY_FIRST,
    WAIT_DELAY_PHY_RESET = WAIT_DELAY_FIRST,
    WAIT_DELAY_AUTO_NEG,
    WAIT_DELAY_MEM_ALLOC,
    WAIT_DELAY_CMD_BUSY,
    WAIT_DELAY_LAST
};


#if 0
#define RCV_HUGE_FRAME
#endif

#ifdef KS_PCI_BUS
#if 0
#define CHECK_RCV_ERRORS
#endif
#if 1
#define SKIP_TX_INT
#endif

#if 0
#define CHECK_OVERRUN
#endif
#ifdef DBG
#if 1
#define DEBUG_OVERRUN
#endif
#endif


#define DESC_ALIGNMENT              16
#define BUFFER_ALIGNMENT            8


#define NUM_OF_RX_DESC  128
#define NUM_OF_TX_DESC  64

#define DESC_RX_FRAME_LEN        0x000007FF
#define DESC_RX_FRAME_TYPE       0x00008000
#define DESC_RX_ERROR_CRC        0x00010000
#define DESC_RX_ERROR_RUNT       0x00020000
#define DESC_RX_ERROR_TOO_LONG   0x00040000
#define DESC_RX_ERROR_PHY        0x00080000
#define DESC_RX_PORT_MASK        0x00300000
#define DESC_RX_MULTICAST        0x01000000
#define DESC_RX_ERROR            0x02000000
#define DESC_RX_ERROR_CSUM_UDP   0x04000000
#define DESC_RX_ERROR_CSUM_TCP   0x08000000
#define DESC_RX_ERROR_CSUM_IP    0x10000000
#define DESC_RX_LAST             0x20000000
#define DESC_RX_FIRST            0x40000000

#define DESC_HW_OWNED            0x80000000

#define DESC_BUF_SIZE            0x000007FF
#define DESC_TX_PORT_MASK        0x00300000
#define DESC_END_OF_RING         0x02000000
#define DESC_TX_CSUM_GEN_UDP     0x04000000
#define DESC_TX_CSUM_GEN_TCP     0x08000000
#define DESC_TX_CSUM_GEN_IP      0x10000000
#define DESC_TX_LAST             0x20000000
#define DESC_TX_FIRST            0x40000000
#define DESC_TX_INTERRUPT        0x80000000

#define DESC_RX_MASK  ( DESC_BUF_SIZE )

#define DESC_TX_MASK  ( DESC_TX_INTERRUPT | DESC_TX_FIRST | DESC_TX_LAST | \
    DESC_BUF_SIZE )


typedef struct
{
    ULONG wFrameLen     : 11;
    ULONG ulReserved1   : 4;
    ULONG fFrameType    : 1;
    ULONG fErrCRC       : 1;
    ULONG fErrRunt      : 1;
    ULONG fErrTooLong   : 1;
    ULONG fErrPHY       : 1;
    ULONG ulSourePort   : 4;
    ULONG fMulticast    : 1;
    ULONG fError        : 1;
    ULONG fCsumErrUDP   : 1;
    ULONG fCsumErrTCP   : 1;
    ULONG fCsumErrIP    : 1;
    ULONG fLastDesc     : 1;
    ULONG fFirstDesc    : 1;
    ULONG fHWOwned      : 1;
} TDescRxStat;

typedef struct
{
    ULONG ulReserved1   : 31;
    ULONG fHWOwned      : 1;
} TDescTxStat;

typedef struct
{
    ULONG wBufSize      : 11;
    ULONG ulReserved3   : 14;
    ULONG fEndOfRing    : 1;
    ULONG ulReserved4   : 6;
} TDescRxBuf;

typedef struct
{
    ULONG wBufSize      : 11;
    ULONG ulReserved3   : 9;
    ULONG ulDestPort    : 4;
    ULONG ulReserved4   : 1;
    ULONG fEndOfRing    : 1;
    ULONG fCsumGenUDP   : 1;
    ULONG fCsumGenTCP   : 1;
    ULONG fCsumGenIP    : 1;
    ULONG fLastSeg      : 1;
    ULONG fFirstSeg     : 1;
    ULONG fInterrupt    : 1;
} TDescTxBuf;

typedef union
{
    TDescRxStat rx;
    TDescTxStat tx;
    ULONG       ulData;
} TDescStat;

typedef union
{
    TDescRxBuf rx;
    TDescTxBuf tx;
    ULONG      ulData;
} TDescBuf;

typedef struct
{
    TDescStat Control;
    TDescBuf  BufSize;
    ULONG     ulBufAddr;
    ULONG     ulNextPtr;
} THw_Desc, *PTHw_Desc;


typedef struct
{
    TDescStat Control;
    TDescBuf  BufSize;

    /* Current buffers size value in hardware descriptor. */
    ULONG     ulBufSize;
} TSw_Desc, *PTSw_Desc;


typedef struct _Desc
{
    /* Hardware descriptor pointer to uncached physical memory. */
    PTHw_Desc     phw;

    /* Cached memory to hold hardware descriptor values for manipulation. */
    TSw_Desc      sw;

    /* Operating system dependent data structure to hold physical memory buffer
       allocation information.
    */
    PVOID         pReserved;

#ifdef CHECK_OVERRUN
    PTHw_Desc     pCheck;
#endif
} TDesc, *PTDesc;


typedef struct
{
    /* First descriptor in the ring. */
    PTDesc    pRing;

    /* Current descriptor being manipulated. */
    PTDesc    pCurrent;

    /* First hardware descriptor in the ring. */
    PTHw_Desc phwRing;

    /* The physical address of the first descriptor of the ring. */
    ULONG     ulRing;

    int       nSize;

    /* Number of descriptors allocated. */
    int       cnAlloc;

    /* Number of descriptors available for use. */
    int       cnAvail;

    /* Index for last descriptor released to hardware .*/
    int       iLast;

    /* Index for next descriptor available for use. */
    int       iNext;

    /* Mask for index wrapping. */
    int       iMax;
} TDescInfo, *PTDescInfo;
#endif


struct hw_fn;

typedef struct
{
    struct hw_fn*           m_hwfn;

    UCHAR                   m_bPermanentAddress[ MAC_ADDRESS_LENGTH ];

    UCHAR                   m_bOverrideAddress[ MAC_ADDRESS_LENGTH ];

    /* PHY status info. */
    ULONG                   m_ulHardwareState;
    ULONG                   m_ulTransmitRate;
    ULONG                   m_ulDuplex;

    /* hardware resources */
    PUCHAR                  m_pVirtualMemory;
    ULONG                   m_ulVIoAddr;             /* device's base address */
    ULONG                   m_boardBusEndianMode;    /* board bus endian mode board specific */

    UCHAR                   m_bMacOverrideAddr;

    UCHAR                   m_bBroadcastPercent;
    USHORT                  m_w802_1P_Mapping;
    USHORT                  m_wDiffServ[ 64 ];      /* possible values from 6-bit of ToS (bit7 ~ bit2) field */
    USHORT                  m_b802_1P_Priority[8];  /* possible values from 3-bit of 802.1p Tag priority field */
    MAC_TABLE               m_MacTable[ STATIC_MAC_TABLE_ENTRIES ];
    PORT_CONFIG             m_Port[ TOTAL_PORT_NUM ];        /* Device switch MIB counters */
    PORT_INFO               m_PortInfo[ SWITCH_PORT_NUM ];
    VLAN_TABLE              m_VlanTable[ VLAN_TABLE_ENTRIES ];

#ifdef KS_PCI_BUS
    ULONG                   m_dwTransmitConfig;
    ULONG                   m_dwReceiveConfig;
    USHORT                  m_wTransmitThreshold;
    USHORT                  m_wReceiveThreshold;
    ULONG                   m_ulInterruptMask;
    ULONG                   m_ulInterruptSet;
    UCHAR                   m_bReceiveStop;

#else
    USHORT                  m_wTransmitConfig;
    USHORT                  m_wTransmitThreshold;
    USHORT                  m_wReceiveConfig;
    USHORT                  m_wReceiveThreshold;
    USHORT                  m_wInterruptMask;
    UCHAR                   m_bBurstLength;
    UCHAR                   m_bReserved1;
#endif
    UCHAR                   m_bEnabled;
    UCHAR                   m_bPromiscuous;
    UCHAR                   m_bAllMulticast;

    /* List of multicast addresses in use. */

    UCHAR                   m_bMulticastListSize;
    UCHAR                   m_bMulticastList[ MAX_MULTICAST_LIST ]
        [ MAC_ADDRESS_LENGTH ];

    /* member variables used for receiving */
    int                     m_nPacketLen;

#ifdef KS_PCI_BUS
    PUCHAR                  m_bLookahead;

#else
    UCHAR                   m_bLookahead[ MAX_BUF_SIZE ];
#endif

    /* member variables used for sending commands, mostly for debug purpose */
    int                     m_nWaitDelay[ WAIT_DELAY_LAST ];

    /* member variables for statistics */
    ULONGLONG               m_cnCounter[ SWITCH_PORT_NUM ][ OID_COUNTER_LAST ];  /* Driver statistics counter */
    ULONG                   m_nBad[ COUNT_BAD_LAST ];
    ULONG                   m_nGood[ COUNT_GOOD_LAST ];

    UCHAR                   m_bBank;
    UCHAR                   m_bReceiveDiscard;
    UCHAR                   m_bSentPacket;
    UCHAR                   m_bTransmitPacket;

    /* hardware configurations read from the registry */
    UCHAR                   m_bDuplex;           /* 10: 10BT; 100: 100BT */
    UCHAR                   m_bSpeed;            /* 1: Full duplex; 2: half duplex */

    USHORT                  m_wPhyAddr;

    UCHAR                   m_bMulticastBits[ HW_MULTICAST_SIZE ];

    UCHAR                   f_dircetMode; /* 1: Tx by direct mode, 0:Tx by loopkup mode */
    UCHAR                   m_bReserved2[ 3 ];

    UCHAR                   m_bPort;
    UCHAR                   m_bPortAlloc;
    UCHAR                   m_bPortRX;    /* 1:Rx from Port1; 2:Rx from Port2 */
    UCHAR                   m_bPortTX;    /* 1:Tx to Port1; 2:Tx to Port2; 3:Tx to Port1 and Port2; 0:Tx by loopkup mode */
    UCHAR                   m_fPortTX;
    UCHAR                   m_bStarted;

    UCHAR                   m_bAcquire;
    UCHAR                   m_bPortSelect;

    /* member variables used for saving registers during interrupt */
    UCHAR                   m_bSavedBank;
    UCHAR                   m_bSavedPacket;
    USHORT                  m_wSavedPointer;

#ifdef KS_PCI_BUS
    TDescInfo               m_RxDescInfo;
    TDescInfo               m_TxDescInfo;

#ifdef SKIP_TX_INT
    int                     m_TxIntCnt;
    int                     m_TxIntMask;
#endif

    void*                   m_pPciCfg;

#ifdef DEBUG_OVERRUN
    ULONG                   m_ulDropped;
    ULONG                   m_ulReceived;
#endif
#endif
    void*                   m_pDevice;

#ifdef KS_ISA_BUS
#ifdef UNDER_CE
    UCHAR                   reg[ 54 ][ 16 ];
#endif
#endif
    /* for debug hardware transmit\receive packets */
    UCHAR                   fDebugDumpTx;    /* Dump transmit packets to Consult port */
    UCHAR                   fDebugDumpRx;    /* Dump received packets to Consult port */
    UCHAR                   fLoopbackStart;  /* loopback the received packets to trasnmit. */
    UCHAR                   m_bReserved3;
} HARDWARE, *PHARDWARE;


struct hw_fn {
    int m_fPCI;

    void ( *fnSwitchDisableMirrorSniffer )( PHARDWARE, UCHAR );
    void ( *fnSwitchEnableMirrorSniffer )( PHARDWARE, UCHAR );
    void ( *fnSwitchDisableMirrorReceive )( PHARDWARE, UCHAR );
    void ( *fnSwitchEnableMirrorReceive )( PHARDWARE, UCHAR );
    void ( *fnSwitchDisableMirrorTransmit )( PHARDWARE, UCHAR );
    void ( *fnSwitchEnableMirrorTransmit )( PHARDWARE, UCHAR );
    void ( *fnSwitchDisableMirrorRxAndTx )( PHARDWARE );
    void ( *fnSwitchEnableMirrorRxAndTx )( PHARDWARE );

    void ( *fnHardwareConfig_TOS_Priority )( PHARDWARE, UCHAR, USHORT );
    void ( *fnSwitchDisableDiffServ )( PHARDWARE, UCHAR );
    void ( *fnSwitchEnableDiffServ )( PHARDWARE, UCHAR );

    void ( *fnHardwareConfig802_1P_Priority )( PHARDWARE, UCHAR, USHORT );
    void ( *fnSwitchDisable802_1P )( PHARDWARE, UCHAR );
    void ( *fnSwitchEnable802_1P )( PHARDWARE, UCHAR );
    void ( *fnSwitchDisableDot1pRemapping )( PHARDWARE, UCHAR );
    void ( *fnSwitchEnableDot1pRemapping )( PHARDWARE, UCHAR );

    void ( *fnSwitchConfigPortBased )( PHARDWARE, UCHAR, UCHAR );

    void ( *fnSwitchDisableMultiQueue )( PHARDWARE, UCHAR );
    void ( *fnSwitchEnableMultiQueue )( PHARDWARE, UCHAR );

    void ( *fnSwitchDisableBroadcastStorm )( PHARDWARE, UCHAR );
    void ( *fnSwitchEnableBroadcastStorm )( PHARDWARE, UCHAR );
    void ( *fnHardwareConfigBroadcastStorm )( PHARDWARE, UCHAR );

    void ( *fnSwitchDisablePriorityRate )( PHARDWARE, UCHAR );
    void ( *fnSwitchEnablePriorityRate )( PHARDWARE, UCHAR );

    void ( *fnHardwareConfigRxPriorityRate )( PHARDWARE, UCHAR, UCHAR,
        ULONG );
    void ( *fnHardwareConfigTxPriorityRate )( PHARDWARE, UCHAR, UCHAR,
        ULONG );

    void ( *fnPortSet_STP_State )( PHARDWARE, UCHAR, int );

    void ( *fnPortReadMIBCounter )( PHARDWARE, UCHAR, USHORT, PULONGLONG );
    void ( *fnPortReadMIBPacket )( PHARDWARE, UCHAR, PULONG, PULONGLONG );

    void ( *fnSwitchEnableVlan )( PHARDWARE );
};


#define BASE_IO_RANGE           0x10

#if 1
#define AUTO_RELEASE
#endif
#if 1
#define AUTO_FAST_AGING
#endif
#if 0
#define EARLY_RECEIVE
#endif
#if 0
#define EARLY_TRANSMIT
#endif


/* Bank select register offset is accessible in all banks to allow bank
   selection.
*/

#define REG_BANK_SEL_OFFSET     0x0E

/* -------------------------------------------------------------------------- */

/*
    KS8841\KS8842 register definitions
*/


#define SW_PHY_AUTO             0         /* autosense */
#define SW_PHY_10BASE_T         1         /* 10Base-T */
#define SW_PHY_10BASE_T_FD      2         /* 10Base-T Full Duplex */
#define SW_PHY_100BASE_TX       3         /* 100Base-TX */
#define SW_PHY_100BASE_TX_FD    4         /* 100Base-TX Full Duplex */

/* Default setting definitions */

#define KS8695_MIN_FBUF         (1536)    /* min data buffer size */
#define BUFFER_1568             1568      /* 0x620 */
#define BUFFER_2044             2044      /* 2K-4 buffer to meet Huge packet support (1916 bytes)
                                             (max buffer length that ks884x allowed */

#define RXCHECKSUM_DEFAULT      TRUE      /* HW Rx IP/TCP/UDP checksum enable */
#define TXCHECKSUM_DEFAULT      TRUE      /* HW Tx IP/TCP/UDP checksum enable */
#define FLOWCONTROL_DEFAULT     TRUE      /* Flow control enable */
#define PBL_DEFAULT             8         /* DMA Tx/Rx burst Size. 0:unlimited, other value for (4 * x) */

#define PHY_POWERDOWN_DEFAULT   TRUE      /* PHY PowerDown Reset enable */
#define PHY_SPEED_DEFAULT       SW_PHY_AUTO /* PHY auto-negotiation enable */

#define PORT_STP_DEFAULT        FALSE     /* Spanning tree disable */
#define PORT_STORM_DEFAULT      TRUE      /* Broadcast storm protection enable */

/* OBCR */
#define BUS_SPEED_125_MHZ       0x0000
#define BUS_SPEED_62_5_MHZ      0x0001
#define BUS_SPEED_41_66_MHZ     0x0002
#define BUS_SPEED_25_MHZ        0x0003

/* EEPCR */
#define EEPROM_CHIP_SELECT      0x0001
#define EEPROM_SERIAL_CLOCK     0x0002
#define EEPROM_DATA_OUT         0x0004
#define EEPROM_DATA_IN          0x0008
#define EEPROM_ACCESS_ENABLE    0x0010

/* MBIR */
#define RX_MEM_TEST_FAILED      0x0008
#define RX_MEM_TEST_FINISHED    0x0010
#define TX_MEM_TEST_FAILED      0x0800
#define TX_MEM_TEST_FINISHED    0x1000

/* PMCS */
#define POWER_STATE_D0          0x0000
#define POWER_STATE_D1          0x0001
#define POWER_STATE_D2          0x0002
#define POWER_STATE_D3          0x0003
#define POWER_STATE_MASK        0x0003
#define POWER_PME_ENABLE        0x0100
#define POWER_PME_STATUS        0x8000

/* WFCR */
#define WOL_MAGIC_ENABLE        0x0080
#define WOL_FRAME3_ENABLE       0x0008
#define WOL_FRAME2_ENABLE       0x0004
#define WOL_FRAME1_ENABLE       0x0002
#define WOL_FRAME0_ENABLE       0x0001


/*
 * KS8841/KS8842 interface to Host by ISA bus.
 */

#ifdef KS_ISA_BUS

/* Bank 0 */

/* BAR */
#define REG_BASE_ADDR_BANK      0
#define REG_BASE_ADDR_OFFSET    0x00

/* BDAR */
#define REG_RX_WATERMARK_BANK   0
#define REG_RX_WATERMARK_OFFSET 0x04

#define RX_HIGH_WATERMARK_2KB   0x1000

/* BESR */
#define REG_BUS_ERROR_BANK      0
#define REG_BUS_ERROR_OFFSET    0x06

/* BBLR */
#define REG_BUS_BURST_BANK      0
#define REG_BUS_BURST_OFFSET    0x08

#define BURST_LENGTH_0          0x0000
#define BURST_LENGTH_4          0x3000
#define BURST_LENGTH_8          0x5000
#define BURST_LENGTH_16         0x7000

/* Bank 2 */

#define REG_ADDR_0_BANK         2
/* MARL */
#define REG_ADDR_0_OFFSET       0x00
#define REG_ADDR_1_OFFSET       0x01
/* MARM */
#define REG_ADDR_2_OFFSET       0x02
#define REG_ADDR_3_OFFSET       0x03
/* MARH */
#define REG_ADDR_4_OFFSET       0x04
#define REG_ADDR_5_OFFSET       0x05


/* Bank 3 */

/* OBCR */
#define REG_BUS_CTRL_BANK       3
#define REG_BUS_CTRL_OFFSET     0x00

/* EEPCR */
#define REG_EEPROM_CTRL_BANK    3
#define REG_EEPROM_CTRL_OFFSET  0x02

/* MBIR */
#define REG_MEM_INFO_BANK       3
#define REG_MEM_INFO_OFFSET     0x04

/* GCR */
#define REG_GLOBAL_CTRL_BANK    3
#define REG_GLOBAL_CTRL_OFFSET  0x06

/* WFCR */
#define REG_WOL_CTRL_BANK       3
#define REG_WOL_CTRL_OFFSET     0x0A

/* WF0 */
#define REG_WOL_FRAME_0_BANK    4
#define WOL_FRAME_CRC_OFFSET    0x00
#define WOL_FRAME_BYTE0_OFFSET  0x04
#define WOL_FRAME_BYTE2_OFFSET  0x08

/* WF1 */
#define REG_WOL_FRAME_1_BANK    5

/* WF2 */
#define REG_WOL_FRAME_2_BANK    6

/* WF3 */
#define REG_WOL_FRAME_3_BANK    7


/* Bank 16 */

/* TXCR */
#define REG_TX_CTRL_BANK        16
#define REG_TX_CTRL_OFFSET      0x00

#define TX_CTRL_ENABLE          0x0001
#define TX_CTRL_CRC_ENABLE      0x0002
#define TX_CTRL_PAD_ENABLE      0x0004
#define TX_CTRL_FLOW_ENABLE     0x0008
#define TX_CTRL_MAC_LOOPBACK    0x2000

/* TXSR */
#define REG_TX_STATUS_BANK      16
#define REG_TX_STATUS_OFFSET    0x02

#define TX_FRAME_ID_MASK        0x003F
#define TX_STAT_MAX_COL         0x1000
#define TX_STAT_LATE_COL        0x2000
#define TX_STAT_UNDERRUN        0x4000
#define TX_STAT_COMPLETE        0x8000

#ifdef EARLY_TRANSMIT
#define TX_STAT_ERRORS          ( TX_STAT_MAX_COL | TX_STAT_LATE_COL | TX_STAT_UNDERRUN )
#else
#define TX_STAT_ERRORS          ( TX_STAT_MAX_COL | TX_STAT_LATE_COL )
#endif

#define TX_CTRL_DEST_PORTS      0x0F00
#define TX_CTRL_INTERRUPT_ON    0x8000

#define TX_DEST_PORTS_SHIFT     8

#define TX_FRAME_ID_MAX         (( (TX_FRAME_ID_MASK + 1) / 2 ) - 1 )
#define TX_FRAME_ID_PORT_SHIFT  5

/* RXCR */
#define REG_RX_CTRL_BANK        16
#define REG_RX_CTRL_OFFSET      0x04

#define RX_CTRL_ENABLE          0x0001
#define RX_CTRL_MULTICAST       0x0004
#define RX_CTRL_STRIP_CRC       0x0008
#define RX_CTRL_PROMISCUOUS     0x0010
#define RX_CTRL_UNICAST         0x0020
#define RX_CTRL_ALL_MULTICAST   0x0040
#define RX_CTRL_BROADCAST       0x0080
#define RX_CTRL_BAD_PACKET      0x0200
#define RX_CTRL_FLOW_ENABLE     0x0400

/* TXMIR */
#define REG_TX_MEM_INFO_BANK    16
#define REG_TX_MEM_INFO_OFFSET  0x08

/* RXMIR */
#define REG_RX_MEM_INFO_BANK    16
#define REG_RX_MEM_INFO_OFFSET  0x0A

#define MEM_AVAILABLE_MASK      0x1FFF


/* Bank 17 */

/* TXQCR */
#define REG_TXQ_CMD_BANK        17
#define REG_TXQ_CMD_OFFSET      0x00

#define TXQ_CMD_ENQUEUE_PACKET  0x0001

/* RXQCR */
#define REG_RXQ_CMD_BANK        17
#define REG_RXQ_CMD_OFFSET      0x02

#define RXQ_CMD_FREE_PACKET     0x0001

/* TXFDPR */
#define REG_TX_ADDR_PTR_BANK    17
#define REG_TX_ADDR_PTR_OFFSET  0x04

/* RXFDPR */
#define REG_RX_ADDR_PTR_BANK    17
#define REG_RX_ADDR_PTR_OFFSET  0x06

#define ADDR_PTR_MASK           0x07FF
#define ADDR_PTR_AUTO_INC       0x4000

#define REG_DATA_BANK           17
/* QDRL */
#define REG_DATA_OFFSET         0x08
/* QDRH */
#define REG_DATA_HI_OFFSET      0x0A


/* Bank 18 */

/* IER */
#define REG_INT_MASK_BANK       18
#define REG_INT_MASK_OFFSET     0x00

#define INT_RX_ERROR            0x0080
#define INT_RX_STOPPED          0x0100
#define INT_TX_STOPPED          0x0200
#define INT_RX_EARLY            0x0400
#define INT_RX_OVERRUN          0x0800
#define INT_TX_UNDERRUN         0x1000
#define INT_RX                  0x2000
#define INT_TX                  0x4000
#define INT_PHY                 0x8000
#define INT_MASK                ( INT_RX | INT_TX )


/* ISR */
#define REG_INT_STATUS_BANK     18
#define REG_INT_STATUS_OFFSET   0x02

#ifdef SH_32BIT_ACCESS_ONLY
#define INT_STATUS( intr )  (( intr ) << 16 )
#endif

/* RXSR */
#define REG_RX_STATUS_BANK      18
#define REG_RX_STATUS_OFFSET    0x04

#define RX_BAD_CRC              0x0001
#define RX_TOO_SHORT            0x0002
#define RX_TOO_LONG             0x0004
#define RX_FRAME_ETHER          0x0008
#define RX_PHY_ERROR            0x0010
#define RX_UNICAST              0x0020
#define RX_MULTICAST            0x0040
#define RX_BROADCAST            0x0080
#define RX_SRC_PORTS            0x0F00
#define RX_VALID                0x8000
#define RX_ERRORS     ( RX_BAD_CRC | RX_TOO_LONG | RX_TOO_SHORT | RX_PHY_ERROR )


#define RX_SRC_PORTS_SHIFT      8

/* RXBC */
#define REG_RX_BYTE_CNT_BANK    18
#define REG_RX_BYTE_CNT_OFFSET  0x06

#define RX_BYTE_CNT_MASK        0x07FF

/* ETXR */
#define REG_EARLY_TX_BANK       18
#define REG_EARLY_TX_OFFSET     0x08

#define EARLY_TX_THRESHOLD      0x001F
#define EARLY_TX_ENABLE         0x0080
#define EARLY_TX_MULTIPLE       64

/* ERXR */
#define REG_EARLY_RX_BANK       18
#define REG_EARLY_RX_OFFSET     0x0A

#define EARLY_RX_THRESHOLD      0x001F
#define EARLY_RX_ENABLE         0x0080
#define EARLY_RX_MULTIPLE       64


/* Bank 19 */

#define REG_MULTICAST_BANK      19
/* MTR0 */
#define REG_MULTICAST_0_OFFSET  0x00
#define REG_MULTICAST_1_OFFSET  0x01
/* MTR1 */
#define REG_MULTICAST_2_OFFSET  0x02
#define REG_MULTICAST_3_OFFSET  0x03
/* MTR2 */
#define REG_MULTICAST_4_OFFSET  0x04
#define REG_MULTICAST_5_OFFSET  0x05
/* MTR3 */
#define REG_MULTICAST_6_OFFSET  0x06
#define REG_MULTICAST_7_OFFSET  0x07


#define REG_POWER_CNTL_BANK     19
/* PMCS */
#define REG_POWER_CNTL_OFFSET   0x08


#endif /* ifdef KS_ISA_BUS */

/*
 * KS8841/KS8842 interface to Host by PCI bus.
 */

#ifdef KS_PCI_BUS

/*
 * PCI Configuration ( Space ) Registers
 *
 */

#define CFID                    0x00               /* Configuration ID Register */
#define   CFID_DEVID              0xFFFF0000           /* vendor id, 2 bytes */
#define   CFID_VENID              0x0000FFFF           /* device id, 2 bytes */

#define CFCS                    0x04               /* Command and Status Configuration Register */
#define   CFCS_STAT               0xFFFF0000           /* status register, 2 bytes */
#define   CFCS_COMM               0x0000FFFF           /* command register, 2 bytes */
#define   CFCS_COMM_MEM           0x00000002           /* memory access enable */
#define   CFCS_COMM_MASTER        0x00000004           /* master enable */
#define   CFCS_COMM_PERRSP        0x00000040           /* parity error response */
#define   CFCS_COMM_SYSERREN      0x00000100           /* system error enable */
#define   COMM_SETTING           (CFCS_COMM_MEM | CFCS_COMM_MASTER | CFCS_COMM_PERRSP | CFCS_COMM_SYSERREN)

#define   CFCS_STAT_DPR           0x0100               /* Data Parity Error */
#define   CFCS_STAT_DST           0x0600               /* Device Select Timing */
#define   CFCS_STAT_RVTAB         0x1000               /* Received Target Abort */
#define   CFCS_STAT_RVMAB         0x2000               /* Received Master Abort */
#define   CFCS_STAT_SYSERR        0x4000               /* Signal System Error */
#define   CFCS_STAT_DPERR         0x8000               /* Detected Parity Error */


#define CFRV                    0x08               /* Configuration Revision Register */
#define   CFRV_BASCLASS           0xFF000000           /* basic code, 1 byte */
#define   CFRV_SUBCLASS           0x00FF0000           /* sub-class code, 1 byte */
#define   CFRV_REVID              0x000000FF           /* revision id (Revision\Step number), 1 byte */

#define CFLT                    0x0C               /* Configuration Latency Timer Register */
#define   CFLT_LATENCY_TIMER      0x0000FF00           /* latency timer, 1 byte */
#define   CFLT_CACHE_LINESZ       0x000000FF           /* cache line size, 1 byte */
#define   LATENCY_TIMER           0x00000080           /* default latency timer - 0 */
#define   CACHE_LINESZ                     8           /* default cache line size - 8 (8-DWORD) */

#define CMBA                    0x10               /* Configuration Memory Base Address Register */

#define CSID                    0x2C               /* Subsystem ID Register */
#define   CSID_SUBSYSID           0xFFFF0000           /* Subsystem ID, 2 bytes */
#define   CSID_SUBVENID           0x0000FFFF           /* Subsystem Vendor ID, 2 bytes*/

#define CFIT                    0x3C               /* Configuration Interrupt Register */
#define   CFIT_MAX_L              0xFF000000           /* maximum latency, 1 byte */
#define   CFIT_MIN_G              0x00FF0000           /* minimum grant, 1 byte */
#define   CFIT_IPIN               0x0000FF00           /* interrupt pin, 1 byte */
#define   CFIT_ILINE              0x000000FF           /* interrupt line, 1 byte */
#define   MAX_LATENCY                   0x28           /* default maximum latency - 0x28 */
#define   MIN_GRANT                     0x14           /* default minimum grant - 0x14 */

#define CPMC                    0x54               /* Power Management Control and Status Register */


/* DMA Registers */

#define REG_DMA_TX_CTRL             0x0000
#define DMA_TX_CTRL_ENABLE          0x00000001
#define DMA_TX_CTRL_CRC_ENABLE      0x00000002
#define DMA_TX_CTRL_PAD_ENABLE      0x00000004
#define DMA_TX_CTRL_LOOPBACK        0x00000100
#define DMA_TX_CTRL_FLOW_ENABLE     0x00000200
#define DMA_TX_CTRL_CSUM_IP         0x00010000
#define DMA_TX_CTRL_CSUM_TCP        0x00020000
#define DMA_TX_CTRL_CSUM_UDP        0x00040000
#define DMA_TX_CTRL_BURST_SIZE      0x3F000000

#define REG_DMA_RX_CTRL             0x0004
#define DMA_RX_CTRL_ENABLE          0x00000001
#define DMA_RX_CTRL_MULTICAST       0x00000002
#define DMA_RX_CTRL_PROMISCUOUS     0x00000004
#define DMA_RX_CTRL_ERROR           0x00000008
#define DMA_RX_CTRL_UNICAST         0x00000010
#define DMA_RX_CTRL_ALL_MULTICAST   0x00000020
#define DMA_RX_CTRL_BROADCAST       0x00000040
#define DMA_RX_CTRL_FLOW_ENABLE     0x00000200
#define DMA_RX_CTRL_CSUM_IP         0x00010000
#define DMA_RX_CTRL_CSUM_TCP        0x00020000
#define DMA_RX_CTRL_CSUM_UDP        0x00040000
#define DMA_RX_CTRL_BURST_SIZE      0x3F000000

#define REG_DMA_TX_START            0x0008
#define REG_DMA_RX_START            0x000C
#define DMA_START                   0x00000001      /* DMA start command */

#define REG_DMA_TX_ADDR             0x0010
#define REG_DMA_RX_ADDR             0x0014

#define DMA_ADDR_LIST_MASK          0xFFFFFFFC
#define DMA_ADDR_LIST_SHIFT         2

/* MTR0 */
#define REG_MULTICAST_0_OFFSET      0x0020
#define REG_MULTICAST_1_OFFSET      0x0021
#define REG_MULTICAST_2_OFFSET      0x0022
#define REG_MULTICAST_3_OFFSET      0x0023
/* MTR1 */
#define REG_MULTICAST_4_OFFSET      0x0024
#define REG_MULTICAST_5_OFFSET      0x0025
#define REG_MULTICAST_6_OFFSET      0x0026
#define REG_MULTICAST_7_OFFSET      0x0027

/* Interrupt Registers */

/* INTEN */
#define REG_INTERRUPTS_ENABLE       0x0028
/* INTST */
#define REG_INTERRUPTS_STATUS       0x002C

#define INT_WAN_RX_STOPPED          0x02000000
#define INT_WAN_TX_STOPPED          0x04000000
#define INT_WAN_RX_BUF_UNAVAIL      0x08000000
#define INT_WAN_TX_BUF_UNAVAIL      0x10000000
#define INT_WAN_RX                  0x20000000
#define INT_WAN_TX                  0x40000000
#define INT_WAN_PHY                 0x80000000

#define INT_RX_STOPPED              INT_WAN_RX_STOPPED
#define INT_TX_STOPPED              INT_WAN_TX_STOPPED
#define INT_RX_OVERRUN              INT_WAN_RX_BUF_UNAVAIL
#define INT_TX_UNDERRUN             INT_WAN_TX_BUF_UNAVAIL
#define INT_TX_EMPTY                INT_WAN_TX_BUF_UNAVAIL
#define INT_RX                      INT_WAN_RX
#define INT_TX                      INT_WAN_TX
#define INT_PHY                     INT_WAN_PHY
#define INT_MASK                    ( INT_RX | INT_TX | INT_TX_EMPTY | INT_RX_STOPPED | INT_TX_STOPPED )


/* MAC Addition Station Address */

/* MAAL0 */
#define REG_ADD_ADDR_0_LO           0x0080
/* MAAH0 */
#define REG_ADD_ADDR_0_HI           0x0084
/* MAAL1 */
#define REG_ADD_ADDR_1_LO           0x0088
/* MAAH1 */
#define REG_ADD_ADDR_1_HI           0x008C
/* MAAL2 */
#define REG_ADD_ADDR_2_LO           0x0090
/* MAAH2 */
#define REG_ADD_ADDR_2_HI           0x0094
/* MAAL3 */
#define REG_ADD_ADDR_3_LO           0x0098
/* MAAH3 */
#define REG_ADD_ADDR_3_HI           0x009C
/* MAAL4 */
#define REG_ADD_ADDR_4_LO           0x00A0
/* MAAH4 */
#define REG_ADD_ADDR_4_HI           0x00A4
/* MAAL5 */
#define REG_ADD_ADDR_5_LO           0x00A8
/* MAAH5 */
#define REG_ADD_ADDR_5_HI           0x00AC
/* MAAL6 */
#define REG_ADD_ADDR_6_LO           0x00B0
/* MAAH6 */
#define REG_ADD_ADDR_6_HI           0x00B4
/* MAAL7 */
#define REG_ADD_ADDR_7_LO           0x00B8
/* MAAH7 */
#define REG_ADD_ADDR_7_HI           0x00BC
/* MAAL8 */
#define REG_ADD_ADDR_8_LO           0x00C0
/* MAAH8 */
#define REG_ADD_ADDR_8_HI           0x00C4
/* MAAL9 */
#define REG_ADD_ADDR_9_LO           0x00C8
/* MAAH9 */
#define REG_ADD_ADDR_9_HI           0x00CC
/* MAAL10 */
#define REG_ADD_ADDR_A_LO           0x00D0
/* MAAH10 */
#define REG_ADD_ADDR_A_HI           0x00D4
/* MAAL11 */
#define REG_ADD_ADDR_B_LO           0x00D8
/* MAAH11 */
#define REG_ADD_ADDR_B_HI           0x00DC
/* MAAL12 */
#define REG_ADD_ADDR_C_LO           0x00E0
/* MAAH12 */
#define REG_ADD_ADDR_C_HI           0x00E4
/* MAAL13 */
#define REG_ADD_ADDR_D_LO           0x00E8
/* MAAH13 */
#define REG_ADD_ADDR_D_HI           0x00EC
/* MAAL14 */
#define REG_ADD_ADDR_E_LO           0x00F0
/* MAAH14 */
#define REG_ADD_ADDR_E_HI           0x00F4
/* MAAL15 */
#define REG_ADD_ADDR_F_LO           0x00F8
/* MAAH15 */
#define REG_ADD_ADDR_F_HI           0x00FC

#define ADD_ADDR_HI_MASK            0x00FF
#define ADD_ADDR_ENABLE             0x8000

/* Miscellaneour Registers */

/* MARL */
#define REG_ADDR_0_OFFSET           0x0200
#define REG_ADDR_1_OFFSET           0x0201
/* MARM */
#define REG_ADDR_2_OFFSET           0x0202
#define REG_ADDR_3_OFFSET           0x0203
/* MARH */
#define REG_ADDR_4_OFFSET           0x0204
#define REG_ADDR_5_OFFSET           0x0205

/* OBCR */
#define REG_BUS_CTRL_OFFSET         0x0210

/* EEPCR */
#define REG_EEPROM_CTRL_OFFSET      0x0212

/* MBIR */
#define REG_MEM_INFO_OFFSET         0x0214

/* GCR */
#define REG_GLOBAL_CTRL_OFFSET      0x0216


/* WFCR */
#define REG_WOL_CTRL_OFFSET         0x021A

/* WF0 */

#define WOL_FRAME_CRC_OFFSET        0x0220
#define WOL_FRAME_BYTE0_OFFSET      0x0224
#define WOL_FRAME_BYTE2_OFFSET      0x0228


#endif /* #ifdef KS_PCI_BUS */

/*
 * ks884x Registers Bit definitions
 *
 *  Note: these bit definitions can be used by both ISA_BUS or PCI_BUS interface.
 */

/* Receive Descriptor */
#define DESC_OWN_BIT            0x80000000      /* Descriptor own bit, 1: own by ks884x, 0: own by host */

#define RFC_FS                  0x40000000      /* First Descriptor of the received frame */
#define RFC_LS                  0x20000000      /* Last Descriptor of the received frame */
#define RFC_IPE                 0x10000000      /* IP checksum generation */
#define RFC_TCPE                0x08000000      /* TCP checksum generation */
#define RFC_UDPE                0x04000000      /* UDP checksum generation */
#define RFC_ES                  0x02000000      /* Error Summary */
#define RFC_MF                  0x01000000      /* Multicast Frame */
#define RFC_RE                  0x00080000      /* Report on MII/GMII error */
#define RFC_TL                  0x00040000      /* Frame Too Long */
#define RFC_RF                  0x00020000      /* Runt Frame */
#define RFC_CRC                 0x00010000      /* CRC error */
#define RFC_FT                  0x00008000      /* Frame Type */
#define RFC_FL_MASK             0x000007ff      /* Frame Length bit mask, 0:10 */

#ifdef RCV_HUGE_FRAME
#define RFC_ERROR_MASK          (RFC_IPE | RFC_TCPE | RFC_UDPE | RFC_RE | RFC_CRC | RFC_RF )
#else
#define RFC_ERROR_MASK          (RFC_IPE | RFC_TCPE | RFC_UDPE | RFC_RE | RFC_CRC | RFC_RF | RFC_TL  )
#endif

/* Transmit Descriptor */
#define TFC_IC                  0x80000000      /* Interrupt on completion */
#define TFC_FS                  0x40000000      /* first segment */
#define TFC_LS                  0x20000000      /* last segment */
#define TFC_IPCKG               0x10000000      /* IP checksum generation */
#define TFC_TCPCKG              0x08000000      /* TCP checksum generation */
#define TFC_UDPCKG              0x04000000      /* UDP checksum generation */
#define TFC_TER                 0x02000000      /* Transmit End of Ring */
#define TFC_TBS_MASK            0x000007ff      /* Transmit Buffer Size Mask (0:10) */


/* DMA Registers */

/* MDTXC                  0x0000 */
/* MDRXC                  0x0004 */
#define DMA_PBLTMASK            0x3f000000      /* DMA Burst Size bit mask */
#define DMA_UDPCHECKSUM         0x00040000      /* MAC UDP checksum enable */
#define DMA_TCPCHECKSUM         0x00020000      /* MAC TCP checksum enable */
#define DMA_IPCHECKSUM          0x00010000      /* MAC IP checksum enable  */
#define DMA_FLOWCTRL            0x00000200      /* MAC flow control enable */
#define DMA_ERRORFRAME          0x00000008      /* MAC will Rx error frame */
#define DMA_PADDING             0x00000004      /* MAC Tx enable padding   */
#define DMA_CRC                 0x00000002      /* MAC Tx add CRC          */

#define DMA_BROADCAST           0x00000040      /* MAC Rx all broadcast frame */
#define DMA_MULTICAST           0x00000020      /* MAC Rx all multicast frame */
#define DMA_UNICAST             0x00000010      /* MAC Rx only unicast frame  */
#define DMA_PROMISCUOUS         0x00000004      /* MAC Rx all all frame       */

/* MDTSC                  0x0008 */
/* MDRSC                  0x000C */
#define DMA_START               0x00000001      /* DMA start command */

/* TDLB                   0x0010 */
/* RDLB                   0x0014 */

/* MTR0                   0x0020 */
/* MTR1                   0x0024 */



/* Interrupt Registers */

/* INTEV                   0x0028 */
/* INTST                   0x002C */
#define INT_TX_DONE             0x40000000      /* Enable Tx completed bit */
#define INT_RX_FRAME            0x20000000      /* Enable Rx at lease a frame bit */
#define INT_TX_STOP             0x04000000      /* Enable Tx stop bit */
#define INT_RX_STOP             0x02000000      /* Enable Tx stop bit */

/* MAC Addition Station Address */

/* MAAL0                   0x0080 */
/* MAAH0                   0x0084 */
#define MAC_ADDR_ENABLE         0x80000000      /* This MAC table entry is Enabled */

/* Miscellaneour Registers */

/* MARL                    0x0200 */
/* MARM                    0x0202 */
/* MARH                    0x0204 */
/* OBCR                    0x0210 */
/* EEPCR                   0x0212 */
/* MBIR                    0x0214 */

/* GCR                     0x0216 */
#define GLOBAL_SOFTWARE_RESET   0x0001      /* pass all frames */

/* Switch Registers */

/* SIDER                   0x0400 */
#define SW_ENABLE               0x0001      /* enable switch */

/* SGCR1                   0x0402 */
#define SW_PASS_ALL_FRAMES      0x8000      /* pass all frames */
#define SW_IEEE_TX_FLOWCNTL     0x2000      /* IEEE 802.3x Tx flow control enable */
#define SW_IEEE_RX_FLOWCNTL     0x1000      /* IEEE 802.3x Rx flow control enable */
#define SW_FRAME_LEN_CHECK      0x0800      /* frame length field check */
#define SW_AGING_ENABLE         0x0400      /* Aging enable */
#define SW_FAST_AGING           0x0200      /* Fast Age enable */
#define SW_BACKOFF_EN           0x0100      /* aggressive back off enable */
#define SW_UNH_BACKOFF_EN       0x0080      /* new backoff enable for UNH */
#define SW_PASS_FLOWCNTL_FRAMES 0x0008      /* NOT filter 802.1x flow control packets */
#define SW_BUFFER_SHARE         0x0004      /* buffer share mode */
#define SW_AUTO_FAST_AGING      0x0001      /* automic fast aging when link changed detected */

/* SGCR2                   0x0404 */
#define SW_8021Q_VLAN_EN        0x8000      /* Enable IEEE 802.1Q VLAN enable */
#define SW_IGMP_SNOOP_EN        0x4000      /* Enable IGMP Snoop on switch MII interface */
#define SW_SNIFF_TX_AND_RX      0x0100      /* Sniff monitor Tx and Rx. */
#define SW_VLAN_MISMATCH_DISCARD 0x0080     /* unicast port-VLAN mismatch discard */
#define SW_NO_MCAST_STORM_INC   0x0040      /* broadcast storm protection not include multicast pkts */
#define SW_PREAMBLE_MODE        0x0020      /* carrier sense based backpressure mode */
#define SW_FLOWCTRL_FAIR        0x0010      /* flow control fair mode */
#define SW_NO_COLLISION_DROP    0x0008      /* no excessive collision drop */
#define SW_HUGE_FRAME_SIZE      0x0004      /* support huge packet size upto 1916-byte */
#define SW_NO_MAX_FRAME_SIZE    0x0002      /* NOT accept packet size upto 1536-byte */
#define SW_PRIORITY_BUF_RESERVE 0x0001      /* pre-allocated 48 buffers per port reserved for high priority pkts */

/* SGCR3                   0x0406 */
#define SW_REPEATER_MODE_EN     0x0080      /* Enable repeater mode */
#define SW_MII_HALF_DUPLEX      0x0040      /* Enable switch MII half duplex mode */
#define SW_MII_FLOW_CNTL        0x0020      /* Enable switch MII flow control */
#define SW_MII_10BT             0x0010      /* The switch MII interface is in 10Mbps mode */
#define SW_NULL_VID             0x0008      /* null VID replacement */

/* SGCR4                   0x0408 */
/* SGCR5                   0x040A */
#define SW_POWER_SAVE           0x0400      /* Enable power save mode */
#define SW_CRC_DROP             0x0200      /* drop MC loop back packets if CRCs are detected */
#define SW_TPID_MODE            0x0100      /* Special TPID mode */

/* SGCR6                   0x0410 */
/* PHAR                    0x0420 */
/* LBS21R                  0x0426 */
/* LBRCTCER                0x0428 */
/* LBRCGR                  0x042A */
/* CSCR                    0x0430 */
/* PSWIR                   0x0432 */
/* RC21R                   0x0434 */
/* RC3R                    0x0436 */
/* VMCRTCR                 0x0438 */
/* S58R                    0x0440 */
/* MVI21R                  0x0444 */
/* MM1V3IR                 0x0446 */
/* MMI32R                  0x0448 */
/* LPVI21R                 0x0450 */
/* LPM1V3IR                0x0452 */
/* LPMI32R                 0x0454 */
/* CSSR                    0x0460 */
/* ASCTR                   0x0464 */
/* MS21R                   0x0468 */
/* LPS21R                  0x046A */

/* MACAR1                  0x0470 */
/* MACAR2                  0x0472 */
/* MACAR3                  0x0474 */

/* TOSR1                   0x0480 */
/* TOSR2                   0x0482 */
/* TOSR3                   0x0484 */
/* TOSR4                   0x0486 */
/* TOSR5                   0x0488 */
/* TOSR6                   0x048A */
/* TOSR7                   0x0490 */
/* TOSR8                   0x0492 */

/* IACR                    0x04A0 */
/* IADR1                   0x04A2 */
/* IADR2                   0x04A4 */
/* IADR3                   0x04A6 */
/* IADR4                   0x04A8 */
/* IADR5                   0x04AA */

/* UDR21                   0x04B0 */
/* UDR3                    0x04B2 */

/* DTSR                    0x04C0 */
/* ATSR                    0x04C2 */
/* DTCR                    0x04C4 */
/* ATCR0                   0x04C6 */
/* ATCR1                   0x04C8 */
/* ATCR2                   0x04CA */

/* P1MBCR                  0x04D0 */
#define PHY_POWER_POWERDOWN     0x0800      /* port power down */
#define PHY_AUTO_NEGOTIATION    0x0200      /* auto-negotiation enable */

/* P1MBSR                  0x04D2 */
#define PHY_AUTONEGO_COMPLETE   0x0020      /* auto nego completed on this port */
#define PHY_LINKUP              0x0004      /* Link is up on this port */

/* PHY1ILR                 0x04D4 */
/* PHY1IHR                 0x04D6 */
/* P1ANAR                  0x04D8 */
/* P1ANLPR                 0x04DA */
#define PARTNER_100FD           0x0100      /* auto nego parterner 100 FD */
#define PARTNER_100HD           0x0080      /* auto nego parterner 100 HD */
#define PARTNER_10FD            0x0040      /* auto nego parterner 10 FD */
#define PARTNER_10HD            0x0020      /* auto nego parterner 10 HD */

/* P2MBCR                  0x04E0 */
/* P2MBSR                  0x04E2 */
/* PHY2ILR                 0x04E4 */
/* PHY2IHR                 0x04E6 */
/* P2ANAR                  0x04E8 */
/* P2ANLPR                 0x04EA */
/* P1VCT                   0x04F0 */
/* P1PHYCTRL               0x04F2 */
/* P2VCT                   0x04F4 */
/* P2PHYCTRL               0x04F6 */

/* P1CR1                   0x0500 */
#define PORT_STORM_PROCTION     0x0080      /* enable broadcast storm protection (ingress) */
#define PORT_QOS_DIFFSERV       0x0040      /* enable QoS - diffServ priority classfication */
#define PORT_QOS_8021P          0x0020      /* enable QoS - 802.1P priority classfication */
#define PORT_TAG_INSERT         0x0004      /* enable VLAN tag insert to the packet (egress) */
#define PORT_TAG_REMOVE         0x0002      /* enable VLAN tag remove from the packet (egress) */
#define PORT_MULTIPLE_Q         0x0001      /* output queue is split into four queues (egress) */

/* P1CR2                   0x0502 */

/* P1VIDCR                 0x0504 */
/* P1CR3                   0x0506 */
/* P1IRCR                  0x0508 */
/* P1ERCR                  0x050A */
/* P1SCSLMD                0x0510 */
/* P1CR4                   0x0512 */
#define PORT_AUTONEGO_RESTART   0x2000      /* auto nego restart */
#define PORT_AUTONEGO_ENABLE    0x0080      /* auto nego enable */
#define PORT_AUTONEGO_ADV_PUASE 0x0010      /* auto nego advertise PAUSE */
#define PORT_AUTONEGO_ADV_100FD 0x0008      /* auto nego advertise 100 FD */
#define PORT_AUTONEGO_ADV_100HD 0x0004      /* auto nego advertise 100 HD */
#define PORT_AUTONEGO_ADV_10FD  0x0002      /* auto nego advertise 10 FD */
#define PORT_AUTONEGO_ADV_10HD  0x0001      /* auto nego advertise 10 HD */
#define PORT_AUTONEGO_ADV_MASK  0x209F

#define PORT_DISABLE_AUTONEG    0x0000      /* port disable auto nego */
#define PORT_100BASE            0x0040      /* force 100 when auto nego disabled */
#define PORT_FULLDUPLEX         0x0020      /* force full duplex when auto nego disabled */
#define PORT_MEDIA_MASK         0x0060

/* P1SR                    0x0514 */
#define PORT_DUPLEX_FULL        0x0400      /* auto nego duplex status (solved) */
#define PORT_SPEED_100BT        0x0200      /* auto nego speed status (solved) */

/* P2CR1                   0x0520 */
/* P2CR2                   0x0522 */
/* P2VIDCR                 0x0524 */
/* P2CR3                   0x0526 */
/* P2IRCR                  0x0528 */
/* P2ERCR                  0x052A */
/* P2SCSLMD                0x0530 */
/* P2CR4                   0x0532 */
/* P2SR                    0x0534 */

/* P3CR1                   0x0540 */
/* P3CR2                   0x0542 */
/* P3VIDCR                 0x0544 */
/* P3CR3                   0x0546 */
/* P3IRCR                  0x0548 */
/* P3ERCR                  0x054A */
/* P3SR                    0x0554 */

/* -------------------------------------------------------------------------- */

#ifdef KS_ISA_BUS
#ifdef INLINE
#ifdef SH_16BIT_WRITE
#define HardwareSelectBank( pHardware, bBank )                              \
{                                                                           \
    HW_WRITE_WORD( pHardware, REG_BANK_SEL_OFFSET, bBank );                 \
    ( pHardware )->m_bBank = bBank;                                         \
}

#else
#define HardwareSelectBank( pHardware, bBank )                              \
{                                                                           \
    HW_WRITE_BYTE( pHardware, REG_BANK_SEL_OFFSET, bBank );                 \
    ( pHardware )->m_bBank = bBank;                                         \
}
#endif

#define HardwareReadRegByte( pHardware, bBank, bOffset, pbData )            \
{                                                                           \
    if ( ( bBank ) != ( pHardware )->m_bBank )                              \
        HardwareSelectBank( pHardware, bBank );                             \
    HW_READ_BYTE( pHardware, bOffset, pbData );                             \
}

#define HardwareWriteRegByte( pHardware, bBank, bOffset, bValue )           \
{                                                                           \
    if ( ( bBank ) != ( pHardware )->m_bBank )                              \
        HardwareSelectBank( pHardware, bBank );                             \
    HW_WRITE_BYTE( pHardware, bOffset, bValue );                            \
}

#define HardwareReadRegWord( pHardware, bBank, bOffset, pwData )            \
{                                                                           \
    if ( ( bBank ) != ( pHardware )->m_bBank )                              \
        HardwareSelectBank( pHardware, bBank );                             \
    HW_READ_WORD( pHardware, bOffset, pwData );                             \
}

#define HardwareWriteRegWord( pHardware, bBank, bOffset, wValue )           \
{                                                                           \
    if ( ( bBank ) != ( pHardware )->m_bBank )                              \
        HardwareSelectBank( pHardware, bBank );                             \
    HW_WRITE_WORD( pHardware, bOffset, wValue );                            \
}

#define HardwareReadRegDWord( pHardware, bBank, bOffset, pwData )           \
{                                                                           \
    if ( ( bBank ) != ( pHardware )->m_bBank )                              \
        HardwareSelectBank( pHardware, bBank );                             \
    HW_READ_DWORD( pHardware, bOffset, pwData );                            \
}

#define HardwareWriteRegDWord( pHardware, bBank, bOffset, wValue )          \
{                                                                           \
    if ( ( bBank ) != ( pHardware )->m_bBank )                              \
        HardwareSelectBank( pHardware, bBank );                             \
    HW_WRITE_DWORD( pHardware, bOffset, wValue );                           \
}

#ifdef SH_32BIT_ACCESS_ONLY
#define HardwareWriteIntMask( pHardware, ulValue )                          \
{                                                                           \
    if ( REG_INT_MASK_BANK != ( pHardware )->m_bBank )                      \
        HardwareSelectBank( pHardware, REG_INT_MASK_BANK );                 \
    HW_WRITE_DWORD( pHardware, REG_INT_MASK_OFFSET, ulValue );              \
}

#define HardwareWriteIntStat( pHardware, ulValue )                          \
{                                                                           \
    ULONG ulIntEnable;                                                      \
    if ( REG_INT_STATUS_BANK != ( pHardware )->m_bBank )                    \
        HardwareSelectBank( pHardware, REG_INT_STATUS_BANK );               \
    HW_READ_DWORD( pHardware, REG_INT_MASK_OFFSET, &ulIntEnable );          \
    ulIntEnable &= 0x0000FFFF;                                              \
    ulIntEnable |= ulValue;                                                 \
    HW_WRITE_DWORD( pHardware, REG_INT_MASK_OFFSET, ulIntEnable );          \
}
#endif

#else
void HardwareSelectBank (
    PHARDWARE pHardware,
    UCHAR     bBank );

void HardwareReadRegByte (
    PHARDWARE pHardware,
    UCHAR     bBank,
    UCHAR     bOffset,
    PUCHAR    pbData );

void HardwareReadRegWord (
    PHARDWARE pHardware,
    UCHAR     bBank,
    UCHAR     bOffset,
    PUSHORT   pwData );

void HardwareReadRegDWord (
    PHARDWARE pHardware,
    UCHAR     bBank,
    UCHAR     bOffset,
    PULONG    pwData );

void HardwareWriteRegByte (
    PHARDWARE pHardware,
    UCHAR     bBank,
    UCHAR     bOffset,
    UCHAR     bData );

void HardwareWriteRegWord (
    PHARDWARE pHardware,
    UCHAR     bBank,
    UCHAR     bOffset,
    USHORT    wData );

void HardwareWriteRegDWord (
    PHARDWARE pHardware,
    UCHAR     bBank,
    UCHAR     bOffset,
    ULONG     wData );

#ifdef SH_32BIT_ACCESS_ONLY
void HardwareWriteIntMask (
    PHARDWARE pHardware,
    ULONG     ulValue );

void HardwareWriteIntStat (
    PHARDWARE pHardware,
    ULONG     ulValue );
#endif

#endif

void hw_read_dword
(
    PHARDWARE phwi,
    UCHAR     addr,
    ULONG    *data
);
void hw_write_dword
(
    PHARDWARE phwi,
    UCHAR     addr,
    ULONG     data
);

void HardwareReadBuffer (
    PHARDWARE pHardware,
    UCHAR     bOffset,
    PULONG    pdwData,
    int       length );

void HardwareWriteBuffer (
    PHARDWARE pHardware,
    UCHAR     bOffset,
    PULONG    pdwData,
    int       length );


#endif

/* -------------------------------------------------------------------------- */

/*
    Initial setup routines
*/

#ifdef KS_ISA_BUS
BOOLEAN HardwareInitialize_ISA (
    PHARDWARE pHardware );

BOOLEAN HardwareReset_ISA (
    PHARDWARE pHardware );

void HardwareSetup_ISA (
    PHARDWARE pHardware );

void HardwareSetupInterrupt_ISA (
    PHARDWARE pHardware );

BOOLEAN HardwareSetBurst (
    PHARDWARE pHardware,
    UCHAR     bBurstLength );

#endif

#ifdef KS_PCI
#ifdef DBG
void CheckDescriptors (
    PTDescInfo pInfo );
#endif

void CheckDescriptorNum (
    PTDescInfo pInfo );

BOOLEAN HardwareInitialize_PCI (
    PHARDWARE pHardware );

BOOLEAN HardwareReset_PCI (
    PHARDWARE pHardware );

void HardwareSetup_PCI (
    PHARDWARE pHardware );

void HardwareSetupInterrupt_PCI (
    PHARDWARE pHardware );
#endif

void HardwareSwitchSetup
(
    PHARDWARE pHardware );


void HardwareReadChipID
(
    PHARDWARE pHardware,
    PUSHORT   pChipID,
    PUCHAR    pDevRevisionID
);

#ifdef KS_PCI_BUS
#define HardwareInitialize      HardwareInitialize_PCI
#define HardwareReset           HardwareReset_PCI
#define HardwareSetup           HardwareSetup_PCI
#define HardwareSetupInterrupt  HardwareSetupInterrupt_PCI

#else
#define HardwareInitialize      HardwareInitialize_ISA
#define HardwareReset           HardwareReset_ISA
#define HardwareSetup           HardwareSetup_ISA
#define HardwareSetupInterrupt  HardwareSetupInterrupt_ISA
#endif

/* -------------------------------------------------------------------------- */

/*
    Link processing primary routines
*/

#ifdef KS_ISA_BUS
#ifdef INLINE
#ifdef SH_32BIT_ACCESS_ONLY
#define HardwareAcknowledgeLink_ISA( pHardware )                            \
{                                                                           \
    HardwareWriteIntStat( pHardware, INT_STATUS( INT_PHY ));                \
}
#else
#define HardwareAcknowledgeLink_ISA( pHardware )                            \
{                                                                           \
    HardwareWriteRegWord( pHardware, REG_INT_STATUS_BANK,                   \
        REG_INT_STATUS_OFFSET, INT_PHY );                                   \
}
#endif

#else
void HardwareAcknowledgeLink_ISA (
    PHARDWARE pHardware );
#endif

void HardwareCheckLink_ISA (
    PHARDWARE pHardware );
#endif

#ifdef KS_PCI_BUS
#ifdef INLINE
#define HardwareAcknowledgeLink_PCI( pHardware )                            \
{                                                                           \
    HW_WRITE_DWORD( pHardware, REG_INTERRUPTS_STATUS, INT_PHY );            \
}

#else
void HardwareAcknowledgeLink_PCI (
    PHARDWARE pHardware );
#endif

void HardwareCheckLink_PCI (
    PHARDWARE pHardware );
#endif

#ifdef KS_PCI_BUS
#define HardwareAcknowledgeLink  HardwareAcknowledgeLink_PCI
#define HardwareCheckLink        HardwareCheckLink_PCI

#else
#define HardwareAcknowledgeLink  HardwareAcknowledgeLink_ISA
#define HardwareCheckLink        HardwareCheckLink_ISA
#endif

/* -------------------------------------------------------------------------- */

/*
    Receive processing primary routines
*/

#ifdef KS_ISA_BUS
#ifdef INLINE
#define HardwareReleaseReceive_ISA( pHardware )                             \
{                                                                           \
    HardwareWriteRegByte( pHardware, REG_RXQ_CMD_BANK, REG_RXQ_CMD_OFFSET,  \
                          RXQ_CMD_FREE_PACKET );                            \
}

#ifdef SH_32BIT_ACCESS_ONLY
#define HardwareAcknowledgeReceive_ISA( pHardware )                         \
{                                                                           \
    HardwareWriteIntStat( pHardware, INT_STATUS( INT_RX ));                 \
}
#else
#define HardwareAcknowledgeReceive_ISA( pHardware )                         \
{                                                                           \
    HardwareWriteRegWord( pHardware, REG_INT_STATUS_BANK, REG_INT_STATUS_OFFSET,  \
                          INT_RX );                                         \
}
#endif

#else

void HardwareReleaseReceive_ISA (
    PHARDWARE pHardware );

void HardwareAcknowledgeReceive_ISA (
    PHARDWARE pHardware );
#endif

void HardwareStartReceive_ISA (
    PHARDWARE pHardware );

void HardwareStopReceive_ISA (
    PHARDWARE pHardware );

#endif /* #ifdef INLINE */

#ifdef KS_PCI_BUS
#ifdef INLINE
#define HardwareAcknowledgeReceive_PCI( pHardware )                         \
{                                                                           \
    HW_WRITE_DWORD( pHardware, REG_INTERRUPTS_STATUS, INT_RX );             \
}

#else
void HardwareReleaseReceive_PCI (
    PHARDWARE pHardware );

void HardwareAcknowledgeReceive_PCI (
    PHARDWARE pHardware );
#endif

#define HardwareResumeReceive( pHardware )                                  \
{                                                                           \
    HW_WRITE_DWORD( pHardware, REG_DMA_RX_START, DMA_START );               \
}

void HardwareStartReceive_PCI (
    PHARDWARE pHardware );

void HardwareStopReceive_PCI (
    PHARDWARE pHardware );
#endif

#ifdef KS_PCI_BUS
#define HardwareReleaseReceive      HardwareReleaseReceive_PCI
#define HardwareAcknowledgeReceive  HardwareAcknowledgeReceive_PCI
#define HardwareStartReceive        HardwareStartReceive_PCI
#define HardwareStopReceive         HardwareStopReceive_PCI

#else
#define HardwareReleaseReceive      HardwareReleaseReceive_ISA
#define HardwareAcknowledgeReceive  HardwareAcknowledgeReceive_ISA
#define HardwareStartReceive        HardwareStartReceive_ISA
#define HardwareStopReceive         HardwareStopReceive_ISA
#endif

/* -------------------------------------------------------------------------- */

/*
    Transmit processing primary routines
*/

#ifdef KS_ISA_BUS
#ifdef INLINE
#ifdef SH_32BIT_ACCESS_ONLY
#define HardwareAcknowledgeTransmit_ISA( pHardware )                        \
{                                                                           \
    HardwareWriteIntStat( pHardware, INT_STATUS( INT_TX ));                 \
}
#else
#define HardwareAcknowledgeTransmit_ISA( pHardware )                        \
{                                                                           \
    HardwareWriteRegWord( pHardware, REG_INT_STATUS_BANK,                   \
        REG_INT_STATUS_OFFSET, INT_TX );                                    \
}
#endif

#else
void HardwareAcknowledgeTransmit_ISA (
    PHARDWARE pHardware );
#endif

void HardwareStartTransmit_ISA (
    PHARDWARE pHardware );

void HardwareStopTransmit_ISA (
    PHARDWARE pHardware );
#endif

#ifdef KS_PCI_BUS
#ifdef INLINE
#define HardwareAcknowledgeTransmit_PCI( pHardware )                        \
{                                                                           \
    HW_WRITE_DWORD( pHardware, REG_INTERRUPTS_STATUS, INT_TX );             \
}

#else
void HardwareAcknowledgeTransmit_PCI (
    PHARDWARE pHardware );
#endif

void HardwareStartTransmit_PCI (
    PHARDWARE pHardware );

void HardwareStopTransmit_PCI (
    PHARDWARE pHardware );
#endif

#ifdef KS_PCI_BUS
#define HardwareAcknowledgeTransmit  HardwareAcknowledgeTransmit_PCI
#define HardwareStartTransmit        HardwareStartTransmit_PCI
#define HardwareStopTransmit         HardwareStopTransmit_PCI

#else
#define HardwareAcknowledgeTransmit  HardwareAcknowledgeTransmit_ISA
#define HardwareStartTransmit        HardwareStartTransmit_ISA
#define HardwareStopTransmit         HardwareStopTransmit_ISA
#endif

/* -------------------------------------------------------------------------- */

/*
    Interrupt processing primary routines
*/

#ifdef KS_ISA_BUS
#ifdef INLINE
#ifdef SH_32BIT_ACCESS_ONLY
#define HardwareAcknowledgeInterrupt_ISA( pHardware, wInterrupt )           \
{                                                                           \
    HardwareWriteIntStat( pHardware, INT_STATUS( wInterrupt ));             \
}

#define HardwareDisableInterrupt_ISA( pHardware )                           \
{                                                                           \
    HardwareWriteIntMask( pHardware, 0 );                                   \
}

#define HardwareEnableInterrupt_ISA( pHardware )                            \
{                                                                           \
    HardwareWriteIntMask( pHardware, ( pHardware )->m_wInterruptMask );     \
}

#define HardwareDisableInterruptBit_ISA( pHardware, wInterrupt )            \
{                                                                           \
    ULONG dwReadInterrupt;                                                  \
    HardwareReadRegDWord( pHardware, REG_INT_MASK_BANK,                     \
        REG_INT_MASK_OFFSET, &dwReadInterrupt );                            \
    dwReadInterrupt &= 0x0000FFFF;                                          \
    dwReadInterrupt &= ~wInterrupt;                                         \
    HW_WRITE_DWORD( pHardware, REG_INT_MASK_OFFSET, dwReadInterrupt );      \
}

#define HardwareEnableInterruptBit_ISA( pHardware, wInterrupt )             \
{                                                                           \
    ULONG dwReadInterrupt;                                                  \
    HardwareReadRegDWord( pHardware, REG_INT_MASK_BANK,                     \
        REG_INT_MASK_OFFSET, &dwReadInterrupt );                            \
    dwReadInterrupt &= 0x0000FFFF;                                          \
    dwReadInterrupt |= wInterrupt;                                          \
    HW_WRITE_DWORD( pHardware, REG_INT_MASK_OFFSET, dwReadInterrupt );      \
}

#define HardwareReadInterrupt_ISA( pHardware, pwStatus )                    \
{                                                                           \
    HardwareReadRegWord( pHardware, REG_INT_STATUS_BANK,                    \
        REG_INT_STATUS_OFFSET, pwStatus );                                  \
}

#define HardwareSetInterrupt_ISA( pHardware, wInterrupt )                   \
{                                                                           \
    HardwareWriteIntMask( pHardware, wInterrupt );                          \
}
#else
#define HardwareAcknowledgeInterrupt_ISA( pHardware, wInterrupt )           \
{                                                                           \
    HardwareWriteRegWord( pHardware, REG_INT_STATUS_BANK,                   \
        REG_INT_STATUS_OFFSET, wInterrupt );                                \
}

#define HardwareDisableInterrupt_ISA( pHardware )                           \
{                                                                           \
    HardwareWriteRegWord( pHardware, REG_INT_MASK_BANK,                     \
        REG_INT_MASK_OFFSET, 0 );                                           \
}

#define HardwareEnableInterrupt_ISA( pHardware )                            \
{                                                                           \
    HardwareWriteRegWord( pHardware, REG_INT_MASK_BANK,                     \
        REG_INT_MASK_OFFSET, ( pHardware )->m_wInterruptMask );             \
}

#define HardwareDisableInterruptBit_ISA( pHardware, wInterrupt )            \
{                                                                           \
    USHORT     wReadInterrupt;                                              \
    HardwareReadRegWord( pHardware, REG_INT_MASK_BANK, REG_INT_MASK_OFFSET, \
                          &wReadInterrupt );                                \
    HardwareWriteRegWord( pHardware, REG_INT_MASK_BANK, REG_INT_MASK_OFFSET,\
                          (wReadInterrupt & ~wInterrupt) );                 \
}

#define HardwareEnableInterruptBit_ISA( pHardware, wInterrupt )             \
{                                                                           \
    USHORT     wReadInterrupt;                                              \
    HardwareReadRegWord( pHardware, REG_INT_MASK_BANK, REG_INT_MASK_OFFSET, \
                          &wReadInterrupt );                                \
    HardwareWriteRegWord( pHardware, REG_INT_MASK_BANK, REG_INT_MASK_OFFSET,\
                          (wReadInterrupt | wInterrupt) );                  \
}

#define HardwareReadInterrupt_ISA( pHardware, pwStatus )                    \
{                                                                           \
    HardwareReadRegWord( pHardware, REG_INT_STATUS_BANK,                    \
        REG_INT_STATUS_OFFSET, pwStatus );                                  \
}

#define HardwareSetInterrupt_ISA( pHardware, wInterrupt )                   \
{                                                                           \
    HardwareWriteRegWord( pHardware, REG_INT_MASK_BANK,                     \
        REG_INT_MASK_OFFSET, wInterrupt );                                  \
}
#endif

#else
void HardwareAcknowledgeInterrupt_ISA (
    PHARDWARE pHardware,
    USHORT    wInterrupt );

void HardwareDisableInterrupt_ISA (
    PHARDWARE pHardware );

void HardwareEnableInterrupt_ISA (
    PHARDWARE pHardware );

void HardwareDisableInterruptBit_ISA (
    PHARDWARE pHardware,
    USHORT    wInterrupt );

void HardwareEnableInterruptBit_ISA (
    PHARDWARE pHardware,
    USHORT    wInterrupt );

void HardwareReadInterrupt_ISA (
    PHARDWARE pHardware,
    PUSHORT   wStatus );

void HardwareSetInterrupt_ISA (
    PHARDWARE pHardware,
    USHORT    wInterrupt );
#endif

USHORT HardwareBlockInterrupt_ISA (
    PHARDWARE pHardware );
#endif

#ifdef KS_PCI_BUS
#ifdef INLINE
#define HardwareAcknowledgeInterrupt_PCI( pHardware, ulInterrupt )          \
{                                                                           \
    HW_WRITE_DWORD( pHardware, REG_INTERRUPTS_STATUS, ulInterrupt );        \
}

#define HardwareDisableInterrupt_PCI( pHardware )                           \
{                                                                           \
    HW_WRITE_DWORD( pHardware, REG_INTERRUPTS_ENABLE, 0 );                  \
    HW_READ_DWORD( pHardware, REG_INTERRUPTS_ENABLE,                        \
        &( pHardware )->m_ulInterruptSet );                                 \
}

#define HardwareEnableInterrupt_PCI( pHardware )                            \
{                                                                           \
    ( pHardware )->m_ulInterruptSet = ( pHardware )->m_ulInterruptMask;     \
    HW_WRITE_DWORD( pHardware, REG_INTERRUPTS_ENABLE,                       \
        ( pHardware )->m_ulInterruptMask );                                 \
}

#define HardwareDisableInterruptBit_PCI( pHardware, ulInterrupt )           \
{                                                                           \
    ULONG     ulReadInterrupt;                                              \
    HW_READ_DWORD( pHardware, REG_INTERRUPTS_ENABLE, &ulReadInterrupt );    \
    ( pHardware )->m_ulInterruptSet = ulReadInterrupt & ~( ulInterrupt );   \
    HW_WRITE_DWORD( pHardware, REG_INTERRUPTS_ENABLE,                       \
        ( pHardware )->m_ulInterruptSet );                                  \
}

#define HardwareEnableInterruptBit_PCI( pHardware, ulInterrupt )            \
{                                                                           \
    ULONG     ulReadInterrupt;                                              \
    HW_READ_DWORD( pHardware, REG_INTERRUPTS_ENABLE, &ulReadInterrupt );    \
    ( pHardware )->m_ulInterruptSet = ulReadInterrupt | ( ulInterrupt );    \
    HW_WRITE_DWORD( pHardware, REG_INTERRUPTS_ENABLE,                       \
        ( pHardware )->m_ulInterruptSet );                                  \
}

#define HardwareReadInterrupt_PCI( pHardware, pulStatus )                   \
{                                                                           \
    HW_READ_DWORD( pHardware, REG_INTERRUPTS_STATUS, pulStatus );           \
}

#define HardwareSetInterrupt_PCI( pHardware, ulInterrupt )                  \
{                                                                           \
    ( pHardware )->m_ulInterruptSet = ulInterrupt;                          \
    HW_WRITE_DWORD( pHardware, REG_INTERRUPTS_ENABLE, ulInterrupt );        \
}

#else
void HardwareAcknowledgeInterrupt_PCI (
    PHARDWARE pHardware,
    ULONG     ulInterrupt );

void HardwareDisableInterrupt_PCI (
    PHARDWARE pHardware );

void HardwareEnableInterrupt_PCI (
    PHARDWARE pHardware );

void HardwareDisableInterruptBit_PCI (
    PHARDWARE pHardware,
    ULONG     ulInterrupt );

void HardwareEnableInterruptBit_PCI (
    PHARDWARE pHardware,
    ULONG     ulInterrupt );

void HardwareReadInterrupt_PCI (
    PHARDWARE pHardware,
    PULONG    pulStatus );

void HardwareSetInterrupt_PCI (
    PHARDWARE pHardware,
    ULONG     ulInterrupt );
#endif

ULONG HardwareBlockInterrupt_PCI (
    PHARDWARE pHardware );
#endif

#ifdef KS_PCI_BUS
#define HardwareAcknowledgeInterrupt  HardwareAcknowledgeInterrupt_PCI
#define HardwareDisableInterrupt      HardwareDisableInterrupt_PCI
#define HardwareEnableInterrupt       HardwareEnableInterrupt_PCI
#define HardwareDisableInterruptBit   HardwareDisableInterruptBit_PCI
#define HardwareEnableInterruptBit    HardwareEnableInterruptBit_PCI
#define HardwareReadInterrupt         HardwareReadInterrupt_PCI
#define HardwareSetInterrupt          HardwareSetInterrupt_PCI
#define HardwareBlockInterrupt        HardwareBlockInterrupt_PCI

#else
#define HardwareAcknowledgeInterrupt  HardwareAcknowledgeInterrupt_ISA
#define HardwareDisableInterrupt      HardwareDisableInterrupt_ISA
#define HardwareEnableInterrupt       HardwareEnableInterrupt_ISA
#define HardwareDisableInterruptBit   HardwareDisableInterruptBit_ISA
#define HardwareEnableInterruptBit    HardwareEnableInterruptBit_ISA
#define HardwareReadInterrupt         HardwareReadInterrupt_ISA
#define HardwareSetInterrupt          HardwareSetInterrupt_ISA
#define HardwareBlockInterrupt        HardwareBlockInterrupt_ISA
#endif

/* -------------------------------------------------------------------------- */

/*
    Other interrupt primary routines
*/

#ifdef KS_ISA_BUS
#ifdef INLINE
#define HardwareTurnOffInterrupt_ISA( pHardware, wInterruptBit )            \
{                                                                           \
    ( pHardware )->m_wInterruptMask &= ~( wInterruptBit );                  \
}

#else
void HardwareTurnOffInterrupt_ISA (
    PHARDWARE pHardware,
    USHORT    wInterruptBit );
#endif

void HardwareTurnOnInterrupt_ISA (
    PHARDWARE pHardware,
    USHORT    wInterruptBit,
    PUSHORT   pwInterruptMask );

#define HardwareTurnOffInterrupt    HardwareTurnOffInterrupt_ISA
#define HardwareTurnOnInterrupt     HardwareTurnOnInterrupt_ISA

#define HardwareAcknowledgeUnderrun( pHardware )                            \
    HardwareAcknowledgeInterrupt( pHardware, INT_TX_UNDERRUN )

#define HardwareAcknowledgeEarly( pHardware )                               \
    HardwareAcknowledgeInterrupt( pHardware, INT_RX_EARLY )

#define HardwareAcknowledgeOverrun( pHardware )                             \
    HardwareAcknowledgeInterrupt( pHardware, INT_RX_OVERRUN )

#define HardwareAcknowledgeErrorFrame( pHardware )                          \
    HardwareAcknowledgeInterrupt( pHardware, INT_RX_ERROR )

#define HardwareAcknowledgeRxStop( pHardware )                              \
    HardwareAcknowledgeInterrupt( pHardware, INT_RX_STOPPED )

#define HardwareAcknowledgeTxStop( pHardware )                              \
    HardwareAcknowledgeInterrupt( pHardware, INT_TX_STOPPED )


#define HardwareTurnOffEarlyInterrupt( pHardware )                          \
    HardwareTurnOffInterrupt( pHardware, INT_RX_EARLY )


#define HardwareTurnOnEarlyInterrupt( pHardware, pbInterruptMask )          \
    HardwareTurnOnInterrupt( pHardware, INT_RX_EARLY, pbInterruptMask )

#endif

#ifdef KS_PCI_BUS
#ifdef INLINE
#define HardwareTurnOffInterrupt_PCI( pHardware, ulInterruptBit )           \
{                                                                           \
    ( pHardware )->m_ulInterruptMask &= ~( ulInterruptBit );                \
}

#else
void HardwareTurnOffInterrupt_PCI (
    PHARDWARE pHardware,
    ULONG     ulInterruptBit );
#endif

void HardwareTurnOnInterrupt_PCI (
    PHARDWARE pHardware,
    ULONG     ulInterruptBit,
    PULONG    pulInterruptMask );

#define HardwareTurnOffInterrupt    HardwareTurnOffInterrupt_PCI
#define HardwareTurnOnInterrupt     HardwareTurnOnInterrupt_PCI

#define HardwareAcknowledgeEmpty( pHardware )                               \
    HardwareAcknowledgeInterrupt( pHardware, INT_WAN_TX_BUF_UNAVAIL )

#define HardwareAcknowledgeOverrun( pHardware )                             \
    HardwareAcknowledgeInterrupt( pHardware, INT_RX_OVERRUN )

#define HardwareTurnOffEmptyInterrupt( pHardware )                          \
    HardwareTurnOffInterrupt( pHardware, INT_WAN_TX_BUF_UNAVAIL )

#define HardwareTurnOnEmptyInterrupt( pHardware, pulInterruptMask )         \
    HardwareTurnOnInterrupt( pHardware, INT_WAN_TX_BUF_UNAVAIL,             \
        pulInterruptMask )
#endif

/* -------------------------------------------------------------------------- */

/*
    Register saving routines
*/

#ifdef KS_ISA_BUS
#ifdef INLINE
#define HardwareRestoreBank( pHardware )                                    \
{                                                                           \
    HardwareSelectBank( pHardware, ( pHardware )->m_bSavedBank );           \
}

#define HardwareSaveBank( pHardware )                                       \
{                                                                           \
    HW_READ_BYTE( pHardware, REG_BANK_SEL_OFFSET, &( pHardware )->m_bBank );  \
    ( pHardware )->m_bSavedBank = ( pHardware )->m_bBank;                   \
}

#define HardwareRestoreRegs( pHardware )                                    \
{                                                                           \
    HardwareRestoreBank( pHardware );                                       \
}

#define HardwareSaveRegs( pHardware )                                       \
{                                                                           \
    HardwareSaveBank( pHardware );                                          \
}


#else
void HardwareRestoreBank (
    PHARDWARE pHardware );

void HardwareSaveBank (
    PHARDWARE pHardware );

void HardwareRestoreRegs (
    PHARDWARE pHardware );

void HardwareSaveRegs (
    PHARDWARE pHardware );
#endif
#endif

/* -------------------------------------------------------------------------- */

/*
    Hardware enable/disable secondary routines
*/

#ifdef KS_ISA_BUS
void HardwareDisable_ISA (
    PHARDWARE pHardware );

void HardwareEnable_ISA (
    PHARDWARE pHardware );
#endif

#ifdef KS_PCI_BUS
void HardwareDisable_PCI (
    PHARDWARE pHardware );

void HardwareEnable_PCI (
    PHARDWARE pHardware );
#endif

#ifdef KS_PCI_BUS
#define HardwareDisable     HardwareDisable_PCI
#define HardwareEnable      HardwareEnable_PCI

#else
#define HardwareDisable     HardwareDisable_ISA
#define HardwareEnable      HardwareEnable_ISA
#endif

/* -------------------------------------------------------------------------- */

/*
    Receive processing secondary routines
*/

#ifdef KS_ISA_BUS
void HardwareReceiveEarly (
    PHARDWARE pHardware );

USHORT HardwareReceiveMoreDataAvailable (
      PHARDWARE pHardware );

BOOLEAN HardwareReceiveStatus (
      PHARDWARE pHardware,
      PUSHORT   pwStatus,
      PUSHORT   pwLength );

int HardwareReceiveLength (
    PHARDWARE pHardware );

void HardwareReceiveBuffer (
    PHARDWARE pHardware,
    void*     pBuffer,
    int       length );

void HardwareReceive_ISA (
    PHARDWARE pHardware );
#endif

#ifdef KS_PCI_BUS
#ifdef INLINE
#define FreeReceivedPacket( pInfo, iNext )                                  \
{                                                                           \
    iNext++;                                                                \
    iNext &= ( pInfo )->iMax;                                               \
}

#define GetReceivedPacket( pInfo, iNext, pDesc, status )                    \
{                                                                           \
    pDesc = &( pInfo )->pRing[ iNext ];                                     \
    status = LE32_TO_CPU( pDesc->phw->Control.ulData );                     \
}

#define GetRxPacket( pInfo, pDesc )                                         \
{                                                                           \
    pDesc = &( pInfo )->pRing[( pInfo )->iLast ];                           \
    ( pInfo )->iLast++;                                                     \
    ( pInfo )->iLast &= ( pInfo )->iMax;                                    \
    ( pInfo )->cnAvail--;                                                   \
    ( pDesc )->sw.BufSize.ulData &= ~DESC_RX_MASK;                          \
}
#else
void HardwareFreeReceivedPacket (
    PTDescInfo pInfo,
    int*       piNext );

#define FreeReceivedPacket( pInfo, iNext )                                  \
    HardwareFreeReceivedPacket( pInfo, &( iNext ))

PTDesc HardwareGetReceivedPacket (
    PTDescInfo pInfo,
    int        iNext,
    ULONG*     pulData );

#define GetReceivedPacket( pInfo, iNext, pDesc, status )                    \
    pDesc = HardwareGetReceivedPacket( pInfo, iNext, &( status ))

PTDesc HardwareGetRxPacket (
    PTDescInfo pInfo );

#define GetRxPacket( pInfo, pDesc )                                         \
    pDesc = HardwareGetRxPacket( pInfo )
#endif


void HardwareReceive_PCI (
    PHARDWARE pHardware );
#endif

#ifdef KS_PCI_BUS
#define HardwareReceive  HardwareReceive_PCI

#else
#define HardwareReceive  HardwareReceive_ISA
#endif

/* -------------------------------------------------------------------------- */

/*
    Transmit processing secondary routines
*/

#ifdef KS_ISA_BUS
int HardwareAllocPacket_ISA (
    PHARDWARE pHardware,
    int       length );


BOOLEAN HardwareSendPacket_ISA (
    PHARDWARE pHardware );


ULONG HardwareSetTransmitLength_ISA (
    PHARDWARE pHardware,
    int       length );

BOOLEAN HardwareTransmitDone_ISA (
    PHARDWARE pHardware );
#endif

#ifdef KS_PCI_BUS
int HardwareAllocPacket_PCI (
    PHARDWARE pHardware,
    int       length,
    int       physical );

#ifdef INLINE
#define FreeTransmittedPacket( pInfo, iLast )                               \
{                                                                           \
    iLast++;                                                                \
    iLast &= ( pInfo )->iMax;                                               \
    ( pInfo )->cnAvail++;                                                   \
}

#define HardwareFreeTxPacket( pInfo )                                       \
{                                                                           \
    ( pInfo )->iNext--;                                                     \
    ( pInfo )->iNext &= ( pInfo )->iMax;                                    \
    ( pInfo )->cnAvail++;                                                   \
}

#define GetTransmittedPacket( pInfo, iLast, pDesc, status )                 \
{                                                                           \
    pDesc = &( pInfo )->pRing[ iLast ];                                     \
    status = LE32_TO_CPU( pDesc->phw->Control.ulData );                     \
}

#define GetTxPacket( pInfo, pDesc )                                         \
{                                                                           \
    pDesc = &( pInfo )->pRing[( pInfo )->iNext ];                           \
    ( pInfo )->iNext++;                                                     \
    ( pInfo )->iNext &= ( pInfo )->iMax;                                    \
    ( pInfo )->cnAvail--;                                                   \
    pDesc->sw.BufSize.ulData &= ~DESC_TX_MASK;                              \
}

#else
void HardwareFreeTransmittedPacket (
    PTDescInfo pInfo,
    int*       piLast );

#define FreeTransmittedPacket( pInfo, iLast )                               \
    HardwareFreeTransmittedPacket( pInfo, &iLast )

void HardwareFreeTxPacket (
    PTDescInfo pInfo );

PTDesc HardwareGetTransmittedPacket (
    PTDescInfo pInfo,
    int        iLast,
    ULONG*     pulData );

#define GetTransmittedPacket( pInfo, iLast, pDesc, status )                 \
    pDesc = HardwareGetTransmittedPacket( pInfo, iLast, &( status ))

PTDesc HardwareGetTxPacket (
    PTDescInfo pInfo );

#define GetTxPacket( pInfo, pDesc )                                         \
    pDesc = HardwareGetTxPacket( pInfo )

#endif

BOOLEAN HardwareSendPacket_PCI (
    PHARDWARE pHardware );

#define SetTransmitBuffer( pDesc, addr )                                    \
    ( pDesc )->phw->ulBufAddr = CPU_TO_LE32( addr )

#define SetTransmitLength( pDesc, len )                                     \
    ( pDesc )->sw.BufSize.tx.wBufSize = len

#ifdef INLINE
#define HardwareSetTransmitBuffer( pHardware, addr )                        \
    SetTransmitBuffer(( pHardware )->m_TxDescInfo.pCurrent, addr )

#else

void HardwareSetTransmitBuffer (
    PHARDWARE pHardware,
    ULONG     ulBufAddr );
#endif

#define HardwareSetTransmitLength( pHardware, len )                         \
    SetTransmitLength(( pHardware )->m_TxDescInfo.pCurrent, len )

BOOLEAN HardwareTransmitDone_PCI (
    PHARDWARE pHardware );
#endif

#ifdef KS_PCI_BUS
#define HardwareAllocPacket         HardwareAllocPacket_PCI
#define HardwareSendPacket          HardwareSendPacket_PCI
#define HardwareTransmitDone        HardwareTransmitDone_PCI

#else
#define HardwareAllocPacket         HardwareAllocPacket_ISA
#define HardwareSendPacket          HardwareSendPacket_ISA
#define HardwareSetTransmitLength   HardwareSetTransmitLength_ISA
#define HardwareTransmitDone        HardwareTransmitDone_ISA
#endif

/* -------------------------------------------------------------------------- */

/*
    Other secondary routines
*/

unsigned long ether_crc (
    int  length,
    unsigned char *data );

#ifdef KS_ISA_BUS
BOOLEAN HardwareReadAddress_ISA (
    PHARDWARE pHardware );

void HardwareSetAddress_ISA (
    PHARDWARE pHardware );

void HardwareClearMulticast_ISA (
    PHARDWARE pHardware );

BOOLEAN HardwareSetGroupAddress_ISA (
    PHARDWARE pHardware );

BOOLEAN HardwareSetMulticast_ISA (
    PHARDWARE pHardware,
    UCHAR     bMulticast );

BOOLEAN HardwareSetPromiscuous_ISA (
    PHARDWARE pHardware,
    UCHAR     bPromiscuous );
#endif

#ifdef KS_PCI_BUS
BOOLEAN HardwareReadAddress_PCI (
    PHARDWARE pHardware );

void HardwareSetAddress_PCI (
    PHARDWARE pHardware );

void HardwareClearMulticast_PCI (
    PHARDWARE pHardware );

BOOLEAN HardwareSetGroupAddress_PCI (
    PHARDWARE pHardware );

BOOLEAN HardwareSetMulticast_PCI (
    PHARDWARE pHardware,
    UCHAR     bMulticast );

BOOLEAN HardwareSetPromiscuous_PCI (
    PHARDWARE pHardware,
    UCHAR     bPromiscuous );
#endif

#ifdef KS_PCI_BUS
#define HardwareReadAddress         HardwareReadAddress_PCI
#define HardwareSetAddress          HardwareSetAddress_PCI
#define HardwareClearMulticast      HardwareClearMulticast_PCI
#define HardwareSetGroupAddress     HardwareSetGroupAddress_PCI
#define HardwareSetMulticast        HardwareSetMulticast_PCI
#define HardwareSetPromiscuous      HardwareSetPromiscuous_PCI

#else
#define HardwareReadAddress         HardwareReadAddress_ISA
#define HardwareSetAddress          HardwareSetAddress_ISA
#define HardwareClearMulticast      HardwareClearMulticast_ISA
#define HardwareSetGroupAddress     HardwareSetGroupAddress_ISA
#define HardwareSetMulticast        HardwareSetMulticast_ISA
#define HardwareSetPromiscuous      HardwareSetPromiscuous_ISA
#endif

void HardwareAddrFilterClear (
    PHARDWARE  pHardware  );

int  HardwareAddrFilterAdd (
    PHARDWARE  pHardware,
    UCHAR     *pMacAddress );

int  HardwareAddrFilterDel (
    PHARDWARE  pHardware,
    UCHAR     *pMacAddress );


/* -------------------------------------------------------------------------- */

void HardwareClearCounters (
    PHARDWARE pHardware );


/* -------------------------------------------------------------------------- */

#ifdef KS_ISA_BUS
void HardwareGenerateInterrupt (
    PHARDWARE pHardware );

void HardwareServiceInterrupt (
    PHARDWARE pHardware );
#endif

/* -------------------------------------------------------------------------- */

#ifdef KS_ISA_BUS
void HardwareGetCableStatus_ISA (
    PHARDWARE pHardware,
    UCHAR     bPhy,
    void*     pBuffer );

void HardwareGetLinkStatus_ISA (
    PHARDWARE pHardware,
    UCHAR     bPhy,
    void*     pBuffer );

void HardwareSetCapabilities_ISA (
    PHARDWARE pHardware,
    UCHAR     bPhy,
    ULONG     ulCapabilities );
#endif

#ifdef KS_PCI_BUS
void HardwareGetCableStatus_PCI (
    PHARDWARE pHardware,
    UCHAR     bPhy,
    void*     pBuffer );

void HardwareGetLinkStatus_PCI (
    PHARDWARE pHardware,
    UCHAR     bPhy,
    void*     pBuffer );

void HardwareSetCapabilities_PCI (
    PHARDWARE pHardware,
    UCHAR     bPhy,
    ULONG     ulCapabilities );
#endif

#ifdef KS_PCI_BUS
#define HardwareGetCableStatus      HardwareGetCableStatus_PCI
#define HardwareGetLinkStatus       HardwareGetLinkStatus_PCI
#define HardwareSetCapabilities     HardwareSetCapabilities_PCI

#else
#define HardwareGetCableStatus      HardwareGetCableStatus_ISA
#define HardwareGetLinkStatus       HardwareGetLinkStatus_ISA
#define HardwareSetCapabilities     HardwareSetCapabilities_ISA
#endif

/* -------------------------------------------------------------------------- */

#ifdef KS_PCI_BUS
#define ReleaseDescriptor( pDesc, status )                                  \
{                                                                           \
    status.rx.fHWOwned = FALSE;                                             \
    ( pDesc )->phw->Control.ulData = CPU_TO_LE32( status.ulData );          \
}

#define ReleasePacket( pDesc )                                              \
{                                                                           \
    ( pDesc )->sw.Control.tx.fHWOwned = TRUE;                               \
    if ( ( pDesc )->sw.ulBufSize != ( pDesc )->sw.BufSize.ulData )          \
    {                                                                       \
        ( pDesc )->sw.ulBufSize = ( pDesc )->sw.BufSize.ulData;             \
        ( pDesc )->phw->BufSize.ulData =                                    \
            CPU_TO_LE32(( pDesc )->sw.BufSize.ulData );                     \
    }                                                                       \
    ( pDesc )->phw->Control.ulData =                                        \
        CPU_TO_LE32(( pDesc )->sw.Control.ulData );                         \
}

#define SetReceiveBuffer( pDesc, addr )                                     \
    ( pDesc )->phw->ulBufAddr = CPU_TO_LE32( addr )

#define SetReceiveLength( pDesc, len )                                      \
    ( pDesc )->sw.BufSize.rx.wBufSize = len

/* -------------------------------------------------------------------------- */

void HardwareInitDescriptors (
    PTDescInfo pDescInfo,
    int        fTransmit );

void HardwareSetDescriptorBase (
    PHARDWARE  pHardware,
    ULONG      TxDescBaseAddr,
    ULONG      RxDescBaseAddr );

void HardwareResetPackets (
    PTDescInfo pInfo );

#define HardwareResetRxPackets( pHardware )                                 \
    HardwareResetPackets( &pHardware->m_RxDescInfo )

#define HardwareResetTxPackets( pHardware )                                 \
    HardwareResetPackets( &pHardware->m_TxDescInfo )
#endif

/* -------------------------------------------------------------------------- */

void HardwareEnableWolMagicPacket
(
    PHARDWARE pHardware
);

void HardwareEnableWolFrame
(
    PHARDWARE pHardware,
    UINT32    dwFrame
);

void HardwareSetWolFrameCRC
(
    PHARDWARE pHardware,
    UINT32    dwFrame,
    UINT32    dwCRC
);

void HardwareSetWolFrameByteMask
(
    PHARDWARE pHardware,
    UINT32    dwFrame,
    UINT8     bByteMask
);

BOOLEAN HardwareCheckWolPMEStatus
(
    PHARDWARE pHardware
);

void HardwareClearWolPMEStatus
(
    PHARDWARE pHardware
);

/* -------------------------------------------------------------------------- */

#ifdef KS_ISA_BUS
ULONG SwapBytes (
    ULONG dwData );

BOOLEAN PortConfigGet_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bBank,
    UCHAR     bffset,
    UCHAR     bBits );

void PortConfigSet_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bBank,
    UCHAR     bOffset,

#ifdef SH_16BIT_WRITE
    USHORT    bBits,

#else
    UCHAR     bBits,
#endif
    BOOLEAN   fSet );
#endif

#ifdef KS_PCI_BUS
BOOLEAN PortConfigGetShift (
    PHARDWARE pHardware,
    UCHAR     bPort,
    ULONG     ulOffset,
    UCHAR     bShift );

void PortConfigSetShift (
    PHARDWARE pHardware,
    UCHAR     bPort,
    ULONG     ulOffset,
    UCHAR     bShift,
    BOOLEAN   fSet );
#endif

#ifdef KS_ISA_BUS
void PortConfigReadByte_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bBank,
    UCHAR     bOffset,
    PUCHAR    pbData );

void PortConfigWriteByte_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bBank,
    UCHAR     bOffset,
    UCHAR     bData );

void PortConfigReadWord_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bBank,
    UCHAR     bOffset,
    PUSHORT   pwData );

void PortConfigWriteWord_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bBank,
    UCHAR     bOffset,
    USHORT    usData );

BOOLEAN SwitchConfigGet_ISA (
    PHARDWARE pHardware,
    int       Offset,
    UCHAR     bBits );

void SwitchConfigSet_ISA (
    PHARDWARE pHardware,
    int       Offset,

#ifdef SH_16BIT_WRITE
    USHORT    bBits,

#else
    UCHAR     bBits,
#endif
    BOOLEAN   fSet );
#endif

#ifdef KS_PCI_BUS
BOOLEAN PortConfigGet_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bOffset,
    UCHAR     bBits );

void PortConfigSet_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bOffset,
    UCHAR     bBits,
    BOOLEAN   fSet );

void PortConfigReadByte_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bOffset,
    PUCHAR    pbData );

void PortConfigWriteByte_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bOffset,
    UCHAR     bData );

void PortConfigReadWord_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bOffset,
    PUSHORT   pwData );

void PortConfigWriteWord_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bOffset,
    USHORT    usData );

BOOLEAN SwitchConfigGet_PCI (
    PHARDWARE pHardware,
    int       Offset,
    UCHAR     bBits );

void SwitchConfigSet_PCI (
    PHARDWARE pHardware,
    int       Offset,
    UCHAR     bBits,
    BOOLEAN   fSet );
#endif

#ifdef KS_PCI_BUS
#define SwitchConfigReadByte( phwi, offset, data )                          \
    HW_READ_BYTE( phwi, offset, data )

#define SwitchConfigWriteByte( phwi, offset, data )                         \
    HW_WRITE_BYTE( phwi, offset, data )

#define PortConfigGet  PortConfigGet_PCI
#define PortConfigSet  PortConfigSet_PCI
#define PortConfigReadByte  PortConfigReadByte_PCI
#define PortConfigWriteByte  PortConfigWriteByte_PCI
#define PortConfigReadWord  PortConfigReadWord_PCI
#define PortConfigWriteWord  PortConfigWriteWord_PCI
#define SwitchConfigGet  SwitchConfigGet_PCI
#define SwitchConfigSet  SwitchConfigSet_PCI

#else
#define SwitchConfigReadByte( phwi, offset, data )                          \
    HardwareReadRegByte( phwi, REG_SWITCH_CTRL_BANK, offset, data )

#define SwitchConfigWriteByte( phwi, offset, data )                         \
    HardwareWriteRegByte( phwi, REG_SWITCH_CTRL_BANK, offset, data )

#define PortConfigGet PortConfigGet_ISA
#define PortConfigSet  PortConfigSet_ISA
#define PortConfigReadByte  PortConfigReadByte_ISA
#define PortConfigWriteByte  PortConfigWriteByte_ISA
#define PortConfigReadWord  PortConfigReadWord_ISA
#define PortConfigWriteWord  PortConfigWriteWord_ISA
#define SwitchConfigGet  SwitchConfigGet_ISA
#define SwitchConfigSet  SwitchConfigSet_ISA
#endif

/* -------------------------------------------------------------------------- */

/* Bandwidth */

#ifdef KS_PCI_BUS
#define PortConfigBroadcastStorm( phwi, port, enable )                      \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_1_OFFSET, PORT_BROADCAST_STORM, enable )

#define PortGetBroadcastStorm( phwi, port )                                 \
    PortConfigGet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_1_OFFSET, PORT_BROADCAST_STORM )

#else
#define PortConfigBroadcastStorm( phwi, port, enable )                      \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_1_OFFSET, PORT_BROADCAST_STORM, enable )

#define PortGetBroadcastStorm( phwi, port )                                 \
    PortConfigGet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_1_OFFSET, PORT_BROADCAST_STORM )
#endif


/* Communication */

#ifdef KS_PCI_BUS
#define PortConfigBackPressure( phwi, port, enable )                        \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_BACK_PRESSURE, enable )

#define PortConfigForceFlowCtrl( phwi, port, enable )                       \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_FORCE_FLOW_CTRL, enable )

#define PortGetBackPressure( phwi, port )                                   \
    PortConfigGet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_BACK_PRESSURE )

#define PortGetForceFlowCtrl( phwi, port )                                  \
    PortConfigGet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_FORCE_FLOW_CTRL )

#else
#define PortConfigBackPressure( phwi, port, enable )                        \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_BACK_PRESSURE, enable )

#define PortConfigForceFlowCtrl( phwi, port, enable )                       \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_FORCE_FLOW_CTRL, enable )

#define PortGetBackPressure( phwi, port )                                   \
    PortConfigGet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_BACK_PRESSURE )

#define PortGetForceFlowCtrl( phwi, port )                                  \
    PortConfigGet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_FORCE_FLOW_CTRL )
#endif


/* Spanning Tree */

#ifdef KS_PCI_BUS
#define PortConfigDisableLearning( phwi, port, enable )                     \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_LEARN_DISABLE, enable )

#define PortConfigEnableReceive( phwi, port, enable )                       \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_RX_ENABLE, enable )

#define PortConfigEnableTransmit( phwi, port, enable )                      \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_TX_ENABLE, enable )

#else
#define PortConfigDisableLearning( phwi, port, enable )                     \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_LEARN_DISABLE, enable )

#define PortConfigEnableReceive( phwi, port, enable )                       \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_RX_ENABLE, enable )

#define PortConfigEnableTransmit( phwi, port, enable )                      \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_TX_ENABLE, enable )
#endif


/* VLAN */

#ifdef KS_PCI_BUS
#define PortConfigDiscardNonVID( phwi, port, enable )                       \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_DISCARD_NON_VID, enable )

#define PortConfigDoubleTag( phwi, port, enable )                           \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_OFFSET, PORT_DOUBLE_TAG, enable )

#define PortConfigIngressFiltering( phwi, port, enable )                    \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_INGRESS_FILTER, enable )

#define PortConfigInsertTag( phwi, port, insert )                           \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_1_OFFSET, PORT_INSERT_TAG, insert )

#define PortConfigRemoveTag( phwi, port, remove )                           \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_1_OFFSET, PORT_REMOVE_TAG, remove )

#define PortGetDiscardNonVID( phwi, port )                                  \
    PortConfigGet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_DISCARD_NON_VID )

#define PortGetDoubleTag( phwi, port )                                      \
    PortConfigGet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_OFFSET, PORT_DOUBLE_TAG )

#define PortGetIngressFiltering( phwi, port )                               \
    PortConfigGet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_INGRESS_FILTER )

#define PortGetInsertTag( phwi, port )                                      \
    PortConfigGet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_1_OFFSET, PORT_INSERT_TAG )

#define PortGetRemoveTag( phwi, port )                                      \
    PortConfigGet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_1_OFFSET, PORT_REMOVE_TAG )

#else
#define PortConfigDiscardNonVID( phwi, port, enable )                       \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_DISCARD_NON_VID, enable )

#define PortConfigDoubleTag( phwi, port, enable )                           \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_OFFSET, PORT_DOUBLE_TAG, enable )

#define PortConfigIngressFiltering( phwi, port, enable )                    \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_INGRESS_FILTER, enable )

#define PortConfigInsertTag( phwi, port, insert )                           \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_1_OFFSET, PORT_INSERT_TAG, insert )

#define PortConfigRemoveTag( phwi, port, remove )                           \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_1_OFFSET, PORT_REMOVE_TAG, remove )

#define PortGetDiscardNonVID( phwi, port )                                  \
    PortConfigGet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_DISCARD_NON_VID )

#define PortGetDoubleTag( phwi, port )                                      \
    PortConfigGet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_OFFSET, PORT_DOUBLE_TAG )

#define PortGetIngressFiltering( phwi, port )                               \
    PortConfigGet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_HI_OFFSET, PORT_INGRESS_FILTER )

#define PortGetInsertTag( phwi, port )                                      \
    PortConfigGet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_1_OFFSET, PORT_INSERT_TAG )

#define PortGetRemoveTag( phwi, port )                                      \
    PortConfigGet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_1_OFFSET, PORT_REMOVE_TAG )
#endif


/* Mirroring */

#ifdef KS_PCI_BUS
#define PortConfigMirrorSniffer( phwi, port, enable )                       \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_OFFSET, PORT_MIRROR_SNIFFER, enable )

#define PortConfigMirrorReceive( phwi, port, enable )                       \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_OFFSET, PORT_MIRROR_RX, enable )

#define PortConfigMirrorTransmit( phwi, port, enable )                      \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_OFFSET, PORT_MIRROR_TX, enable )

#define SwitchConfigMirrorRxAndTx( phwi, enable )                           \
    SwitchConfigSet_PCI( phwi,                                              \
        REG_SWITCH_CTRL_2_HI_OFFSET, SWITCH_MIRROR_RX_TX, enable )

#else
#define PortConfigMirrorSniffer( phwi, port, enable )                       \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_OFFSET, PORT_MIRROR_SNIFFER, enable )

#define PortConfigMirrorReceive( phwi, port, enable )                       \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_OFFSET, PORT_MIRROR_RX, enable )

#define PortConfigMirrorTransmit( phwi, port, enable )                      \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_OFFSET, PORT_MIRROR_TX, enable )

#define SwitchConfigMirrorRxAndTx( phwi, enable )                           \
    SwitchConfigSet_ISA( phwi,                                              \
        REG_SWITCH_CTRL_2_HI_OFFSET, SWITCH_MIRROR_RX_TX, enable )
#endif


/* Priority */

#ifdef KS_PCI_BUS
#define PortConfigDiffServ( phwi, port, enable )                            \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_1_OFFSET, PORT_DIFFSERV_ENABLE, enable )

#define PortConfig802_1P( phwi, port, enable )                              \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_1_OFFSET, PORT_802_1P_ENABLE, enable )

#define PortConfigPriority( phwi, port, enable )                            \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_1_OFFSET, PORT_PRIORITY_ENABLE, enable )

#define PortConfig802_1P_Remapping( phwi, port, enable )                    \
    PortConfigSet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_2_OFFSET, PORT_802_1P_REMAPPING, enable )

#define PortGetDiffServ( phwi, port )                                       \
    PortConfigGet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_1_OFFSET, PORT_DIFFSERV_ENABLE )

#define PortGet802_1P( phwi, port )                                         \
    PortConfigGet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_1_OFFSET, PORT_802_1P_ENABLE )

#define PortGetPriority( phwi, port )                                       \
    PortConfigGet_PCI( phwi, port,                                          \
        REG_PORT_CTRL_1_OFFSET, PORT_PRIORITY_ENABLE )

#else
#define PortConfigDiffServ( phwi, port, enable )                            \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_1_OFFSET, PORT_DIFFSERV_ENABLE, enable )

#define PortConfig802_1P( phwi, port, enable )                              \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_1_OFFSET, PORT_802_1P_ENABLE, enable )

#define PortConfigPriority( phwi, port, enable )                            \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_1_OFFSET, PORT_PRIORITY_ENABLE, enable )

#define PortConfig802_1P_Remapping( phwi, port, enable )                    \
    PortConfigSet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_2_OFFSET, PORT_802_1P_REMAPPING, enable )

#define PortGetDiffServ( phwi, port )                                       \
    PortConfigGet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_1_OFFSET, PORT_DIFFSERV_ENABLE )

#define PortGet802_1P( phwi, port )                                         \
    PortConfigGet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_1_OFFSET, PORT_802_1P_ENABLE )

#define PortGetPriority( phwi, port )                                       \
    PortConfigGet_ISA( phwi, port, REG_PORT_CTRL_BANK,                      \
        REG_PORT_CTRL_1_OFFSET, PORT_PRIORITY_ENABLE )
#endif

/* -------------------------------------------------------------------------- */

/* ks_config.c */

#ifdef KS_PCI_BUS
void SwitchGetAddress_PCI (
#else
void SwitchGetAddress_ISA (
#endif
    PHARDWARE pHardware,
    PUCHAR    MacAddr );

#ifdef KS_PCI_BUS
void SwitchSetAddress_PCI (
#else
void SwitchSetAddress_ISA (
#endif
    PHARDWARE pHardware,
    PUCHAR    MacAddr );

#ifdef KS_PCI_BUS
void SwitchGetLinkStatus_PCI (
#else
void SwitchGetLinkStatus_ISA (
#endif
    PHARDWARE pHardware );

#ifdef KS_PCI_BUS
void SwitchSetLinkSpeed_PCI (
#else
void SwitchSetLinkSpeed_ISA (
#endif
    PHARDWARE pHardware );

#ifdef KS_PCI_BUS
void SwitchSetGlobalControl_PCI (
#else
void SwitchSetGlobalControl_ISA (
#endif
    PHARDWARE pHardware );

#ifdef KS_PCI_BUS
void SwitchRestartAutoNego_PCI (
#else
void SwitchRestartAutoNego_ISA (
#endif
    PHARDWARE pHardware );

#ifdef KS_PCI_BUS
void SwitchEnable_PCI (
#else
void SwitchEnable_ISA (
#endif
    PHARDWARE, BOOLEAN );


#ifdef KS_PCI_BUS
#define SwitchGetAddress  SwitchGetAddress_PCI
#define SwitchSetAddress  SwitchSetAddress_PCI
#define SwitchGetLinkStatus  SwitchGetLinkStatus_PCI
#define SwitchSetLinkSpeed   SwitchSetLinkSpeed_PCI
#define SwitchSetGlobalControl  SwitchSetGlobalControl_PCI
#define SwitchRestartAutoNego   SwitchRestartAutoNego_PCI
#define SwitchEnable  SwitchEnable_PCI

#else
#define SwitchGetAddress  SwitchGetAddress_ISA
#define SwitchSetAddress  SwitchSetAddress_ISA
#define SwitchGetLinkStatus  SwitchGetLinkStatus_ISA
#define SwitchSetLinkSpeed   SwitchSetLinkSpeed_ISA
#define SwitchSetGlobalControl  SwitchSetGlobalControl_ISA
#define SwitchRestartAutoNego   SwitchRestartAutoNego_ISA
#define SwitchEnable  SwitchEnable_ISA
#endif

/* -------------------------------------------------------------------------- */

/* ks_mirror.c */

#ifdef KS_ISA
void SwitchDisableMirrorSniffer_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchEnableMirrorSniffer_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchDisableMirrorReceive_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchEnableMirrorReceive_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchDisableMirrorTransmit_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchEnableMirrorTransmit_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchDisableMirrorRxAndTx_ISA (
    PHARDWARE pHardware );

void SwitchEnableMirrorRxAndTx_ISA (
    PHARDWARE pHardware );

#endif

#ifdef KS_PCI
void SwitchDisableMirrorSniffer_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchEnableMirrorSniffer_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchDisableMirrorReceive_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchEnableMirrorReceive_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchDisableMirrorTransmit_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchEnableMirrorTransmit_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchDisableMirrorRxAndTx_PCI (
    PHARDWARE pHardware );

void SwitchEnableMirrorRxAndTx_PCI (
    PHARDWARE pHardware );

#endif

void SwitchInitMirror (
    PHARDWARE pHardware );

#define SwitchDisableMirrorSniffer( phwi, port )                            \
    ( phwi )->m_hwfn->fnSwitchDisableMirrorSniffer( phwi, port )

#define SwitchEnableMirrorSniffer( phwi, port )                             \
    ( phwi )->m_hwfn->fnSwitchEnableMirrorSniffer( phwi, port )

#define SwitchDisableMirrorReceive( phwi, port )                            \
    ( phwi )->m_hwfn->fnSwitchDisableMirrorReceive( phwi, port )

#define SwitchEnableMirrorReceive( phwi, port )                             \
    ( phwi )->m_hwfn->fnSwitchEnableMirrorReceive( phwi, port )

#define SwitchDisableMirrorTransmit( phwi, port )                           \
    ( phwi )->m_hwfn->fnSwitchDisableMirrorTransmit( phwi, port )

#define SwitchEnableMirrorTransmit( phwi, port )                            \
    ( phwi )->m_hwfn->fnSwitchEnableMirrorTransmit( phwi, port )

#define SwitchDisableMirrorRxAndTx( phwi )                                  \
    ( phwi )->m_hwfn->fnSwitchDisableMirrorRxAndTx( phwi )

#define SwitchEnableMirrorRxAndTx( phwi )                                   \
    ( phwi )->m_hwfn->fnSwitchEnableMirrorRxAndTx( phwi )


/* -------------------------------------------------------------------------- */

/* ks_qos.c */

#ifdef KS_ISA
void HardwareConfig_TOS_Priority_ISA (
    PHARDWARE pHardware,
    UCHAR     bTosValue,
    USHORT    wPriority );

void SwitchDisableDiffServ_ISA (
    PHARDWARE pHardware,
    UCHAR bPort );

void SwitchEnableDiffServ_ISA (
    PHARDWARE pHardware ,
    UCHAR bPort );


void HardwareConfig802_1P_Priorit_ISA (
    PHARDWARE pHardware,
    UCHAR     bTagPriorityValue,
    USHORT    wPriority );

void SwitchDisable802_1P_ISA (
    PHARDWARE pHardware,
    UCHAR bPort );

void SwitchEnable802_1P_ISA (
    PHARDWARE pHardware,
    UCHAR bPort );

void SwitchDisableDot1pRemapping_ISA (
    PHARDWARE pHardware,
    UCHAR bPort );

void SwitchEnableDot1pRemapping_ISA (
    PHARDWARE pHardware,
    UCHAR bPort );


void SwitchConfigPortBased_ISA (
    PHARDWARE pHardware,
    UCHAR     bPriority,
    UCHAR     bPort );


void SwitchDisableMultiQueue_ISA (
    PHARDWARE pHardware,
    UCHAR bPort );

void SwitchEnableMultiQueue_ISA (
    PHARDWARE pHardware,
    UCHAR bPort );

#endif

#ifdef KS_PCI
void HardwareConfig_TOS_Priority_PCI (
    PHARDWARE pHardware,
    UCHAR     bTosValue,
    USHORT    wPriority );

void SwitchDisableDiffServ_PCI (
    PHARDWARE pHardware ,
    UCHAR bPort );

void SwitchEnableDiffServ_PCI (
    PHARDWARE pHardware ,
    UCHAR bPort );


void HardwareConfig802_1P_Priorit_PCI (
    PHARDWARE pHardware,
    UCHAR     bTagPriorityValue,
    USHORT    wPriority );

void SwitchDisable802_1P_PCI (
    PHARDWARE pHardware,
    UCHAR bPort );

void SwitchEnable802_1P_PCI (
    PHARDWARE pHardware,
    UCHAR bPort );

void SwitchDisableDot1pRemapping_PCI (
    PHARDWARE pHardware,
    UCHAR bPort );

void SwitchEnableDot1pRemapping_PCI (
    PHARDWARE pHardware,
    UCHAR bPort );


void SwitchConfigPortBased_PCI (
    PHARDWARE pHardware,
    UCHAR     bPriority,
    UCHAR     bPort );


void SwitchDisableMultiQueue_PCI (
    PHARDWARE pHardware,
    UCHAR bPort );

void SwitchEnableMultiQueue_PCI (
    PHARDWARE pHardware,
    UCHAR bPort );

#endif


#define HardwareConfig_TOS_Priority( phwi, p0, p1 )                         \
    ( phwi )->m_hwfn->fnHardwareConfig_TOS_Priority( phwi, p0, p1 )

#define SwitchDisableDiffServ( phwi, p0 )                                   \
    ( phwi )->m_hwfn->fnSwitchDisableDiffServ( phwi, p0 )

#define SwitchEnableDiffServ( phwi, p0 )                                    \
    ( phwi )->m_hwfn->fnSwitchEnableDiffServ( phwi, p0 )

#define HardwareConfig802_1P_Priority( phwi, p0, p1 )                       \
    ( phwi )->m_hwfn->fnHardwareConfig802_1P_Priority( phwi, p0, p1 )

#define SwitchDisable802_1P( phwi, p0 )                                     \
    ( phwi )->m_hwfn->fnSwitchDisable802_1P( phwi, p0 )

#define SwitchEnable802_1P( phwi, p0 )                                      \
    ( phwi )->m_hwfn->fnSwitchEnable802_1P( phwi, p0 )

#define SwitchDisableDot1pRemapping( phwi, p0 )                             \
    ( phwi )->m_hwfn->fnSwitchDisableDot1pRemapping( phwi, p0 )

#define SwitchEnableDot1pRemapping( phwi, p0 )                              \
    ( phwi )->m_hwfn->fnSwitchEnableDot1pRemapping( phwi, p0 )

#define SwitchConfigPortBased( phwi, p0, p1 )                               \
    ( phwi )->m_hwfn->fnSwitchConfigPortBased( phwi, p0, p1 )

void SwitchInitPriority (
    PHARDWARE pHardware );

#define SwitchDisableMultiQueue( phwi, p0 )                                 \
    ( phwi )->m_hwfn->fnSwitchDisableMultiQueue( phwi, p0 )

#define SwitchEnableMultiQueue( phwi, p0 )                                  \
    ( phwi )->m_hwfn->fnSwitchEnableMultiQueue( phwi, p0 )

/* -------------------------------------------------------------------------- */

/* ks_rate.c */

#ifdef KS_ISA
void SwitchDisableBroadcastStorm_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchEnableBroadcastStorm_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort );
#endif

#ifdef KS_PCI
void SwitchDisableBroadcastStorm_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchEnableBroadcastStorm_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort );
#endif

void SwitchInitBroadcastStorm (
    PHARDWARE pHardware );

#ifdef KS_ISA
void HardwareConfigBroadcastStorm_ISA (
    PHARDWARE pHardware,
    int       percent );


void SwitchDisablePriorityRate_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchEnablePriorityRate_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort );
#endif

#ifdef KS_PCI
void HardwareConfigBroadcastStorm_PCI (
    PHARDWARE pHardware,
    int       percent );


void SwitchDisablePriorityRate_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort );

void SwitchEnablePriorityRate_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort );
#endif

void SwitchInitPriorityRate (
    PHARDWARE pHardware );

#ifdef KS_ISA
void HardwareConfigRxPriorityRate_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bPriority,
    ULONG     dwBytes );

void HardwareConfigTxPriorityRate_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bPriority,
    ULONG     dwBytes );
#endif

#ifdef KS_PCI
void HardwareConfigRxPriorityRate_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bPriority,
    ULONG     dwBytes );

void HardwareConfigTxPriorityRate_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bPriority,
    ULONG     dwBytes );
#endif

#define SwitchDisableBroadcastStorm( phwi, port )                           \
    ( phwi )->m_hwfn->fnSwitchDisableBroadcastStorm( phwi, port )

#define SwitchEnableBroadcastStorm( phwi, port )                            \
    ( phwi )->m_hwfn->fnSwitchEnableBroadcastStorm( phwi, port )

#define HardwareConfigBroadcastStorm( phwi, percent )                       \
    ( phwi )->m_hwfn->fnHardwareConfigBroadcastStorm( phwi, percent )


#define SwitchDisablePriorityRate( phwi, port )                             \
    ( phwi )->m_hwfn->fnSwitchDisablePriorityRate( phwi, port )

#define SwitchEnablePriorityRate( phwi, port )                              \
    ( phwi )->m_hwfn->fnSwitchEnablePriorityRate( phwi, port )

#define HardwareConfigRxPriorityRate( phwi, port, priority, bytes )         \
    ( phwi )->m_hwfn->fnHardwareConfigRxPriorityRate( phwi, port,           \
        priority, bytes )

#define HardwareConfigTxPriorityRate( phwi, port, priority, bytes )         \
    ( phwi )->m_hwfn->fnHardwareConfigTxPriorityRate( phwi, port,           \
        priority, bytes )

/* -------------------------------------------------------------------------- */

/* ks_stp.c */

#ifdef KS_ISA
void PortSet_STP_State_ISA (
    PHARDWARE pHardware,
    UCHAR     bPort,
    int       nState );
#endif

#ifdef KS_PCI
void PortSet_STP_State_PCI (
    PHARDWARE pHardware,
    UCHAR     bPort,
    int       nState );
#endif

#define PortSet_STP_State( phwi, port, state )                              \
    ( phwi )->m_hwfn->fnPortSet_STP_State( phwi, port, state )

void HardwareInit_STP (
    PHARDWARE pHardware );

/* -------------------------------------------------------------------------- */

/* ks_table.c */

#ifdef KS_ISA
BOOLEAN SwitchReadDynMacTable_ISA (
    PHARDWARE pHardware,
    USHORT    wAddr,
    PUCHAR    MacAddr,
    PUCHAR    pbFID,
    PUCHAR    pbSrcPort,
    PUCHAR    pbTimestamp,
    PUSHORT   pwEntries );

#define SwitchReadDynMacTable     SwitchReadDynMacTable_ISA

#endif

#ifdef KS_PCI
BOOLEAN SwitchReadDynMacTable_PCI (
    PHARDWARE pHardware,
    USHORT    wAddr,
    PUCHAR    MacAddr,
    PUCHAR    pbFID,
    PUCHAR    pbSrcPort,
    PUCHAR    pbTimestamp,
    PUSHORT   pwEntries );

#define SwitchReadDynMacTable     SwitchReadDynMacTable_PCI

#endif

BOOLEAN SwitchReadStaticMacTable (
    PHARDWARE pHardware,
    USHORT    wAddr,
    PUCHAR    MacAddr,
    PUCHAR    pbPorts,
    PBOOLEAN  pfOverride,
    PBOOLEAN  pfUseFID,
    PUCHAR    pbFID );

void SwitchWriteStaticMacTable (
    PHARDWARE pHardware,
    USHORT    wAddr,
    PUCHAR    MacAddr,
    UCHAR     bPorts,
    BOOLEAN   fOverride,
    BOOLEAN   fValid,
    BOOLEAN   fUseFID,
    UCHAR     bFID );

BOOLEAN SwitchReadVlanTable (
    PHARDWARE pHardware,
    USHORT    wAddr,
    PUSHORT   pwVID,
    PUCHAR    pbFID,
    PUCHAR    pbMember );

void SwitchWriteVlanTable (
    PHARDWARE pHardware,
    USHORT    wAddr,
    USHORT    wVID,
    UCHAR     bFID,
    UCHAR     bMember,
    BOOLEAN   fValid );

#ifdef KS_ISA
void PortReadMIBCounter_ISA (
    PHARDWARE  pHardware,
    UCHAR      bPort,
    USHORT     wAddr,
    PULONGLONG pqData );

void PortReadMIBPacket_ISA (
    PHARDWARE  pHardware,
    UCHAR      bPort,
    PULONG     pdwCurrent,
    PULONGLONG pqData );
#endif

#ifdef KS_PCI
void PortReadMIBCounter_PCI (
    PHARDWARE  pHardware,
    UCHAR      bPort,
    USHORT     wAddr,
    PULONGLONG pqData );

void PortReadMIBPacket_PCI (
    PHARDWARE  pHardware,
    UCHAR      bPort,
    PULONG     pdwCurrent,
    PULONGLONG pqData );
#endif

#define PortReadMIBCounter( phwi, port, counter, data )                     \
    ( phwi )->m_hwfn->fnPortReadMIBCounter( phwi, port, counter, data )

#define PortReadMIBPacket( phwi, port, counter, data )                      \
    ( phwi )->m_hwfn->fnPortReadMIBPacket( phwi, port, counter, data )


void PortInitCounters (
    PHARDWARE pHardware,
    UCHAR     bPort );

void PortReadCounters (
    PHARDWARE pHardware,
    UCHAR     bPort );

/* -------------------------------------------------------------------------- */

/* ks_vlan.c */

void SwitchDisableVlan (
    PHARDWARE pHardware );

#ifdef KS_ISA
void SwitchEnableVlan_ISA (
    PHARDWARE pHardware );
#endif

#ifdef KS_PCI
void SwitchEnableVlan_PCI (
    PHARDWARE pHardware );
#endif

#define SwitchEnableVlan( phwi )                                            \
    ( phwi )->m_hwfn->fnSwitchEnableVlan( phwi )

void SwitchInitVlan (
    PHARDWARE pHardware );

void HardwareConfigDefaultVID (
    PHARDWARE pHardware,
    UCHAR     bPort,
    USHORT    wVID );

void HardwareConfigPortBaseVlan
(
    PHARDWARE pHardware,
    UCHAR     bPort,
    UCHAR     bMember
);

void SwitchVlanConfigDiscardNonVID
(
    PHARDWARE pHardware,
    UCHAR     bPort,
    BOOLEAN   fSet
);

void SwitchVlanConfigIngressFiltering
(
    PHARDWARE pHardware,
    UCHAR     bPort,
    BOOLEAN   fSet
);

void SwitchVlanConfigInsertTag
(
    PHARDWARE pHardware,
    UCHAR     bPort,
    BOOLEAN   fSet
);

void SwitchVlanConfigRemoveTag
(
    PHARDWARE pHardware,
    UCHAR     bPort,
    BOOLEAN   fSet
);

/* -------------------------------------------------------------------------- */

#endif  /* _KS884X_H_ */
