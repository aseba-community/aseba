ifeq (,$(filter build-%,$(notdir $(CURDIR))))
include target.mk
else
#----- End Boilerplate

VPATH = $(SRCDIR)

sources = utils.cpp network.cpp FormatableString.cpp
objects = $(patsubst %.cpp,%.o,$(sources))
target = libasebautils.a

CXXFLAGS +=-g -Wall
CXX = $(prefix)g++

$(target): $(objects)
	$(prefix)ar rsc $@ $(objects)

%.d: %.cpp
	set -e; $(CXX) -MM $(CPPFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@

include $(sources:.cpp=.d)

#----- Begin Boilerplate
endif
