.POSIX:
.SUFFIXES:

VERSION = 1.0
TARGET = vip
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

CFLAGS = -O3 -march=native -mtune=native -pipe -s -flto -std=c99 -pedantic -Wall

SRC != find src -name *.c
INCLUDE = include

$(TARGET): $(SRC) config.h
	$(CC) $(SRC) config.h -o $@ $(CFLAGS) -I$(INCLUDE) -I.

dist:
	mkdir -p $(TARGET)-$(VERSION)
	cp -R README.md $(TARGET) $(TARGET)-$(VERSION)
	tar -cf $(TARGET)-$(VERSION).tar $(TARGET)-$(VERSION)
	gzip $(TARGET)-$(VERSION).tar
	rm -rf $(TARGET)-$(VERSION)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -p $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	chmod 755 $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm $(TARGET)

all: $(TARGET)

.PHONY: all dist install uninstall clean
