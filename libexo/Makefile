## these are set in ../Makefile
## CC, AR, AROPT, CFLAGS, RANLIB

LEX	= flex

#
# all the sources
#
SRC	= libexo.c exolex.l
HDR	= libexo.h

#
# common objects
#
OBJ	= libexo.$(OEXT) exolex.$(OEXT)

#
# external libraries required for build
#
LIB	=

#
# all targets
#
all: libexo.$(LEXT) # exo-test
	@echo "my work is done here..."

exo-test:	exo-test.$(OEXT) options.$(OEXT) stats.$(OEXT) eval.$(OEXT) misc.$(OEXT) libexo.$(LEXT)
	$(CC) -o exo-test $(CFLAGS) exo-test.$(OEXT) options.$(OEXT) \
	stats.$(OEXT) eval.$(OEXT) misc.$(OEXT) libexo.$(LEXT) $(LIB) -lm

libexo.$(LEXT):	$(OBJ)
	$(RM) libexo.$(LEXT)
	$(AR) $(AROPT)libexo.$(LEXT) $(OBJ)
	$(RANLIB) libexo.$(LEXT)

.c.$(OEXT):
	$(CC) $(CFLAGS) -c $*.c

exolex.c:	exolex.l
	$(LEX) exolex.l
	mv -f lex.yy.c exolex.c

filelist:
	@echo $(SRC) $(HDR) Makefile

diffs:
	-rcsdiff RCS/*

clean:
	-$(RM) *.o *.obj core *~ Makefile.bak exo-test libexo.a libexo.lib

unpure:
	rm -f sim.pure *pure*.o sim.pure.pure_hardlink sim.pure.pure_linkinfo

depend:
	makedepend.local -n -x $(SRC)


# DO NOT DELETE THIS LINE -- make depend depends on it.

libexo.$(OEXT): ../host.h ../misc.h ../machine.h ../host.h ../misc.h
libexo.$(OEXT): ../machine.def libexo.h
exolex.$(OEXT): ../host.h ../misc.h libexo.h ../machine.h ../host.h ../misc.h
exolex.$(OEXT): ../machine.def
