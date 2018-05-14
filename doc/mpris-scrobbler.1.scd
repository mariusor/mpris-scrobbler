MPRIS-SCROBBLER(1)

# NAME

*mpris-scrobbler* - The MPRIS scrobbler user daemon

# SYNOPSIS

mpris-scrobbler [OPTIONS...]

# DESCRIPTION

*mpris-scrobbler* is a minimalistic user daemon for submitting the current playing songs to  
any of the supported scrobbling services.

It can interact with any media-player that conforms to the *MPRIS D-Bus Interface Specification*[1].

# SERVICES

*mpris-scrobbler* supported services are:

	*listenbrainz.org*
		Music service associated with the Music Brainz audio fingerprinting service.

	*libre.fm*
		Open source variant of the original audioscrobbler.com service.

	*last.fm*
		The evolution of the closed audioscrobbler.com service under the CBS umbrella.

# OPTIONS

*-h*, *--help*
	Print a short help message and version number and exit.

*-q*, *--quiet*
	Do not print any debugging messages.

*-v*, *-vv*, *-vvv*, *--verbose=[1..3]*
	Increase print verbosity to level:
		*1* Info messages.

		*2* Debug messages.

		*3* Tracing messages.

# SIGNALS

*SIGTERM*
	Upon receiving this signal the *mpris-scrobbler* daemon will try to cleanly exit.

*SIGINT*
	The *mpris-scrobbler* daemon treats this signal the same as *SIGTERM*.

*SIGHUP*
	Reloads the credentials file.

# ENVIRONMENT

_$XDG\_CONFIG\_HOME_, _$XDG\_DATA\_HOME_, _$XDG\_CACHE\_HOME_, _$XDG\_RUNTIME\_DIR_
	The *mpris-scrobbler* daemon uses these variables in accordance to the  
	*XDG Base Directory specification*[2] to find its configuration and save its PID file.

# NOTES

1.  *MPRIS D-Bus Interface Specification*
	https://specifications.freedesktop.org/mpris-spec/latest/

2.  *XDG Base Directory specification*
	http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html

# SEE ALSO

*mpris-scrobbler-signon*(1), *mpris-scrobbler-credentials*(5)
