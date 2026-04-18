CC ?= clang
STRIP ?= strip

ARCH_FLAGS ?= -march=x86-64-v3
CFLAGS += -std=c23 -O3 $(ARCH_FLAGS) -flto -fno-plt \
          -Wall -Wextra -fno-stack-protector -fomit-frame-pointer \
          -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L

LDFLAGS += -flto -Wl,-O3 -Wl,--gc-sections -Wl,--as-needed

OUT = axbr
SRC = main.c blocks.c
HDR = config.h

PREFIX ?= /usr/local
BINDIR = $(DESTDIR)$(PREFIX)/bin

.PHONY: all clean install uninstall

all: $(OUT)

$(OUT): $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $@
	$(STRIP) -s $@

install: all
	install -d $(BINDIR)
	install -m 755 $(OUT) $(BINDIR)

uninstall:
	rm -f $(BINDIR)/$(OUT)

clean:
	rm -f $(OUT)
