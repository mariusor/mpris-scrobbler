# mpris-scrobbler

Wants to be a minimalistic user daemon which submits the currently playing song to libre.fm and compatible services.
To retrieve song information it uses the MPRIS DBus interface, so it works with any media player that exposes this interface.

[![MIT Licensed](https://img.shields.io/github/license/mariusor/mpris-scrobbler.svg)](https://raw.githubusercontent.com/mariusor/mpris-scrobbler/master/LICENSE)
[![Build status](https://img.shields.io/travis/mariusor/mpris-scrobbler.svg)](https://travis-ci.org/mariusor/mpris-scrobbler)
<!-- [![Coverity Scan status](https://img.shields.io/coverity/scan/14230.svg)](https://scan.coverity.com/projects/14230)
[![Latest build](https://img.shields.io/github/release/mariusor/mpris-scrobbler.svg)](https://github.com/mariusor/mpris-scrobbler/releases/latest) -->
[![AUR package](https://img.shields.io/aur/version/mpris-scrobbler.svg)](https://aur.archlinux.org/packages/mpris-scrobbler/)

In order to compile the application you must have a valid development environment containing pkg-config, a compiler - known to work are clang>=5.0 or gcc>=7.0 - and the build system meson plus ninja.

The compile time dependencies are: `libevent`, `dbus-1.0`, `libcurl`, `json-c` and their development equivalent packages.

For the moment it also requires the [ragel](http://www.colm.net/open-source/ragel/) state machine compiler to generate the basic ini parser used to read the configuration files.

## Getting the source

You can clone the git repository or download the latest release from [here](https://github.com/mariusor/mpris-scrobbler/releases/latest).

    $ git clone git@github.com:mariusor/mpris-scrobbler.git
    $ cd mpris-scrobbler

## Installing

To compile the scrobbler manually, you need to already have installed the dependencies mentioned above.
By default the prefix for the installation is `/usr`.

    $ meson build/

    $ ninja -C build/

    $ sudo ninja -C build/ install

## Usage

The scrobbler is comprised of two binaries: the daemon and the signon helper.

The daemon is meant run as a user systemd service which listens for any signals coming from your MPRIS enabled media player. To have it start when you login, please execute the following command:

    $ systemctl --user enable --now mpris-scrobbler.service

If the command above didn't start the service, you can do it manually:

    $ systemctl --user start mpris-scrobbler.service

It can submit the tracks being played to the [last.fm](https://last.fm) and [libre.fm](https://libre.fm) services, and to [listenbrainz.org](https://listenbrainz.org/).

At first nothing will get submitted as you need to enable one or more of the available services and also generate a valid API session for your account.

The valid services that mpris-scrobbler knows are: `librefm`, `lastfm` and `listenbrainz`.

### Enabling a service

Enabling a service is done automatically once you obtain a valid token/session for it. See the [authentication](#authenticate-to-the-service) section.

You can however disable submitting tracks to a service by invoking:

    $ mpris-scrobbler-signon disable <service>

If you want to re-enable a service which you previously disabled you can call, without the need to re-authenticate:

    $ mpris-scrobbler-signon enable <service>

### Authenticate to the service

##### ListenBrainz

Because ListenBrainz doesn't have yet support for OAuth authentication, the credentials must be added manually using the signon binary.
First you need to get the **user token** from  your [ListenBrainz profile page](https://listenbrainz.org/profile).
Then you call the following command and type or paste the token, then press Enter:

    $ mpris-scrobbler-signon token listenbrainz
    Token for listenbrainz.org:


##### Audioscrobbler compatible

Libre.fm and Last.fm are using the same API for authentication and currently this is the mechanism:

Use the signon binary in the following sequence:

    $ mpris-scrobbler-signon token <service>

    $ mpris-scrobbler-signon session <service>

The valid service labels are: `librefm` and `lastfm`.

The first step opens a browser window -- for this to work your system requires `xdg-open` binary -- which asks you to login to last.fm or libre.fm and then approve access for the `mpris-scrobbler` application.

After granting permission to the application from the browser, you execute the second command to create a valid API session and complete the process.

The daemon loads the new generated credentials automatically and you don't need to do it manually.

The authentication for the libre.fm and listenbrainz.org services supports custom URLs that can be passed to the signon binary using the `--url` argument.

