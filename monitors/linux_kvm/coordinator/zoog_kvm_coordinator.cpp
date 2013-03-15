/*
architecture sketch:

- each monitor process establishes a connection with the coordinator
	- coordinator has one thread per monitor
- connection from app (zoog_kvm_monitor) to coordinator (Listener)
-	is a named-pipe signalling channel
- messages may be inline or via an external file (for large messages)
- messages, going both ways, include:
	- deliver message
	- launch application
*/

#include <stdlib.h>
#include <unistd.h>
#include "LiteLib.h"
#include "Listener.h"
#include "standard_malloc_factory.h"
#include "malloc_factory_operator_new.h"
#include "ambient_malloc.h"

int main(int argc, char **argv)
{
	fprintf(stderr, "sudo gdb -p %d\n", getpid());

	MallocFactory *mf = standard_malloc_factory_init();
	malloc_factory_operator_new_init(mf);
	ambient_malloc_init(mf);

	Listener listener(mf);
	listener.run();
}
