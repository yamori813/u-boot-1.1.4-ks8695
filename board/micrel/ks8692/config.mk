# Manually set the TEXT_BASE depending on whether U-Boot is to be burned to the
# EEPROM or downloaded.

sinclude $(TOPDIR)/board/$(VENDOR)/config.tmp

ifdef CONFIG_SKIP_LOWLEVEL_INIT
TEXT_BASE = 0x01000000
else
TEXT_BASE = 0x00F00000
endif
