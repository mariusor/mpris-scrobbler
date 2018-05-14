MPRIS-SCROBBLER-CREDENTIALS(5)

# NAME

mpris-scrobbler-credentials - The MPRIS scrobbler daemon credentials file

# SYNOPSIS

_$XDG\_DATA\_HOME/mpris-scrobbler/credentials_

# DESCRIPTION

This is the only configuration file of the mpris-scrobbler daemon and is used to store the authentication credentials for the services that are enabled. The credentials are stored in plain text, so it's important that the file is not world readable.

The file format supports multiple sections, each of them corresponding to a scrobbling service and which can contain multiple value assignments.

A section is introduced by a line containing the service name enclosed in square brackets, so
```
[librefm]
```
would introduce a service called *librefm*.

A value assignment is a single line that has the name of the value, an equals sign, and a setting for the value, so
```
enabled = true
```
would set the value named *enabled* in the current service section to *true*. Any spaces preceding and succeeding the name and the value are ignored.

Any line starting with *;* is ignored, as is any blank line.

# SERVICES

The recognized service labels are:

*listenbrainz*
	Used to store configuration settings related to the open *listenbrainz.org* platform.

*librefm*
	Used to store configuration settings related to the open *libre.fm* platform.

*lastfm*
	Used to store configuration settings related to the proprietary *last.fm* platform.

# OPTIONS

_enabled=_
	Boolean value used to show if current service is active or not. Value can be *0*|*1* or *enabled*|*disabled*

_username=_
	String value containing the user name of the authenticated session.

_token=_
	String value containing the application *token* that the current session is authenticated with. 

_session=_
	String value containing the application *session key* that the current user is authenticated with.

_url=_
	String value containing the _custom_ URL for the current service's end-point. The only services that support this option are *libre.fm* and *listenbrainz.org*.

# EXAMPLE

```
[librefm]
enabled = true
username = <username>
token = <oauth-token-value>
session = <oauth-session-value>

[listenbrainz]
enabled = true
token = <listenbrainz-auth-token>
```

# FILES

_$XDG\_DATA\_HOME/mpris-scrobbler/credentials_


# SEE ALSO

*mpris-scrobbler*(1) *mpris-scrobbler-signon*(1)
