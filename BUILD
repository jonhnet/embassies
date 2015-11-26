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

sudo apt-get install vim-gnome git curl make dpkg-dev xorg-dev libpcap-dev

git clone https://git01.codeplex.com/forks/howell/fixbuild embassies-fixbuild
cd embassies-fixbuild

# build the root certs
(cd common/crypto-patched && make)
(cd toolchains/linux_elf/crypto && make build/crypto_util && ./regenerate-keys-certs.sh)
(cd monitors && make)
(cd toolchains/linux_elf && make)

TODO: sha-openssl* need to be modified to be extracted from some
	downloaded distro.
TODO: reenable zarfile creation in zftp_create_zarfile (or move to a neighboring directory
	to get deps right).
