DAEMONNAME := mpris-scrobbler
SIGNONNAME := mpris-scrobbler-signon
CC ?= cc
LIBS = libevent libcurl expat dbus-1 openssl
COMPILE_FLAGS = -std=c11 -Wpedantic -D_GNU_SOURCE -Wall -Wextra
LINK_FLAGS =
RCOMPILE_FLAGS = -O2 -fno-omit-frame-pointer
DCOMPILE_FLAGS = -Wstrict-prototypes -Werror -DDEBUG -Og -g -fno-stack-protector
RLINK_FLAGS =
DLINK_FLAGS =
M4 = /usr/bin/m4
M4_FLAGS =
RAGEL = /usr/bin/ragel
RAGELFLAGS = -G2 -C -n

DAEMONSOURCES = src/daemon.c
SIGNONSOURCES = src/signon.c
UNIT_NAME=$(DAEMONNAME).service

DESTDIR ?= /
INSTALL_PREFIX ?= /usr/
BINDIR ?= bin/
USERUNITDIR ?= /lib64/systemd/user
BUSNAME=org.mpris.scrobbler

ifeq ($(findstring cc,$(CC)),cc)
	COMPILE_FLAGS += -Wimplicit-fallthrough=0
endif
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
run: $(DAEMONNAME)
	./$(DAEMONNAME) -vvv

release: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(RCOMPILE_FLAGS)
release: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(RLINK_FLAGS)
debug: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
debug: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)

.PHONY: release
release: $(DAEMONNAME) $(SIGNONNAME)

.PHONY: debug
debug: $(DAEMONNAME) $(SIGNONNAME)

.PHONY: clean
clean:
	$(RM) $(UNIT_NAME)
	$(RM) $(SIGNONNAME)
	$(RM) $(DAEMONNAME)
	$(RM) src/ini.c
	$(RM) src/version.h

.PHONY: install
install: $(DAEMONNAME) $(UNIT_NAME)
	mkdir -p -m 0755 $(DESTDIR)$(INSTALL_PREFIX)$(BINDIR)
	install $(DAEMONNAME) $(DESTDIR)$(INSTALL_PREFIX)$(BINDIR)
	install $(SIGNONNAME) $(DESTDIR)$(INSTALL_PREFIX)$(BINDIR)
	mkdir -p -m 0755 $(DESTDIR)$(INSTALL_PREFIX)$(USERUNITDIR)
	cp $(UNIT_NAME) $(DESTDIR)$(INSTALL_PREFIX)$(USERUNITDIR)

.PHONY: uninstall
uninstall:
	$(RM) $(DESTDIR)$(INSTALL_PREFIX)$(BINDIR)/$(DAEMONNAME)
	$(RM) $(DESTDIR)$(INSTALL_PREFIX)$(BINDIR)/$(SIGNONNAME)
	$(RM) $(DESTDIR)$(INSTALL_PREFIX)$(USERUNITDIR)/$(UNIT_NAME)

$(DAEMONNAME): $(DAEMONSOURCES) src/ini.c src/version.h src/*.h
	$(CC) $(CFLAGS) -DBUSNAME=$(BUSNAME) -DAPPLICATION_NAME=\"$(DAEMONNAME)\" $(DAEMONSOURCES) $(LDFLAGS) -o$(DAEMONNAME)

$(SIGNONNAME): $(SIGNONSOURCES) src/ini.c src/version.h src/*.h
	$(CC) $(CFLAGS) -DAPPLICATION_NAME=\"$(DAEMONNAME)\" $(SIGNONSOURCES) $(LDFLAGS) -o$(SIGNONNAME)

$(UNIT_NAME): units/systemd-user.service.in
	$(M4) -DDESTDIR=$(DESTDIR) -DINSTALL_PREFIX=$(INSTALL_PREFIX) -DBINDIR=$(BINDIR) -DBUSNAME=$(DBUSNAME) -DBINNAME=$(DAEMONNAME) $< >$@

src/version.h: src/version.h.in
	$(M4) -DGIT_VERSION=$(GIT_VERSION) $< >$@

src/ini.c: src/ini.rl
	$(RAGEL) $(RAGELFLAGS) src/ini.rl -o src/ini.c

