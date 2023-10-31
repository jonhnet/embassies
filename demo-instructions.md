
## virt-manager configuration

Configure a virt-manager VM with this image:
(Embassies Squeeze Demo Image)[https://drive.google.com/file/d/0B8QZ7d-q8CrNT21SRTZZb2RKVUU/view?usp=drive_link&resourcekey=0-OyLaAKZgItugq3QoY8s9_Q]
Configure the storage device to be SATA (not virtio).
Configure the network device to be e1000e (not virtio).
Launch the image from virt-manager.

## Login info

user name is `embassies`.
password for root & user is `embassies`

From the VM UI:

    * Log into the graphical UI.

    * Open a terminal window from the Applications/Accessories menu.

    * Type `ip a` to learn the local IP address of the VM.

From the host:

Modify ~/.ssh/config to add the following block. Replace the IP address
with the address you learned above.
```
# Embassies vm
Host 192.168.122.153
  HostKeyAlgorithms ssh-rsa
  PubkeyAcceptedKeyTypes ssh-rsa 
```

Connect to the host with a tunnel back to your X session:
```
ssh -Y embassies@192.168.122.153
```

In the ssh window:
```
embassies/toolchains/linux_elf/scripts/demo-abiword
```

**TODO:** As of this update (2023-10-31), the demo crashes with Segmentation
faults. I'll continue investigating why.

# Troubleshooting

## Timed out looking for root device.

Symptom: virt-manager launches grub, but gives up trying to mount the root
device.

Cause: By default, a modern virt-manager will attach the image as a virt-io
paravirtualized disk; the kernel in the guest image doesn't have drivers for it.

Solution: In virt-manager, change from a virt-io disk to a SATA disk.

## network access

Symptom: `ip a` command shows loopback but no outbound network.

Cause: virt-manager provided a default para-virtualized virtio network device.

Solution: Replace `virtio` device with `e1000e` in network configuration.

## ssh into guest

Symptom: Can't ssh into guest.
```
jonh@host:~$ ssh embassies@192.168.122.153
Unable to negotiate with 192.168.122.153 port 22: no matching host key type found. Their offer: ssh-rsa,ssh-dss
```

Cause: guest contains an old sshd that uses now-deprecated algorithms.

Solution: Tell your client (perhaps temporarily) to accept the deprecated
algorithms:
```
cat >> ~/.ssh/config <<__EOF__
HostKeyAlgorithms ssh-rsa
PubkeyAcceptedKeyTypes ssh-rsa 
__EOF__
`
