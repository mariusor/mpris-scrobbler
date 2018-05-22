DAEMONNAME := mpris-scrobbler
SIGNONNAME := mpris-scrobbler-signon
CC ?= cc
LIBS = libevent libcurl dbus-1 json-c
COMPILE_FLAGS = -std=c11 -Wpedantic -Wall -Wextra
LINK_FLAGS =
RCOMPILE_FLAGS = -O2 -fno-omit-frame-pointer
DCOMPILE_FLAGS = -Wstrict-prototypes -Werror -DDEBUG -Og -g -fno-stack-protector
RLINK_FLAGS =
DLINK_FLAGS =
M4 = /usr/bin/m4
M4_FLAGS =

DAEMONSOURCES = src/daemon.c
SIGNONSOURCES = src/signon.c
UNIT_NAME=$(DAEMONNAME).service

DESTDIR ?= /
PREFIX ?= usr/local/
BINDIR ?= bin/
USERUNITDIR ?= lib/systemd/user
BUSNAME=org.mpris.scrobbler

prefix ?= $(DESTDIR)$(PREFIX)
exec_prefix ?= $(prefix)
bindir ?= $(exec_prefix)$(BINDIR)
userunitdir = $(prefix)$(USERUNITDIR)

#$(warning 'THIS BUILD METHOD IS NOW OBSOLETE')
#$(warning 'Please read the documentation on how to use Meson')

#ifeq ($(findstring cc,$(CC)),cc)
#	COMPILE_FLAGS += -Wimplicit-fallthrough=0
#endif
ifneq ($(LIBS),)
	CFLAGS += $(shell pkg-config --cflags $(LIBS))
	LDFLAGS += $(shell pkg-config --libs $(LIBS))
endif

ifeq ($(shell git describe --always > /dev/null 2>&1 ; echo $$?), 0)
	GIT_VERSION = $(shell git describe --tags --long --dirty=-git --always )
else
	GIT_VERSION = '(unknown)'
endif


CFLAGS += -DVERSION_HASH=\"$(GIT_VERSION)\" \
		  -DLASTFM_API_KEY=\"$(LASTFM_API_KEY)\" -DLASTFM_API_SECRET=\"$(LASTFM_API_SECRET)\" \
		  -DLIBREFM_API_KEY=\"$(LIBREFM_API_KEY)\" -DLIBREFM_API_SECRET=\"$(LIBREFM_API_SECRET)\" \
		  -DLISTENBRAINZ_API_KEY=\"$(LISTENBRAINZ_API_KEY)\" -DLISTENBRAINZ_API_SECRET=\"$(LISTENBRAINZ_API_SECRET)\" \

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
.PHONY: debug
ifndef LASTFM_API_SECRET
release:
	$(error "Please pass last.fm API key and secret")
debug:
	$(error "Please pass last.fm API key and secret")
else
release: $(DAEMONNAME) $(SIGNONNAME)
debug: $(DAEMONNAME) $(SIGNONNAME)
endif

.PHONY: clean
clean:
	$(RM) $(UNIT_NAME)
	$(RM) $(SIGNONNAME)
	$(RM) $(DAEMONNAME)

.PHONY: install
install: $(DAEMONNAME) $(SIGNONNAME) $(UNIT_NAME)
	mkdir -p -m 0755 $(bindir)
	install $(DAEMONNAME) $(bindir)
	install $(SIGNONNAME) $(bindir)
	mkdir -p -m 0755 $(userunitdir)
	cp $(UNIT_NAME) $(userunitdir)

.PHONY: uninstall
uninstall:
	$(RM) $(bindir)/$(DAEMONNAME)
	$(RM) $(bindir)/$(SIGNONNAME)
	$(RM) $(userunitdir)/$(UNIT_NAME)

$(DAEMONNAME): $(DAEMONSOURCES) src/*.h
	$(CC) $(CFLAGS) -DBUSNAME=$(BUSNAME) -DAPPLICATION_NAME=\"$(DAEMONNAME)\" $(DAEMONSOURCES) $(LDFLAGS) -o$(DAEMONNAME)

$(SIGNONNAME): $(SIGNONSOURCES) src/*.h
	$(CC) $(CFLAGS) -DAPPLICATION_NAME=\"$(DAEMONNAME)\" $(SIGNONSOURCES) $(LDFLAGS) -o$(SIGNONNAME)

$(UNIT_NAME): units/systemd-user.service.in
	$(M4) -DBINPATH=$(DESTDIR)$(PREFIX)$(BINDIR) -DDAEMONNAME=$(DAEMONNAME) $< >$@

