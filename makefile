CC      ?= clang
STRIP   ?= strip
PREFIX  ?= /usr/local

ARCH_FLAGS ?= -march=x86-64-v3
CFLAGS   += -std=c23 $(ARCH_FLAGS) -Wall -Wextra \
            -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
LDFLAGS  += -Wl,--gc-sections -Wl,--as-needed

OUT = axbr
SRC = main.c blocks.c
HDR = config.h
BINDIR = $(DESTDIR)$(PREFIX)/bin

.PHONY: all clean install

all: $(OUT)

$(OUT): $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $@

install: all
	install -d $(BINDIR)
	install -m 755 $(OUT) $(BINDIR)

clean:
	rm -f $(OUT)
