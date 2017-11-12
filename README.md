# mpris-scrobbler

Wants to be a minimalistic user daemon which submits the currently playing song to libre.fm and compatible services.
To retrieve song information it uses the MPRIS DBus interface, so it works with any media player that exposes this interface.

[![MIT Licensed](https://img.shields.io/github/license/mariusor/mpris-scrobbler.svg)](https://raw.githubusercontent.com/mariusor/mpris-scrobbler/master/LICENSE)
[![Build status](https://img.shields.io/travis/mariusor/mpris-scrobbler.svg)](https://travis-ci.org/mariusor/mpris-scrobbler)
[![Coverity Scan status](https://img.shields.io/coverity/scan/14230.svg)](https://scan.coverity.com/projects/14230)

Its dependencies are: libevent dbus-1.0 libcurl expat openssl

## Installing

To compile the scrobbler manually, you need to already have installed the dependencies mentioned above. Compilation requires just:

    $ make release

    $ sudo make install

By default the prefix for the installation is `usr/local`, but you can control it by modifying the `INSTALL_PREFIX` environment variable at install time. Check the [Makefile](Makefile) for some other useful ones.

## Usage

The scrobbler is comprised of two binaries: the daemon and the signon helper.

The daemon is meant run as a user systemd service which listens for any signals coming from your MPRIS enabled media player. To enable it, do the following:

    $ systemctl --user enable mpris-scrobbler.service

It can submit the tracks being played to the [last.fm](https://last.fm) and [libre.fm](https://libre.fm) services.

At first nothing will get submitted as you need to generate a valid API session for your account.

To authenticate you must use the signon binary using the following sequence:

    $ mpris-scrobbler-signon token <service>

    $ mpris-scrobbler-signon session <service>

The first step opens a browser window to ask the user - that's you - to approve access for the application.

After granting permission to the application from the browser, you execute the second command to complete the authentication process.

The daemon loads the new generated credentials automatically and doesn't need to do it manually.
