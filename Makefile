VERCMD  ?= git describe --tags 2> /dev/null
VERSION := $(shell $(VERCMD) || cat VERSION)

CPPFLAGS += -D_POSIX_C_SOURCE=200809L -DVERSION=\"$(VERSION)\"
CFLAGS   += -std=c99 -pedantic -Wall -Wextra -DJSMN_STRICT
LDFLAGS  ?=
LDLIBS    = $(LDFLAGS) -lXfixes -lX11 -lXi -lm

PREFIX    ?= /usr/local
BINPREFIX ?= $(PREFIX)/bin

SRC = xbarrier.c
OBJ := $(SRC:.c=.o)

.PHONY: all
all: xbarrier

.PHONY: clean
clean:
	rm -f $(OBJ) xbarrier

.PHONY: install
install:
	mkdir -p "$(DESTDIR)$(BINPREFIX)"
	cp -pf xbarrier "$(DESTDIR)$(BINPREFIX)"

$(OBJ): Makefile

xbarrier: $(OBJ)
