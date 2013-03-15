/*
	basic setup invocation, patterned after unixlib.c:
	call xaxif_init, giving netif our interface parameters.
*/
#include "lwip/init.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/stats.h"
#include "lwip/sockets.h"

#include "pal_abi/pal_abi.h"
#include "xaxif.h"
#include "LiteLib.h"

struct xax_args {
	void *zdt;
	sys_sem_t sem;
};

struct netif netif;

static ip_addr_t *ip_addr_from_xip(XIPAddr *addr)
{
	lite_assert(addr->version == ipv4);
	return (ip_addr_t*) &addr->xip_addr_un.xv4_addr;
}

static void
tcpip_init_done(void *arg)
{
	struct xax_args *xargs = arg;
	ZoogDispatchTable_v1 *zdt = (ZoogDispatchTable_v1 *) xargs->zdt;

	XIPifconfig ifconfig;
	int count=1;
	(zdt->zoog_get_ifconfig)(&ifconfig, &count);
	lite_assert(count==1);

	netif_set_default(netif_add(
		&netif,
		ip_addr_from_xip(&ifconfig.local_interface),
		ip_addr_from_xip(&ifconfig.netmask),
		ip_addr_from_xip(&ifconfig.gateway),
		xargs->zdt,
		xaxif_init,
		tcpip_input));
	netif_set_up(&netif);

	sys_sem_signal(&xargs->sem);
}

void xaxif_setup(void *zdt){
	struct xax_args xargs;

	xargs.zdt = zdt;
	xax_arch_sem_configure(zdt);
	if(sys_sem_new(&xargs.sem, 0) != ERR_OK) {
		LWIP_ASSERT("failed to create semaphore", 0);
	}
	tcpip_init(tcpip_init_done, &xargs);
	sys_sem_wait(&xargs.sem);
	sys_sem_free(&xargs.sem);
}

void _fini(void){
}
