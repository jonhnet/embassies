Use virt-manager to create a VM configured for Linux/Debian Squeeze.

Attach iso
  http://cdimage.debian.org/cdimage/archive/6.0.10/i386/iso-cd/debian-6.0.10-i386-netinst.iso
from
  https://www.debian.org/releases/squeeze/debian-installer/
to the VM, and start it up to install squeeze to the VM.

Inside the vm:

Put user account in sudoers.

sudo apt-get install vim-gnome git curl make dpkg-dev

git clone https://git01.codeplex.com/forks/howell/fixbuild embassies-fixbuild
cd embassies-fixbuild
make
