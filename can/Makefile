ifeq (,$(filter build-%,$(notdir $(CURDIR))))
include target.mk
else
#----- End Boilerplate

VPATH = $(SRCDIR)

sources = can.c
objects = $(patsubst %.c,%.o,$(sources))
target = libasebacan.a

CFLAGS +=-g -Wall
CC = $(prefix)gcc

$(target): $(objects)
	$(prefix)ar rsc $@ $(objects)

%.d: %.c
	set -e; $(CC) -MM $(CPPFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@

include $(sources:.c=.d)

#----- Begin Boilerplate
endif
