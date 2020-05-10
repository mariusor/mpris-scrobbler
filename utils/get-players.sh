#!/bin/sh
dbus-send --print-reply --type=method_call --dest=org.freedesktop.DBus / org.freedesktop.DBus.ListNames | grep mpris
