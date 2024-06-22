.POSIX:
.SUFFIXES:

CC = cc
VERSION = 1.0
TARGET = vip
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

# Flags
CFLAGS = -O3 -march=native -mtune=native -pipe -s -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600

SRC = vip.c

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $@ $(CFLAGS)

dist:
	mkdir -p $(TARGET)-$(VERSION)
	cp -R README.md $(TARGET) $(TARGET)-$(VERSION)
	tar -cf $(TARGET)-$(VERSION).tar $(TARGET)-$(VERSION)
	gzip $(TARGET)-$(VERSION).tar
	rm -rf $(TARGET)-$(VERSION)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)
	cp -p $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	chmod 755 $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	$(RM) $(TARGET)

all: $(TARGET)

.PHONY: all dist install uninstall clean
