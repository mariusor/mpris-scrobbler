BIN_NAME := mpris-scrobbler
LIBS = libevent expat libcurl dbus-1
COMPILE_FLAGS = -std=c11 -Wpedantic -D_GNU_SOURCE -Wall -Wextra -fno-omit-frame-pointer
LINK_FLAGS =
RCOMPILE_FLAGS = -D NDEBUG -O2
DCOMPILE_FLAGS = -D DEBUG -Og -g
RLINK_FLAGS =
DLINK_FLAGS =
RAGEL = /usr/bin/ragel
RAGELFLAGS = -G2 -C -n

ifeq ($(origin CC),default)
	CC = clang
endif

SOURCES = src/main.c
DESTDIR = /
INSTALL_PREFIX = usr/local
BINDIR = /bin

UNIT_NAME=$(BIN_NAME).service
USERUNITDIR=/lib/systemd/user

ifneq ($(LIBS),)
	CFLAGS += $(shell pkg-config --cflags $(LIBS))
	LDFLAGS += $(shell pkg-config --libs $(LIBS))
endif

ifeq ($(shell git describe > /dev/null 2>&1 ; echo $$?), 0)
	VERSION := $(shell git describe --tags --long --dirty=-git --always )
endif
ifneq ($(VERSION), )
	override CFLAGS := $(CFLAGS) -D VERSION_HASH=\"$(VERSION)\"
endif

src/ini.c: src/ini.rl
	$(RAGEL) $(RAGELFLAGS) src/ini.rl -o src/ini.c

.PHONY: all
all: release

#.PHONY: check
#check: check_leak check_memory

.PHONY: check_leak
check_leak: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS) -fsanitize=address
check_leak: BIN_NAME := $(BIN_NAME)-test
check_leak: clean run

.PHONY: check_memory
check_memory: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS) -fsanitize=memory -fsanitize-memory-track-origins=2
check_memory: BIN_NAME := $(BIN_NAME)-test
check_memory: clean run

.PHONY: check_undefined
check_undefined: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS) -fsanitize=undefined
check_undefined: BIN_NAME := $(BIN_NAME)-test
check_undefined: clean run

.PHONY: run
run: executable
	./$(BIN_NAME) -vvv

release: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(RCOMPILE_FLAGS)
release: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(RLINK_FLAGS)
debug: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
debug: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)

.PHONY: release
release: executable

.PHONY: debug
debug: executable

.PHONY: clean
clean:
	$(RM) $(BIN_NAME)
	$(RM) src/ini.c

.PHONY: install
install: $(BIN_NAME)
	install $(BIN_NAME) $(DESTDIR)$(INSTALL_PREFIX)$(BINDIR)
	mkdir -p -m 0755 $(DESTDIR)$(INSTALL_PREFIX)$(USERUNITDIR)
	install units/$(UNIT_NAME) $(DESTDIR)$(INSTALL_PREFIX)$(USERUNITDIR)

.PHONY: uninstall
uninstall:
	$(RM) $(DESTDIR)$(INSTALL_PREFIX)$(BINDIR)/$(BIN_NAME)
	$(RM) $(DESTDIR)$(INSTALL_PREFIX)$(USERUNITDIR)/$(UNIT_NAME)

.PHONY: executable
executable: src/ini.c
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCES) $(LDFLAGS) -o$(BIN_NAME)
