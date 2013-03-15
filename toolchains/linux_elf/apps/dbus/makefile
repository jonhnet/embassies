ZOOG_ROOT=../../../..

PACKAGE_NAME=dbus
PIE_NAME=dbus-daemon-pie
PRE_PATCH_SCRIPT=\
	patch build/source-files/dbus-1.2.24/debian/rules < verbose.patch

BUILT_PIE=build/source-files/dbus-1.2.24/bus/dbus-daemon

include ../make.in
