#include <stdio.h>	//NULL
#include "xax_stack_table.h"
#include "xax_util.h"
#include "cheesy_snprintf.h"

#include "LiteLib.h"
#include "pal_abi/pal_extensions.h"

uint32_t xste_hash(const void *datum)
{
	XaxStackTableEntry *xste = (XaxStackTableEntry *) datum;
	return xste->gs_thread_handle;
}

int xste_cmp(const void *a, const void *b)
{
	XaxStackTableEntry *xste_a = (XaxStackTableEntry *) a;
	XaxStackTableEntry *xste_b = (XaxStackTableEntry *) b;
	return xste_a->gs_thread_handle - xste_b->gs_thread_handle;
}

void xst_init(XaxStackTable *xst, MallocFactory *mf, ZoogDispatchTable_v1 *xdt)
{
	xst->mf = mf;
	xst->xdt = xdt;
	zmutex_init(&xst->mutex);
	hash_table_init(&xst->ht, xst->mf, xste_hash, xste_cmp);
}

void xst_insert(XaxStackTable *xst, XaxStackTableEntry *xte)
{
	zmutex_lock(xst->xdt, &xst->mutex);
	xax_assert(hash_table_lookup(&xst->ht, xte)==NULL);
	XaxStackTableEntry *xte_copy =
		(XaxStackTableEntry *) mf_malloc(xst->mf, sizeof(XaxStackTableEntry));
	*xte_copy = *xte;
	hash_table_insert(&xst->ht, xte_copy);

	{
		char msg[160], hexbuf[12];
		lite_strcpy(msg, "xst_insert(gs_thread_handle=0x");
		lite_strcat(msg, hexstr_08x(hexbuf, xte->gs_thread_handle));
		lite_strcat(msg, ", stack_base=0x");
		lite_strcat(msg, hexstr_08x(hexbuf, (uint32_t) xte->stack_base));
		lite_strcat(msg, ", tid=0x");
		lite_strcat(msg, hexstr_08x(hexbuf, xte->tid));
		lite_strcat(msg, ")\n");

		debug_logfile_append_f *dla = (debug_logfile_append_f *)
			(xst->xdt->zoog_lookup_extension)("debug_logfile_append");
		(dla)("stack_bases", msg);
	}

	zmutex_unlock(xst->xdt, &xst->mutex);
}

XaxStackTableEntry *xst_peek(XaxStackTable *xst, uint32_t gs_thread_handle_key)
{
	zmutex_lock(xst->xdt, &xst->mutex);
	XaxStackTableEntry key;
	key.gs_thread_handle = gs_thread_handle_key;
	XaxStackTableEntry *xte =
		(XaxStackTableEntry *) hash_table_lookup(&xst->ht, &key);
	zmutex_unlock(xst->xdt, &xst->mutex);
	return xte;
}

void xst_remove(XaxStackTable *xst, uint32_t gs_thread_handle_key, XaxStackTableEntry *out_xte)
{
	zmutex_lock(xst->xdt, &xst->mutex);
	XaxStackTableEntry key;
	key.gs_thread_handle = gs_thread_handle_key;
	XaxStackTableEntry *xte =
		(XaxStackTableEntry *) hash_table_lookup(&xst->ht, &key);
	xax_assert(xte!=NULL);
	*out_xte = *xte;
	hash_table_remove(&xst->ht, &key);
	zmutex_unlock(xst->xdt, &xst->mutex);
}
