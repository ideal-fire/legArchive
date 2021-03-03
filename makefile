src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS =
CFLAGS = -Wall
OUTNAME = a.out

PREFIX = /usr/local

$(OUTNAME): $(obj)
	$(CC) -o $(OUTNAME) $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(OUTNAME)

.PHONY: depend
depend:
	makedepend -Y $(src)

install: $(OUTNAME)
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 755 a.out $(DESTDIR)$(PREFIX)/bin/legarchive

# DO NOT DELETE

legArchive.o: legArchive.h
main.o: legArchive.h
