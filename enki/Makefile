ifeq (,$(filter build-%,$(notdir $(CURDIR))))
include target.mk
else
#----- End Boilerplate

VPATH = $(SRCDIR)

sources = AsebaMarxbot.cpp EnkiTest.cpp
objects = $(patsubst %.cpp,%.o,$(sources))
target = enkiTest

CXXFLAGS +=-g -Wall
CXX = $(prefix)g++
LDFLAGS += -L$(SRCDIR)/../vm/build-$(builddir)/ -L$(SRCDIR)/../utils/build-$(builddir)/
LDLIBS = -lasebavm -lasebautils -lenki


$(target): $(objects)
	$(CXX) $(objects) $(LDFLAGS) $(LDLIBS) -o $@

%.d: %.cpp
	set -e; $(CXX) -MM $(CPPFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@

include $(sources:.cpp=.d)

#----- Begin Boilerplate
endif
