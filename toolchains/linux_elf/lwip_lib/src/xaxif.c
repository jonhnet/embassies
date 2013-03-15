#include "xaxif.h"

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>

#include "lwip/debug.h"

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/ip.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"

#include "netif/etharp.h"

#include "pal_abi/pal_abi.h"
#include "zutex_sync.h"
#include "LegacyPF.h"
#include "cheesymalloc.h"
#include "xax_extensions.h"
#include "xax_network_defs.h"

#include "xaxif.h"

#ifndef XAXIF_DEBUG
#define XAXIF_DEBUG LWIP_DBG_OFF
#endif

struct xaxif {
	ZoogDispatchTable_v1 *zdt;
	LegacyPF *pf;
};

static void  xaxif_input(struct netif *netif);
static void xaxif_thread(void *data);

static void
xax_low_level_init(struct netif *netif)
{
	struct xaxif *xaxif;

	xaxif = (struct xaxif *)netif->state;

	 // ip4_addr2(&(netif->gw)),

	sys_thread_new("xaxif_thread", xaxif_thread, netif, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);

}


static err_t
xax_low_level_output(struct netif *netif, struct pbuf *p)
{
	struct xaxif *xaxif = (struct xaxif *)netif->state;

	ZoogNetBuffer *xnb = (xaxif->zdt->zoog_alloc_net_buffer)(1514);

	char *bufptr = ZNB_DATA(xnb);
	uint32_t length = 0;

	struct pbuf *q;
	for(q = p; q != NULL; q = q->next) {
		/* Send the data from the pbuf to the interface, one pbuf at a
			 time. The size of the data in each pbuf is kept in the ->len
			 variable. */
		/* send data from(q->payload, q->len); */
		memcpy(bufptr, q->payload, q->len);
		bufptr += q->len;
		length += q->len;
	}

	/* signal that packet should be sent(); */
	(xaxif->zdt->zoog_send_net_buffer)(xnb, true);
	return ERR_OK;
}

static err_t
xax_output(struct netif *netif, struct pbuf *p,
		  ip_addr_t *ipaddr)
{
	LWIP_UNUSED_ARG(ipaddr);

	return xax_low_level_output(netif, p);
}

static struct pbuf *pbuf_fill(char *buffer, int len)
{
	struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
	if(p == NULL) {
		return NULL;
	}

	// NB pbuf_alloc, when successful, will create a chain whose
	// total length is exactly 'len', so we needn't worry about
	// tracking the remaining len here.
	char *bufptr = buffer;
	struct pbuf *q;
	for(q = p; q != NULL; q = q->next) {
		memcpy(q->payload, bufptr, q->len);
		bufptr += q->len;
	}
	return p;
}

static struct pbuf *
xax_low_level_input(struct xaxif *xaxif)
{
	struct pbuf *p;

	ZAS *zas = xaxif->pf->methods->legacy_pf_get_zas(xaxif->pf);
	while (1)
	{
		int idx = zas_wait_any(xaxif->zdt, &zas, 1);
		if (idx!=0)
		{
			LWIP_DEBUGF(XAXIF_DEBUG, ("xaxif_input: zas returned %d.\n", idx));
			continue;
		}
		LegacyMessage *lm = xaxif->pf->methods->legacy_pf_pop(xaxif->pf);
		if (lm==NULL)
		{
			LWIP_DEBUGF(XAXIF_DEBUG, ("xaxif_input: mq ready but returned NULL.\n"));
			continue;
		}

		uint32_t length = xaxif->pf->methods->legacy_message_get_length(lm);

		p = pbuf_fill((char*) xaxif->pf->methods->legacy_message_get_data(lm), length);
		xaxif->pf->methods->legacy_message_release(lm);
		return p;
	}
}

static void
xaxif_thread(void *arg)
{
	struct netif *netif;
	struct xaxif *xaxif;

	netif = (struct netif *)arg;
	xaxif = (struct xaxif *)netif->state;

	while(1) {
		xaxif_input(netif);
	}
}
/*-----------------------------------------------------------------------------------*/
/*
 * xaxif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
/*-----------------------------------------------------------------------------------*/
static void
xaxif_input(struct netif *netif)
{
	struct xaxif *xaxif;
	struct eth_hdr *ethhdr;
	struct pbuf *p;


	xaxif = (struct xaxif *)netif->state;

	p = xax_low_level_input(xaxif);

	if(p == NULL) {
		LWIP_DEBUGF(XAXIF_DEBUG, ("xaxif_input: low_level_input returned NULL\n"));
		return;
	}
	ethhdr = (struct eth_hdr *)p->payload;

	if (netif->input(p, netif) != ERR_OK) {
		LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
		pbuf_free(p);
		p = NULL;
	}
}
/*-----------------------------------------------------------------------------------*/
/*
 * xaxif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
/*-----------------------------------------------------------------------------------*/
err_t
xaxif_init(struct netif *netif)
{
	struct xaxif *xaxif;

	xaxif = (struct xaxif *)mem_malloc(sizeof(struct xaxif));
	if (!xaxif) {
		return ERR_MEM;
	}
	
	xaxif->zdt = netif->state;
	xaxif->pf = xe_network_add_default_handler();

	netif->state = xaxif;
	netif->name[0] = 'x';
	netif->name[1] = 'x';
	netif->output = xax_output;
	netif->linkoutput = NULL; //xax_low_level_output;
	netif->mtu = 1500;
	/* hardware address length */
	netif->hwaddr_len = 6;

	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_IGMP;

	xax_low_level_init(netif);

	return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/

ZAS *lwip_get_read_zas(int sockfd)
{
	struct netconn *nconn = get_netconn_for_socket(sockfd);
	ZAS *zas = sys_arch_get_mbox_zas(nconn);
	return zas;
}
