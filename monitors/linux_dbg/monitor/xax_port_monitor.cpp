#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <asm/unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <pwd.h>

#include "linux_dbg_port_protocol.h"
#include "xax_network_defs.h"
#include "PacketQueue.h"
#include "netifc.h"
#include "xpbpool.h"
#include "xax_port_monitor.h"
#include "Monitor.h"
#include "standard_malloc_factory.h"
#include "ambient_malloc.h"

int main(int argc, char **argv)
{
	ambient_malloc_init(standard_malloc_factory_init());

	fprintf(stderr, "sudo gdb -p %d\n", getpid());
	if (getenv("DEBUG_ME")!=NULL)
	{
		fprintf(stderr, "waiting\n");
		bool wait=true;
		while (wait)
		{
			sleep(1);
		}
	}
	else
	{
		fprintf(stderr, "no DEBUG_ME\n");
	}

	Monitor monitor;

	// main thread does nothing; monitor makes its own worker threads.
	while(1) { sleep(10); }
}

#if 1
extern "C" {
void exit(int status)
{
	fprintf(stderr, "HEY! I'M DYIN' HERE! debugger attach point\n");
	fprintf(stderr, "sudo gdb -p %ld\n", syscall(__NR_gettid));
	while (1)
	{
		sleep(1);
	}
}
}
#endif
