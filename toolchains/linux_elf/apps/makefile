ZOOG_ROOT=../../..

APP_DIRS=\
	xvnc \
	dbus \
	midori \
	x11-apps \
	gnumeric \
	abiword \
	inkscape \
	gimp \

PREREQUISITE_DIRS=$(APP_DIRS)

TARGETS=

include ../make.in

clean:
	@for dir in $(PREREQUISITE_DIRS); do \
		$(MAKE) -C $${dir} clean ;\
	done
