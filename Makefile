SHELL = /bin/sh

.SUFFIXES:

all:

include doc/Makefile

clean:
	$(MAKE) -C doc clean

dvi:
	$(MAKE) -C doc diplomski_rad.dvi

html:

pdf:
	$(MAKE) -C doc

ps:
	$(MAKE) -C doc diplomski_rad.ps

