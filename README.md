# mpris-scrobbler

Is a minimalistic user daemon which submits the currently playing song to libre.fm and compatible services.
To retrieve song information it uses the MPRIS DBus interface, so it works with any media player that exposes this interface.

[![MIT Licensed](https://img.shields.io/github/license/mariusor/mpris-scrobbler.svg)](https://raw.githubusercontent.com/mariusor/mpris-scrobbler/master/LICENSE)
[![Build status](https://builds.sr.ht/~mariusor/mpris-scrobbler.svg)](https://builds.sr.ht/~mariusor/mpris-scrobbler)
[![Coverity Scan status](https://img.shields.io/coverity/scan/14230.svg)](https://scan.coverity.com/projects/14230)
[![Latest build](https://img.shields.io/github/release/mariusor/mpris-scrobbler.svg)](https://github.com/mariusor/mpris-scrobbler/releases/latest)
[![AUR package](https://img.shields.io/aur/version/mpris-scrobbler.svg)](https://aur.archlinux.org/packages/mpris-scrobbler/)

In order to compile the application you must have a valid development environment containing pkg-config, a compiler - known to work are `clang>=5.0` or `gcc>=7.0` - and the build system `meson` plus `ninja`.

The compile time dependencies are: `libevent`, `dbus-1.0>=1.9`, `libcurl`, `json-c` and their development equivalent packages.

## Getting the source

You can clone the git repository or download the latest release from [here](https://github.com/mariusor/mpris-scrobbler/releases/latest).

    $ git clone git@github.com:mariusor/mpris-scrobbler.git
    $ cd mpris-scrobbler

## Installing

For **packagers** please see the note at the [bottom](#packaging).

### CentOS, RHEL

`mpris-scrobbler` is available for CentOS and RHEL 7 or later via the [EPEL repository](https://fedoraproject.org/wiki/EPEL "EPEL: Extra Packages for Enterprise Linux").
Run the following commands to install it:

```sh
$ sudo yum install epel-release
$ sudo yum install mpris-scrobbler
```

### Fedora

`mpris-scrobbler` is available since Fedora 28.
Run the following command to install it:

```sh
$ sudo dnf install mpris-scrobbler
```

### Mageia, openSUSE

`mpris-scrobbler` is available for Mageia, openSUSE, and other RPM distributions via the [COPR repository](https://copr.fedorainfracloud.org/coprs/jflory7/mpris-scrobbler/).
First, install the correct repository for your operating system from COPR.
Then, install the `mpris-scrobbler` package with your package manager of choice.

### Ubuntu

Install the dependencies:

#### Ubuntu 18.04

```sh
sudo apt install libevent-2.1-6 libevent-dev libdbus-1-dev dbus dbus-user-session \
libcurl4 libcurl4-openssl-dev libjson-c-dev libjson-c3 meson
```

#### Ubuntu 20.04

```sh
sudo apt install libevent-2.1-7 libevent-dev libdbus-1-dev dbus dbus-user-session \
libcurl4 libcurl4-openssl-dev libjson-c-dev libjson-c-dev meson m4 scdoc
```

D-bus will need to be restarted:

    $ systemctl --user restart dbus.service

### Compile from source

To compile the scrobbler manually, you need to already have installed the dependencies mentioned above.
By default the prefix for the installation is `/usr`.

    $ meson setup build/

    $ ninja -C build/

    $ sudo ninja -C build/ install

## Usage

The scrobbler is comprised of two binaries: the daemon and the signon helper.

The daemon is meant run as a user systemd service which listens for any signals coming from your MPRIS enabled media player. To have it start at login, please execute the following command:

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

    $ mpris-scrobbler-signon --url http://127.0.0.1:8080 token [listenbrainz|librefm]

For the moment we don't support multiple entries for the same API. Ex, have a local instance for the ListenBrainz API and use the official one at the same time.

## Troubleshooting

If `mpris-scrobbler` does not seem to be working after following all usage instructions, confirm that `~/.local/share/mpris-scrobbler/credentials` contains:

    [yourservice]             ;; where yourservice is lastfm, librefm or listenbrainz
	enabled = true            ;; set via $ mpris-scrobbler-signon enable <service>
	username = {USERNAME}     ;; set via $ mpris-scrobbler-signon session <service> - only available for lastfm/librefm
	token = {TOKEN}           ;; set via $ mpris-scrobbler-signon token <service>
	session = {SESSION}       ;; set via $ mpris-scrobbler-signon session <service> - only available for lastfm/librefm

### Enhanced output verbosity

If the credentials are correct for the service you're having trouble with it could be helpfull to increase the verbosity of the logs. This can be achieved with the verbosity flag:

    $ mpris-scrobbler -v      # enable    INFO messages
    $ mpris-scrobbler -vv     # enable   DEBUG messages
    $ mpris-scrobbler -vvv    # enable TRACING messages

Further verbosity can be achieved by compiling the scrobbler using the `debug` build type, and then running it using the `-vvvv` maximum verbosity output flag, which makes the `TRACING` logs be even more verbose, including the actual requests sent to the scrobbling services and potentially sensitive information like credentials or authorization tokens.

    $ meson setup -Dbuildtype=debug build/

    $ ninja -C build

    $ ./build/mpris-scrobbler -vvvv # enable TRACING2 messages

For the exceptional cases when you might require verbosity from the libraries that the scrobbler links against, the following options can be additionally passed at build time:

* `-Dlibcurl_debug=true` to enable **libcurl** debug messages
* `-Dlibevent_debug=true` to enable **libevent2** debug messages
* `-Dlibdbus_debug=true` does not enable libdbus verbose logging, only a couple of extra logs related to loading the MPRIS metadata from the DBus messages.

An example for compiling the scrobbler with maximum verbosity would look like this:

    $ meson setup --reconfigure -Dbuildtype=debug -Dlibcurldebug=true -Dlibeventdebug=true -Dlibdbusdebug=true ./build
    $ ninja -C ./build

## Packaging

If you are a packager for mpris-scrobbler, please create separate credentials for the last.fm API at the [following URL](https://www.last.fm/api/account/create) instead of using the default ones packaged with the upstream source.

To use them in your build they need to be passed to meson setup:

    $ meson setup --reconfigure -Dlastfm_api_key=2XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX -Dlastfm_api_secret=YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY build/

## Resources

For discussions related to the project without requiring a Github account please see our mailing list: [https://lists.sr.ht/~mariusor/mpris-tools](https://lists.sr.ht/~mariusor/mpris-tools).

The documentation in the README file will be soon moved to a dedicated [wiki](https://man.sr.ht/~mariusor/mpris-tools/mpris-scrobbler/)

Check out the following articles and resources about mpris-scrobbler:

* [2 new apps for music tweakers on Fedora Workstation - Fedora Magazine](https://fedoramagazine.org/2-new-apps-for-music-tweakers-on-fedora-workstation/ "2 new apps for music tweakers on Fedora Workstation")
