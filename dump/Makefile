ifeq (,$(filter build-%,$(notdir $(CURDIR))))
include target.mk
else
#----- End Boilerplate

VPATH = $(SRCDIR)

sources = dump.cpp
objects = $(patsubst %.cpp,%.o,$(sources))
target = asebadump

CXXFLAGS +=-g -Wall
CXX = $(prefix)g++
LDFLAGS = -L$(SRCDIR)/../msg/build-$(builddir)/ -L$(SRCDIR)/../utils/build-$(builddir)/
LDLIBS = -lasebamsg -lasebautils -ldasheld

$(target): $(objects)
	$(CXX) $(objects) $(LDFLAGS) $(LDLIBS) -o $@

%.d: %.cpp
	set -e; $(CC) -MM $(CPPFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@

include $(sources:.cpp=.d)

#----- Begin Boilerplate
endif
