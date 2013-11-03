CC=mpicc
BIN_CFLAGS=-g -O0 -Wall
CFLAGS=$(BIN_CFLAGS) -fPIC
LIBPATH=-L.
INCLUDE=
LIBS=-lgyan
EXE=tool.o
TESTDIR=tests

all:
	$(CC) $(CFLAGS) -c utility.c -o utility.o
	$(CC) $(CFLAGS) -c gyan.c -o gyan.o
	ar rcs libgyan.a gyan.o
	$(CC) -shared -o libgyan.so utility.o gyan.o
	$(CC) $(CFLAGS) $(INCLUDE) $(TESTDIR)/osu_bw.c $(LIBPATH) -o $(TESTDIR)/osu_bw $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDE) $(TESTDIR)/osu_bcast.c $(LIBPATH) -o $(TESTDIR)/osu_bcast $(LIBS)
clean:
	rm -f *.o
	rm -f $(TESTDIR)/osu_bw $(TESTDIR)/osu_bcast
	rm -f libgyan.so libgyan.a
