LIBSRC = lexer.cpp parser.cpp tree-build.cpp tree-dump.cpp tree-optimize.cpp tree-emit.cpp errors.cpp compiler.cpp
LIBOBJ = $(patsubst %.cpp,%.o,$(LIBSRC))
LIBTARGET = libaeslc.a
INC = compiler.h tree.h power-of-two.h
CXXFLAGS =-g -Wall

${LIBTARGET}: ${LIBOBJ} ${INC}
	ar rsc $@ ${LIBOBJ}

clean:
	rm -f ${LIBOBJ} *~ depend

distclean: clean
	rm -f ${LIBTARGET}

depend: Makefile
	${CXX} -MM ${CFLAGS} ${LIBSRC} > $@

include depend
