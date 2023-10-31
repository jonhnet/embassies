# Building the demo VM

This file documents how I constructed the demo VM.

## Use virt-manager to create a VM configured for Linux/Debian Squeeze.

### Install from Debian Squeeze:

Use virt-manager to create a VM configured for Linux/Debian Squeeze.

Attach iso
```
http://cdimage.debian.org/cdimage/archive/6.0.10/i386/iso-cd/debian-6.0.10-i386-netinst.iso
```
from
```
  https://www.debian.org/releases/squeeze/debian-installer/
```
to the VM, and start it up to install squeeze to the VM.

See [demo-instructons.md](demo-instructons.md) for tips on configuring
2023-era virt-manager to work with 2015-era squeeze, which doesn't know
about paravirtualized `virto` devices.

### Inside the VM:

### Put user account in sudoers.

Perhaps add `NOPASSWD: ALL` to `/etc/sudoers` to avoid sudo credentials
expiring (mentioned below).

### Install requisite libraries and tools

### Build the parts.
```
sudo apt-get install vim-gnome git curl make dpkg-dev xorg-dev libpcap-dev libpng12-dev libgtk2.0-dev abiword
```
### Clone embassies and build it.
```
git clone https://git01.codeplex.com/embassies
cd embassies
sudo true   # cach sudo credentials
make	 # this takes hours, due mostly to building eglibc
make -C toolchains/linux_elf/apps/xvnc
make -C toolchains/linux_elf/apps/abiword
toolchains/linux_elf/scripts/demo-abiword
```

This will take a very long time, as in hours, most of which goes to
building eglibc. Irritatingly, it'll stop every so often for sudo
credentials when it needs to fech a package. There's also a stop for
a verification prompt from a bash script. Be patient, but wait by
the terminal. :v)

### more app examples:
```
sudo apt-get install gimp inkscape gnumeric midori gnucash
(cd toolchains/linux_elf/apps/gimp && make)
(cd toolchains/linux_elf/apps/inkscape && make)
(cd toolchains/linux_elf/apps/gnumeric && make)
(cd toolchains/linux_elf/apps/midory && make)
(cd toolchains/linux_elf/apps/gnucash && make)
```

# run an app

Invoke this script, which will fire up a gnome-terminal with
tabs for the zftp backend server, the Embassies monitor,
a picoprocess with a zftp cache in it, and
a picoprocess with Abiword running in it.
It should pop up a window with abiword running.

```
toolchains/linux_elf/scripts/demo-abiword
```

**NOTE:** If, when trying the above, the monitor task crashes with:
```
xax_port_monitor: ../../../monitors/linux_common/xblit/XblitCanvas.cpp:198: void XblitCanvas::define_zoog_canvas(ZCanvas*): Assertion `bytes_per_pixel == zoog_bytes_per_pixel(zoog_pixel_format_truecolor24)' failed.
```
...then try running the command above from an 'ssh -Y' shell, so that
the DISPLAY variable points back at your host's X server.

Explanation follows:
The problem is that the Embassies monitor is really dumb, and
only knows how to paint a single kind of X display. You need a display
for which `depth 24` has `bits_per_pixel 32`:
```
xdpyinfo | grep 'depth 24'
  depth 24, bits_per_pixel 32, scanline_pad 32
```
That's pretty common on native displays, but apparently the virtual display
from virt-manager has
```
  depth 24, bits_per_pixel 24, scanline_pad 32
```
instead. To fix this, one might either improve the X display code in the monitor
(it's shared between the `linux_dbg` and `linux_kvm` monitors), or point
your monitor's `DISPLAY `variable at your host's native X display, such
as by ssh'ing into the VM with -Y.

**TODO:** sha-openssl* need to be modified to be extracted from some downloaded
distro.

**TODO:** reenable zarfile creation in `zftp_create_zarfile` (or move to a neighboring directory to get deps right). Without it the demo won't exploit the
benefits from the Missive paper.
