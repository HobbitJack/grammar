CC	?= gcc
CFLAGS	?= -std=c99 -Wall -Wextra -pedantic -Os
FEATURES	?= -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=600 -D_XOPEN_SOURCE_EXTENDED
DESTDIR ?= /usr/local
MANDIR	?= $(DESTDIR)/share/man/man1

grammar:
	$(CC) $(CFLAGS) $(FEATURES) -o grammar -lharper_c grammar.c

install: grammar
	chmod a+x grammar
	cp grammar $(DESTDIR)/bin/grammar
	gzip -k grammar.1	 
	cp grammar.1.gz $(MANDIR)/grammar.1.gz

install-symlink: grammar
	chmod a+x grammar
	ln -s $(PWD)/grammar $(DESTDIR)/bin/grammar
	gzip -k grammar.1
	ln -s $(PWD)/grammar.1.gz $(MANDIR)/grammar.1.gz

clean:
	rm grammar
	rm grammar.1.gz

install-clean:
	install
	clean

uninstall:
	rm $(DESTDIR)/bin/grammar
	rm $(MANDIR)/grammar.1.gz
