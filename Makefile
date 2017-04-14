BIN_NAME := mpris-scrobbler
CC ?= clang
LIBS = libclastfm libxdg-basedir dbus-1
COMPILE_FLAGS = -std=c99 -Wall -Wextra
LINK_FLAGS =
RCOMPILE_FLAGS = -D NDEBUG
DCOMPILE_FLAGS = -g -D DEBUG -O1
RLINK_FLAGS =
DLINK_FLAGS =

SOURCES = src/main.c
DESTDIR = /
INSTALL_PREFIX = usr/local

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


.PHONY: all
all: release

#.PHONY: check
#check: check_leak check_thread

.PHONY: check_leak
check_leak: export CC := clang
check_leak: DCOMPILE_FLAGS += -fsanitize=address -fPIE
check_leak: DLINK_FLAGS += -pie
check_leak: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
check_leak: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)
check_leak: BIN_NAME := $(BIN_NAME)-test
check_leak: clean executable run

.PHONY: check_thread
check_thread: export CC := clang
check_thread: DCOMPILE_FLAGS += -fsanitize=thread -lpthread -fPIE
check_thread: DLINK_FLAGS += -pie
check_thread: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
check_thread: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)
check_thread: BIN_NAME := $(BIN_NAME)-test
check_thread: clean executable run

.PHONY: check_memory
check_memory: export CC := clang
check_memory: DCOMPILE_FLAGS += -fsanitize=memory -fPIE
check_memory: DLINK_FLAGS += -pie
check_memory: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
check_memory: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)
check_memory: BIN_NAME := $(BIN_NAME)-test
check_memory: clean executable run

.PHONY: check_undefined
check_undefined: export CC := clang
check_undefined: DCOMPILE_FLAGS += -fsanitize=undefined -fPIE
check_undefined: DLINK_FLAGS += -pie
check_undefined: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
check_undefined: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)
check_undefined: BIN_NAME := $(BIN_NAME)-test
check_undefined: clean executable run

.PHONY: run
run: executable
	./$(BIN_NAME)

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

.PHONY: install
install: executable
	install $(BIN_NAME) $(DESTDIR)$(INSTALL_PREFIX)/bin

.PHONY: uninstall
uninstall:
	$(RM) $(DESTDIR)$(INSTALL_PREFIX)/bin/$(BIN_NAME)

.PHONY: executable
executable:
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCES) $(LDFLAGS) -o$(BIN_NAME)
