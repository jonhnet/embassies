#include <malloc.h>
#include "xax_network_utils.h"
#include "forwarding_table.h"
#include "format_ip.h"
#include "LiteLib.h"

void forwarding_table_init(ForwardingTable *ft, MallocFactory *mf)
{
	linked_list_init(&ft->list, mf);
}

void forwarding_table_add(ForwardingTable *ft, XIPAddr *dest, XIPAddr *mask, DeliverFunc *deliver_func, const char *deliver_dbg_name)
{
	ForwardingEntry *fe = (ForwardingEntry *) malloc(sizeof(ForwardingEntry));
	fe->dest = *dest;
	fe->mask = *mask;
	fe->deliver_func = deliver_func;
	fe->deliver_dbg_name = deliver_dbg_name;
	linked_list_insert_head(&ft->list, fe);
}

void forwarding_table_remove(ForwardingTable *ft, XIPAddr *dest, XIPAddr *mask)
{
	LinkedListIterator li;
	for (ll_start(&ft->list, &li); ll_has_more(&li); ll_advance(&li))
	{
		ForwardingEntry *fe = (ForwardingEntry *) ll_read(&li);
		if (cmp_xip(&fe->dest, dest)==0 && cmp_xip(&fe->mask, mask)==0)
		{
			ll_remove(&ft->list, &li);
			return;
		}
	}
	lite_assert(false);	// expected entry absent
}

bool _buf_mask(void *vb0, void *vb1, void *vmask, int size)
{
	lite_assert((size & 0x3) == 0);
	int words = size>>2;
	uint32_t *b0 = (uint32_t *) vb0;
	uint32_t *b1 = (uint32_t *) vb1;
	uint32_t *mask = (uint32_t *) vmask;
	int i;
	for (i=0; i<words; i++)
	{
		if ((b0[i] & mask[i]) != (b1[i] & mask[i]))
		{
			return false;
		}
	}
	return true;
}

bool mask_match(XIPAddr *a0, XIPAddr *a1, XIPAddr *mask)
{
	if (a0->version==ipv4)
	{
		if (a1->version!=ipv4)
		{
			return false;
		}
		lite_assert(a0->version==ipv4);
		lite_assert(a1->version==ipv4);
		lite_assert(mask->version==ipv4);
		return _buf_mask(
			&a0->xip_addr_un.xv4_addr,
			&a1->xip_addr_un.xv4_addr,
			&mask->xip_addr_un.xv4_addr,
			sizeof(a0->xip_addr_un.xv4_addr));
	}
	else
	{
		if (a1->version!=ipv6)
		{
			return false;
		}
		lite_assert(a0->version==ipv6);
		lite_assert(a1->version==ipv6);
		lite_assert(mask->version==ipv6);
		return _buf_mask(
			&a0->xip_addr_un.xv6_addr,
			&a1->xip_addr_un.xv6_addr,
			&mask->xip_addr_un.xv6_addr,
			sizeof(a0->xip_addr_un.xv6_addr));
	}
}

ForwardingEntry *forwarding_table_lookup(ForwardingTable *ft, XIPAddr *addr)
{
	LinkedListIterator li;
	for (ll_start(&ft->list, &li); ll_has_more(&li); ll_advance(&li))
	{
		ForwardingEntry *fe = (ForwardingEntry *) ll_read(&li);
		if (mask_match(addr, &fe->dest, &fe->mask))
		{
			return fe;
		}
	}
	lite_assert(false);	// expected entry absent
	return NULL;
}

void forwarding_table_print(ForwardingTable *ft)
{
	LinkedListIterator li;
	for (ll_start(&ft->list, &li); ll_has_more(&li); ll_advance(&li))
	{
		ForwardingEntry *fe = (ForwardingEntry *) ll_read(&li);
		char s_dest[100], s_mask[100];
		format_ip(s_dest, sizeof(s_dest), &fe->dest);
		format_netmask(s_mask, sizeof(s_mask), &fe->mask);
		fprintf(stderr, "%30s %30s %s\n",
			s_dest, s_mask, fe->deliver_dbg_name);
	}
}

