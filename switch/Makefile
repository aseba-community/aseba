ifeq (,$(filter build-%,$(notdir $(CURDIR))))
include target.mk
else
#----- End Boilerplate

VPATH = $(SRCDIR)

sources = switch.cpp
objects = $(patsubst %.cpp,%.o,$(sources))
target = asebaswitch

CXXFLAGS +=-g -Wall `pkg-config --cflags hal` 
CXX = $(prefix)g++
LDFLAGS = -L$(SRCDIR)/../utils/build-$(builddir)/ -L$(SRCDIR)/../can/build-$(builddir)/ 
LDLIBS = -lasebautils -lasebacan -ldasheld `pkg-config --libs hal` 

$(target): $(objects)
	$(CXX) $(objects) $(LDFLAGS) $(LDLIBS) -o $@

%.d: %.cpp
	set -e; $(CC) -MM $(CPPFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@

include $(sources:.cpp=.d)

#----- Begin Boilerplate
endif
