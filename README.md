# mpris-scrobbler

Wants to be a minimalistic user daemon which submits the currently playing song to libre.fm and compatible services.
To retrieve song information it uses the MPRIS DBus interface, so it works with any media player that exposes this interface.

[![MIT Licensed](https://img.shields.io/github/license/mariusor/mpris-scrobbler.svg)](https://raw.githubusercontent.com/mariusor/mpris-scrobbler/master/LICENSE)
[![Build status](https://img.shields.io/travis/mariusor/mpris-scrobbler.svg)](https://travis-ci.org/mariusor/mpris-scrobbler)
[![Coverity Scan status](https://img.shields.io/coverity/scan/14230.svg)](https://scan.coverity.com/projects/14230)
[![Latest build](https://img.shields.io/github/release/mariusor/mpris-scrobbler.svg)](https://github.com/mariusor/mpris-scrobbler/releases/latest)
[![AUR package](https://img.shields.io/aur/version/mpris-scrobbler.svg)](https://aur.archlinux.org/packages/mpris-scrobbler/)

In order to compile the application you must have a valid development environment containing pkg-config, a compiler - known to work are clang>=5.0 or gcc>=7.0 - and the build system meson plus ninja.

The compile time dependencies are: `libevent`, `dbus-1.0`, `libcurl`, `jon-c`, `openssl` and their development equivalent packages.

For the moment it also requires the [ragel](http://www.colm.net/open-source/ragel/) state machine compiler to generate the basic ini parser used to read the configuration files.

## Getting the source

You can clone the git repository or download the latest release from [here](https://github.com/mariusor/mpris-scrobbler/releases/latest).

    $ git clone git@github.com:mariusor/mpris-scrobbler.git
    $ cd mpris-scrobbler

## Installing

To compile the scrobbler manually, you need to already have installed the dependencies mentioned above.
By default the prefix for the installation is `/usr`.

    $ mkdir build

    $ meson build

    $ ninja -C build

    $ sudo ninja -C build install

## Usage

The scrobbler is comprised of two binaries: the daemon and the signon helper.

The daemon is meant run as a user systemd service which listens for any signals coming from your MPRIS enabled media player. To have it start when you login, please execute the following command:

    $ systemctl --user enable --now mpris-scrobbler.service

If the command above didn't start the service, you can do it manually:

    $ systemctl --user start mpris-scrobbler.service

It can submit the tracks being played to the [last.fm](https://last.fm) and [libre.fm](https://libre.fm) services, and hopefully soon to [listenbrainz.org](https://listenbrainz.org/) - see [#5](https://github.com/mariusor/mpris-scrobbler/issues/5).

At first nothing will get submitted as you need to enable one or more of the available services and also generate a valid API session for your account.

To enable a service you must create/edit the `~/.config/mpris-scrobbler.ini` file by adding the following lines:

    [service]
    enabled = true

To authenticate you must use the signon binary using the following sequence:

    $ mpris-scrobbler-signon token <service>

    $ mpris-scrobbler-signon session <service>

The valid service labels are: `librefm` and `lastfm`.

The first step opens a browser window to ask the user - that's you - to approve access for the application.

After granting permission to the application from the browser, you execute the second command to complete the process.

The daemon loads the new generated credentials automatically and you don't need to do it manually.

