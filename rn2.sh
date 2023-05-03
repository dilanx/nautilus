qemu-system-x86_64 \
	-smp 2 \
	-m 2048 \
	-vga std \
	-serial stdio \
	-cdrom nautilus.iso \
	-device virtio-net,netdev=mynet0 \
	-netdev user,id=mynet0,net=192.168.76.0/24,dhcpstart=192.168.76.9 \
	-redir tcp:5555::7
