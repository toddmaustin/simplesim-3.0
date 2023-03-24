#
# Makefile arguments:
#
#	DIFF	  = diff program
#	SIM_DIR   = {simulator directory}
#	SIM_BIN   = {simulator binary}
#	SIM_OPTS  = {simulator binary}
#       GCC_BIN   = {SimpleScalar-enabled GNU GCC binary}
#       CFLAGS = {GNU GCC compiler flags}
#
CFLAGS	= -O2 -g

all: tests-live tests-eio

local-make-bins:
	$(MAKE) make-bins "SIM_DIR=.." \
	"GCC_BIN=/fs3/j/kskadro/SS/release/bin/ssbig-na-sstrix-gcc"

make-bins:
	cd bin; $(MAKE) "CC=$(GCC_BIN)" "CFLAGS=$(CFLAGS)"

do-tests: # make-bins
	-cd results $(CS) $(RM) dummy.out $(CS) cd ..
	$(SIM_DIR)$(X)$(SIM_BIN) -redir:prog results/anagram.progout \
		-redir:sim results/anagram.simout $(SIM_OPTS) \
		bin/anagram inputs/words < inputs/input.txt \
		> results$(X)dummy.out
	$(SIM_DIR)$(X)$(SIM_BIN) -redir:prog results/test-math.progout \
		-redir:sim results/test-math.simout $(SIM_OPTS) \
		bin/test-math
	$(SIM_DIR)$(X)$(SIM_BIN) -redir:prog results/test-printf.progout \
		-redir:sim results/test-printf.simout $(SIM_OPTS) \
		bin/test-printf
	$(SIM_DIR)$(X)$(SIM_BIN) -redir:prog results/test-fmath.progout \
		-redir:sim results/test-fmath.simout $(SIM_OPTS) \
		bin/test-fmath
	$(SIM_DIR)$(X)$(SIM_BIN) -redir:prog results/test-llong.progout \
		-redir:sim results/test-llong.simout $(SIM_OPTS) \
		bin/test-llong
	$(SIM_DIR)$(X)$(SIM_BIN) -redir:prog results/test-lswlr.progout \
		-redir:sim results/test-lswlr.simout $(SIM_OPTS)\
		bin/test-lswlr

diff-tests:
	@echo "#"
	@echo "# diff'ing output, NOTE: no differences should be detected..."
	@echo "#"
	-$(DIFF) outputs$(X)anagram.progout results$(X)anagram.progout
	-$(DIFF) outputs$(X)test-math.progout results$(X)test-math.progout
	-$(DIFF) outputs$(X)test-printf.progout results$(X)test-printf.progout
	-$(DIFF) outputs$(X)test-fmath.progout results$(X)test-fmath.progout
	-$(DIFF) outputs$(X)test-llong.progout results$(X)test-llong.progout
	-$(DIFF) outputs$(X)test-lswlr.progout results$(X)test-lswlr.progout

diff-errs:
	@echo "#"
	@echo "# diff'ing simout, NOTE: some differences will be detected..."
	@echo "#"
	-$(DIFF) outputs$(X)anagram.simout results$(X)anagram.simout
	-$(DIFF) outputs$(X)test-math.simout results$(X)test-math.simout
	-$(DIFF) outputs$(X)test-printf.simout results$(X)test-printf.simout
	-$(DIFF) outputs$(X)test-fmath.simout results$(X)test-fmath.simout
	-$(DIFF) outputs$(X)test-llong.simout results$(X)test-llong.simout
	-$(DIFF) outputs$(X)test-lswlr.simout results$(X)test-lswlr.simout

tests-eio:
	@echo "#"
	@echo "# executing w/EIO traces, NOTE: no errors should be detected..."
	@echo "#"
	$(SIM_DIR)$(X)$(SIM_BIN) -redir:prog results/anagram.eio-progout \
		-redir:sim results/anagram.eio-simout $(SIM_OPTS) \
		eio/anagram.eio
	$(SIM_DIR)$(X)$(SIM_BIN) -redir:prog results/test-math.eio-progout \
		-redir:sim results/test-math.eio-simout $(SIM_OPTS) \
		eio/test-math.eio
	$(SIM_DIR)$(X)$(SIM_BIN) -redir:prog results/test-printf.eio-progout \
		-redir:sim results/test-printf.eio-simout $(SIM_OPTS) \
		eio/test-printf.eio
	$(SIM_DIR)$(X)$(SIM_BIN) -redir:prog results/test-fmath.eio-progout \
		-redir:sim results/test-fmath.eio-simout $(SIM_OPTS) \
		eio/test-fmath.eio
	$(SIM_DIR)$(X)$(SIM_BIN) -redir:prog results/test-llong.eio-progout \
		-redir:sim results/test-llong.eio-simout $(SIM_OPTS) \
		eio/test-llong.eio
	$(SIM_DIR)$(X)$(SIM_BIN) -redir:prog results/test-lswlr.eio-progout \
		-redir:sim results/test-lswlr.eio-simout $(SIM_OPTS) \
		eio/test-lswlr.eio

local-tests:
	$(MAKE) tests-live "SIM_DIR=.." "SIM_BIN=sim-safe"

tests:
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" \
		"SIM_DIR=$(SIM_DIR)" "SIM_BIN=$(SIM_BIN)" \
		"SIM_OPTS=$(SIM_OPTS)" "DIFF=$(DIFF)" "X=$(X)" "CS=$(CS)" \
		do-tests
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" \
		"SIM_DIR=$(SIM_DIR)" "SIM_BIN=$(SIM_BIN)" \
		"SIM_OPTS=$(SIM_OPTS)" "DIFF=$(DIFF)" "X=$(X)" "CS=$(CS)" \
		diff-tests
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" \
		"SIM_DIR=$(SIM_DIR)" "SIM_BIN=$(SIM_BIN)" \
		"SIM_OPTS=$(SIM_OPTS)" "DIFF=$(DIFF)" "X=$(X)" "CS=$(CS)" \
		tests-eio

clean:
	-cd results $(CS) $(RM) * core $(CS) cd ..
	-$(RM) *.o *.i *.a *.obj *.exe core *~

veryclean:
	$(MAKE) clean
	cd bin.big; $(MAKE) clean
	cd bin.little; $(MAKE) clean
