
all: anagram test-printf test-fmath test-math test-llong test-lswlr

anagram: ../src/anagram.c
	$(CC) $(CFLAGS) -o anagram ../src/anagram.c

test-printf: ../src/test-printf.c
	$(CC) $(CFLAGS) -o test-printf ../src/test-printf.c -lm

test-fmath: ../src/test-fmath.c
	$(CC) $(CFLAGS) -o test-fmath ../src/test-fmath.c -lm

test-math: ../src/test-math.c
	$(CC) $(CFLAGS) -o test-math ../src/test-math.c -lm

test-llong: ../src/test-llong.c
	$(CC) $(CFLAGS) -o test-llong ../src/test-llong.c

test-lswlr: ../src/test-lswlr.c
	$(CC) $(CFLAGS) -o test-lswlr ../src/test-lswlr.c

clean:
	rm -f anagram test-printf test-fmath test-math test-llong test-lswlr
	rm -f *.o core *~ Makefile.bak

