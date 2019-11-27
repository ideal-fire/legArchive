src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS =
CFLAGS = -Wall
OUTNAME = a.out

$(OUTNAME): $(obj)
	$(CC) -o $(OUTNAME) $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(OUTNAME)

.PHONY: depend
depend:
	makedepend -Y $(src)

# DO NOT DELETE

legArchive.o: legArchive.h
main.o: legArchive.h
