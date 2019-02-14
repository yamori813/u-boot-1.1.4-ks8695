#
# (C) Copyright 2000-2005
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= lib$(SOC).a

OBJS	= interrupts.o serial.o 
SOBJS	= lowlevel_init.o

all:	.depend $(LIB) nand-large.bin nand-small.bin

$(LIB):	$(OBJS) $(SOBJS)
	$(AR) crv $@ $(OBJS) $(SOBJS)

prebootS.S:	boot2S
	$(TOPDIR)/tools/code_convert boot2S prebootS.S

boot2S:		boot2S.S
	arm-linux-gcc -I${TOPDIR}/include -c -o boot2S boot2S.S
	arm-linux-objcopy -O binary boot2S

nand-small.bin:		boot1S.S prebootS.S
	arm-linux-gcc -I${TOPDIR}/include -c -o boot1S boot1S.S
	arm-linux-objcopy -O binary boot1S nand-small.bin
	cp nand-small.bin $(TOPDIR)/.

prebootL.S:	boot2L
	$(TOPDIR)/tools/code_convert boot2L prebootL.S

boot2L:		boot2L.S
	arm-linux-gcc -I${TOPDIR}/include -c -o boot2L boot2L.S
	arm-linux-objcopy -O binary boot2L

nand-large.bin:		boot1L.S prebootL.S
	arm-linux-gcc -I${TOPDIR}/include -c -o boot1L boot1L.S
	arm-linux-objcopy -O binary boot1L nand-large.bin
	cp nand-large.bin $(TOPDIR)/.

#########################################################################

.depend:	Makefile $(OBJS:.o=.c)
		$(CC) -M $(CFLAGS) $(OBJS:.o=.c) > $@

sinclude .depend

#########################################################################
