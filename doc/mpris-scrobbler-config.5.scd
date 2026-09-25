MPRIS-SCROBBLER-CONFIG(5)

# NAME

*mpris-scrobbler-config* - The MPRIS scrobbler daemon config file

# SYNOPSIS
_$XDG\_CONFIG\_HOME/mpris-scrobbler/config_

# DESCRIPTION

This is the configuration file of the *mpris-scrobbler* daemon and is used to store the list of players
that are to be ignored when loading MPRIS information.

The file format supports multiple value assignments in the form of:

```
ignore = PlayerName
ignore = org.mpris.MediaPlayer2.ServiceName
ignore = org.mpris.MediaPlayer2.ServiceName.instance_*
```

The player name and service name values are case sensitive and they support the '\*' wildcard ending, for
ignoring multiple players based on the same prefix.
