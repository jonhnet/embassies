Need to be running X, e.g., by starting a VNC server via:
	vnc4server -geometry 1700x1100 -depth 24 :PortNumber

where PortNumber is your special Zoog number.  Then connect with a standard VNC client.

To run the monitor without debugging:
	~/zoog/monitors/linux_dbg/monitor$ ./build/xax_port_monitor

To debug the monitor:
	DEBUG_ME=1 ./build/xax_port_monitor
	Grab the sudo gdb -p 28830 line and run it in another terminal
	In GDB:
		f 3
		set wait=0
		cont

To run paltest (with debugging):
	cd ~/zoog/toolchains/linux_elf/paltest
	rm debug_*
	gdb ../../../monitors/linux_dbg/pal/build/xax_port_pal
	run build/paltest.signed

To run elf_loader:
	In one window, run a copy of zftp_backend
		cd ~/zoog/toolchains/linux_elf/zftp_backend
		build/zftp_backend --origin-filesystem true --origin-reference true --listen-zftp tunid --listen-lookup tunid --index-dir zftp_index_origin
	Once zftp is running, run:
		cd ~/zoog/toolchains/linux_elf/elf_loader
		gdb ../../../monitors/linux_dbg/pal/build/xax_port_pal
		run
	For extra debugging info, run the following in gdb:
		(gdb) shell ../scripts/debuggershelper.py
		(gdb) source addsyms
