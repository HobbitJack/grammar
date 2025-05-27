CC	?= gcc
CFLAGS	?= -std=c99 -Wall -Wextra -pedantic -Os
FEATURES	?= -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=600 -D_XOPEN_SOURCE_EXTENDED
DESTDIR ?= /usr/local
MANDIR	?= $(DESTDIR)/share/man/man1

grammar:
	$(CC) $(CFLAGS) $(FEATURES) -o grammar -lharper grammar.c

debug:
	$(CC) $(CFLAGS) -DDEBUG -g $(FEATURES) -o grammar_debug -lharper grammar.c

docs:
	rm -f grammar.1.gz
	gzip -k grammar.1

install: grammar docs
	cp -f grammar $(DESTDIR)/bin/grammar
	cp -f grammar.1.gz $(MANDIR)/grammar.1.gz

install-symlink: grammar docs
	ln -fs $(PWD)/grammar $(DESTDIR)/bin/grammar
	ln -fs $(PWD)/grammar.1.gz $(MANDIR)/grammar.1.gz

install-debug: debug
	cp -f grammar_debug ~/.local/bin/grammar_debug

clean:
	rm -f grammar
	rm -f grammar_debug
	rm -f grammar.1.gz

install-clean:
	install
	clean

uninstall:
	rm -f $(DESTDIR)/bin/grammar
	rm -f $(MANDIR)/grammar.1.gz
