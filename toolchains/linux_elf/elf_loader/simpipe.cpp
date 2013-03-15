#include <errno.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <dirent.h>

#include "LiteLib.h"
#include "pal_abi/pal_abi.h"
#include "zutex_sync.h"
#include "xax_posix_emulation.h"	// xax_unimpl_assert
#include "xax_util.h"

#include "simpipe.h"

SimParcel::SimParcel(MallocFactory *mf, const void *buf, size_t count)
{
	this->count = count;
	this->buffer_to_free = (mf->c_malloc)(mf, count);
	this->data = this->buffer_to_free;
	this->mf = mf;
	lite_memcpy(this->data, buf, count);
}

SimParcel::~SimParcel()
{
	mf_free(mf, buffer_to_free);
}

bool SimPipe::_zas_check(ZoogDispatchTable_v1 *xdt, ZAS *zas, ZutexWaitSpec *out_spec)
{
	bool result;
	SimPipe *spipe = (SimPipe *) zas;
	zmutex_lock(xdt, &spipe->zmutex);
	if (spipe->parcels.count==0)
	{
		result = false;
		out_spec->zutex = &spipe->zutex;
		out_spec->match_val = spipe->zutex;
	}
	else
	{
		result = true;
	}
	zmutex_unlock(xdt, &spipe->zmutex);
	return result;
}

ZASMethods3 SimPipe::simpipe_zas_methods = { NULL, _zas_check };

SimPipe::SimPipe(MallocFactory *mf, ZoogDispatchTable_v1 *xdt)
{
	this->zas.method_table = &simpipe_zas_methods;
	this->zutex = 6;
	zmutex_init(&this->zmutex);
	this->readhdl = new SimHandle(mf, xdt, this);
	this->writehdl = new SimHandle(mf, xdt, this);
	linked_list_init(&parcels, mf);
	zar_init(&this->always_ready);
}

SimHandle::SimHandle(MallocFactory *mf, ZoogDispatchTable_v1 *xdt, SimPipe *simpipe)
{
	this->mf = mf;
	this->xdt = xdt;
	this->simpipe = simpipe;
}

SimHandle::~SimHandle()
{
	// TODO leaking SimPipes; should refcount in SimPipe
}

uint32_t SimHandle::read(
	XfsErr *err, void *dst, size_t len)
{
	xax_assert(simpipe->readhdl == this);

	size_t amount;
	while (true)
	{
		zmutex_lock(xdt, &simpipe->zmutex);
		if (simpipe->parcels.count==0)
		{
			zmutex_unlock(xdt, &simpipe->zmutex);
			ZAS *the_zas = &simpipe->zas;
			zas_wait_any(xdt, &the_zas, 1);
			continue;
		}

		SimParcel *parcel = (SimParcel*) linked_list_remove_head(&simpipe->parcels);
		amount = parcel->count;
		if (amount > len)
		{
			amount = len;
		}
		lite_memcpy(dst, parcel->data, amount);
		parcel->data = ((uint8_t*) parcel->data) + amount;
		parcel->count -= amount;
		if (parcel->count > 0)
		{
			// oh, there's some left: stuff it back into the list
			linked_list_insert_head(&simpipe->parcels, parcel);
		}
		else
		{
			delete parcel;
		}

		zmutex_unlock(xdt, &simpipe->zmutex);
		break;
	}
	*err = XFS_NO_ERROR;
	return amount;
}

void SimHandle::write(
	XfsErr *err, const void *src, size_t len, uint64_t offset)
{
	lite_assert(offset==0);
	xax_assert(simpipe->writehdl == this);

	SimParcel *parcel = new SimParcel(mf, src, len);
	zmutex_lock(xdt, &simpipe->zmutex);
	linked_list_insert_tail(&simpipe->parcels, parcel);

	// TODO sloppy signalling always calls kernel.
	simpipe->zutex += 1;
	(xdt->zoog_zutex_wake)
		(&simpipe->zutex, simpipe->zutex, ZUTEX_ALL, 0, NULL, 0);

	zmutex_unlock(xdt, &simpipe->zmutex);

	*err = XFS_NO_ERROR;
}

void SimHandle::xvfs_fstat64(
	XfsErr *err, struct stat64 *buf)
{
	*err = (XfsErr) EINVAL;
}

ZAS *SimHandle::get_zas(
	ZASChannel channel)
{
	if (channel == zas_read)
	{
		return &simpipe->zas;
	}
	else
	{
		return &simpipe->always_ready.zas;
	}
}

