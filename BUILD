//////////////////////////////////////////////////////////////////////////////
// VM setup

Use virt-manager to create a VM configured for Linux/Debian Squeeze.

Attach iso
  http://cdimage.debian.org/cdimage/archive/6.0.10/i386/iso-cd/debian-6.0.10-i386-netinst.iso
from
  https://www.debian.org/releases/squeeze/debian-installer/
to the VM, and start it up to install squeeze to the VM.

//////////////////////////////////////////////////////////////////////////////
// Inside the vm:

Put user account in sudoers.

# Install requisite libraries and tools
sudo apt-get install vim-gnome git curl make dpkg-dev xorg-dev libpcap-dev libpng12-dev libgtk2.0-dev

git clone https://git01.codeplex.com/embassies
cd embassies-fixbuild

# cache sudo credentials.
sudo true

# build everything
# This will take a very long time, as in hours, most of which goes to
# building eglibc. Irritatingly, it'll stop every so often for sudo
# credentials when it needs to fech a package. There's also a stop for
# a verification prompt from a bash script. Be patient, but wait by
# the terminal. :v)
make

########################################
# build some apps

# build embassies-ified xvnc, which all the graphical apps depend upon
(cdtoolchains/linux_elf/apps/xvnc && make) 

# Intsall Linux versions of the apps we're going to build Embassies versions
# of, so that the shared libraries and data files for each are available
# to be served by zftp-backend to the Embassies client binary.
sudo apt-get install abiword
(cd toolchains/linux_elf/apps/abiword && make)

# more app examples:
sudo apt-get install gimp inkscape gnumeric midori gnucash
(cd toolchains/linux_elf/apps/gimp && make)
(cd toolchains/linux_elf/apps/inkscape && make)
(cd toolchains/linux_elf/apps/gnumeric && make)
(cd toolchains/linux_elf/apps/midory && make)
(cd toolchains/linux_elf/apps/gnucash && make)

########################################
# run an app

# In separate windows, run each line below. The first one should run in a
# terminal on the X console; it will try to make a shared memory connection
# to draw on the local X display. (It might also work remotely, but man,
# no idea.)

cd ~/embassies3/monitors/linux_dbg/monitor; ./build/xax_port_monitor
cd ~/embassies3/toolchains/linux_elf/zftp_backend; make run_backend_server
cd ~/embassies3/monitors/linux_dbg/pal; build/xax_port_pal ~/embassies3/toolchains/linux_elf/zftp_zoog/build/zftp_zoog.signed
cd ~/embassies3/monitors/linux_dbg/pal; build/xax_port_pal ~/embassies3/toolchains/linux_elf/elf_loader/build/elf_loader.wordprocessor_office_org.signed



TODO: sha-openssl* need to be modified to be extracted from some
	downloaded distro.
TODO: reenable zarfile creation in zftp_create_zarfile (or move to a neighboring directory
	to get deps right).

