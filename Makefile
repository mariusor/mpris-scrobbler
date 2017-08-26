BIN_NAME := mpris-scrobbler
CC ?= cc
LIBS = libevent libcurl expat dbus-1
COMPILE_FLAGS = -std=c11 -Wpedantic -D_GNU_SOURCE -Wall -Wextra -Wimplicit-fallthrough=0
LINK_FLAGS =
RCOMPILE_FLAGS = -DNDEBUG -O2 -fno-omit-frame-pointer
DCOMPILE_FLAGS = -DDEBUG -Og -g -fno-stack-protector
RLINK_FLAGS =
DLINK_FLAGS =
M4 = /usr/bin/m4
M4_FLAGS =
RAGEL = /usr/bin/ragel
RAGELFLAGS = -G2 -C -n

SOURCES = src/main.c
UNIT_NAME=$(BIN_NAME).service

DESTDIR = /
INSTALL_PREFIX = usr/local/
BINDIR = bin
USERUNITDIR = lib/systemd/user
DBUSNAME=org.mpris.scrobbler

ifneq ($(LIBS),)
	CFLAGS += $(shell pkg-config --cflags $(LIBS))
	LDFLAGS += $(shell pkg-config --libs $(LIBS))
endif

ifeq ($(shell git describe --always > /dev/null 2>&1 ; echo $$?), 0)
	GIT_VERSION = $(shell git describe --tags --long --dirty=-git --always )
else
	GIT_VERSION = 'unknown'
endif

.PHONY: all
all: release

#.PHONY: check
#check: check_leak check_memory

.PHONY: check_leak
check_leak: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS) -fsanitize=address
check_leak: clean run

.PHONY: check_memory
check_memory: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS) -fsanitize=memory -fsanitize-memory-track-origins=2
check_memory: clean run

.PHONY: check_undefined
check_undefined: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS) -fsanitize=undefined
check_undefined: clean run

.PHONY: run
run: $(BIN_NAME)
	./$(BIN_NAME) -vvv

release: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(RCOMPILE_FLAGS)
release: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(RLINK_FLAGS)
debug: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
debug: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)

.PHONY: release
release: $(BIN_NAME)

.PHONY: debug
debug: $(BIN_NAME)

.PHONY: clean
clean:
	$(RM) $(UNIT_NAME)
	$(RM) $(BIN_NAME)
	$(RM) src/ini.c
	$(RM) src/version.h

.PHONY: install
install: $(BIN_NAME) $(UNIT_NAME)
	mkdir -p -m 0755 $(DESTDIR)$(INSTALL_PREFIX)$(BINDIR)
	install $(BIN_NAME) $(DESTDIR)$(INSTALL_PREFIX)$(BINDIR)
	mkdir -p -m 0755 $(DESTDIR)$(INSTALL_PREFIX)$(USERUNITDIR)
	cp $(UNIT_NAME) $(DESTDIR)$(INSTALL_PREFIX)$(USERUNITDIR)

.PHONY: uninstall
uninstall:
	$(RM) $(DESTDIR)$(INSTALL_PREFIX)$(BINDIR)/$(BIN_NAME)
	$(RM) $(DESTDIR)$(INSTALL_PREFIX)$(USERUNITDIR)/$(UNIT_NAME)

$(BIN_NAME): src/ini.c src/*.c src/*.h src/version.h
	$(CC) $(CFLAGS) -DDBUSNAME=$(DBUSNAME) $(SOURCES) $(LDFLAGS) -o$(BIN_NAME)

$(UNIT_NAME): units/systemd-user.service.in
	$(M4) -DDESTDIR=$(DESTDIR) -DINSTALL_PREFIX=$(INSTALL_PREFIX) -DBINDIR=$(BINDIR) -DDBUSNAME=$(DBUSNAME) -DBIN_NAME=$(BIN_NAME) $< >$@

src/version.h: src/version.h.in
	$(M4) -DGIT_VERSION=$(GIT_VERSION) $< >$@

src/ini.c: src/ini.rl
	$(RAGEL) $(RAGELFLAGS) src/ini.rl -o src/ini.c

