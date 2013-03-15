#include "ZLCArgs.h"
#include "ZFTPApp.h"
#include "standard_malloc_factory.h"
#include "SyncFactory_Pthreads.h"
#include "SocketFactory_Posix.h"
#include "SocketFactory_Counter.h"
#include "ThreadFactory_Pthreads.h"
#include "SendBufferFactory_Memcpy.h"
#include "TunIDAllocator.h"

int main(int argc, char **argv)
{
	TunIDAllocator tunid;
	ZLCArgs zlc_args(argc, argv, tunid.get_tunid());
	ZFTPApp app;

  MallocFactory *mf = standard_malloc_factory_init();
	SocketFactory *socket_factory_poix = new SocketFactory_Posix(mf);
	SocketFactory *socket_factory = new SocketFactory_Counter(socket_factory_poix);
	SyncFactory *sf = new SyncFactory_Pthreads();
	ThreadFactory *tf = new ThreadFactory_Pthreads();
	SendBufferFactory *sbf = new SendBufferFactory_Memcpy(mf);

	app.run(&zlc_args, mf, socket_factory, sf, tf, sbf);
}


