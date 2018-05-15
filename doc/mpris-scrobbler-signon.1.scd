MPRIS-SCROBBLER-SIGNON(1)

# NAME

*mpris-scrobbler-signon* - The MPRIS scrobbler sign-on utility

# SYNOPSIS

mpris-scrobbler-signon [OPTIONS...] [COMMAND] [SERVICE]

# DESCRIPTION

*mpris-scrobbler-signon* is the utility command used to execute the authentication steps required  
by each audioscrobbler service.

The credentials are stored in the *mpris-scrobbler-credentials*(5) file.

# COMMANDS

*token*
	Get the authentication token for SERVICE.

	This step requires that the *xdg-open*(1) executable exists on the machine, so it can open a  
	browser window with the authorization page of the SERVICE.

*session*
	Activate a new session for SERVICE. It requires that a valid token was previously acquired for SERVICE.

*enable*
	Activate SERVICE for submitting tracks.

*disable*
	Deactivate SERVICE for submitting tracks.

# SERVICES

*mpris-scrobbler* supports the following service labels. For a full description check the *SERVICES* section of *mpris-scrobbler*(5):

1.  *listenbrainz*: listenbrainz.org[1]

2.  *librefm*: libre.fm[2]

3.  *lastfm*: last.fm[3]

# OPTIONS

*-h*, *--help*
	Print a short help message and exit.

*-q*, *--quiet*
	Do not print any debugging messages.

*-v*, *-vv*, *-vvv*, *--verbose=[1..3]*
	Increase print verbosity to level:

		*1* Info messages.

		*2* Debug messages.

		*3* Tracing messages.

	The default is to display only error and warning messages.

*-u*, *--url=*
	Use to a custom URL to connect to the SERVICE. 

	This option is valid only when using the *libre.fm* or *listenbrainz.org* services which provide the code to create a local instance of the service.

# NOTES

1.  *listenbrainz.org*
	https://listenbrainz.org 

2.  *libre.fm*
	https://libre.fm

3.  *last.fm*
	https://last.fm

# SEE ALSO

*mpris-scrobbler*(1), *mpris-scrobbler-credentials*(5)
