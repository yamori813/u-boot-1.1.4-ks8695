sinclude $(TOPDIR)/board/$(VENDOR)/config.tmp

ifdef CONFIG_SKIP_LOWLEVEL_INIT
TEXT_BASE = 0x00800000
else
TEXT_BASE = 0x00f00000
endif
