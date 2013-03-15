#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <linux/unistd.h>
#include <asm/ldt.h>
//#define _GNU_SOURCE
#include <sched.h>
#include <time.h>

#include <sys/ipc.h>	// IPC_RMID

#include "xpbtable.h"
#include "gsfree_syscall.h"
#include "cheesy_snprintf.h"

#define IPCOP_shmat			21
#define IPCOP_shmdt			22
#define IPCOP_shmget		23
#define IPCOP_shmctl		24

XPBMappingKey::XPBMappingKey(uint32_t handle)
{
	this->xpb = NULL;
	this->handle = handle;
}

uint32_t XPBMapping::hash(const void *v_a)
{
	XPBMapping *x_a = (XPBMapping *) v_a;
	return x_a->handle;
}

int XPBMapping::cmp(const void *v_a, const void *v_b)
{
	XPBMapping *x_a = (XPBMapping *) v_a;
	XPBMapping *x_b = (XPBMapping *) v_b;
	return x_a->handle - x_b->handle;
}

MMapMapping::MMapMapping(MapFileSpec *spec)
{
	this->length = spec->length;

	int fd = _gsfree_syscall(__NR_open, spec->path, O_RDWR);
	gsfree_assert(fd>=0);
	
	void *mapping = (void*) _gsfree_syscall6(__NR_mmap2,
		0, this->length, PROT_READ|PROT_WRITE,
		MAP_SHARED, fd, 0);
	gsfree_assert(mapping!=MAP_FAILED && mapping!=NULL);

	_gsfree_syscall(__NR_close, fd);

	_gsfree_syscall(__NR_unlink, spec->path);

	this->xpb = (XaxPortBuffer*) mapping;
	this->handle = xpb->buffer_handle;
}

MMapMapping::~MMapMapping()
{
	int rc = _gsfree_syscall(__NR_munmap,
		xpb, length);
	gsfree_assert(rc==0);
}

void *gsfree_shmat(int shmid, const void *shmaddr, int shmflg)
{
	uint32_t raddr;
	uint32_t result = _gsfree_syscall(
		__NR_ipc, IPCOP_shmat, shmid, shmflg, &raddr, shmaddr);
	if ((result | 0x3ff)==(uint32_t)-1)
	{
		gsfree_assert(false);
	}
	return (void*) raddr;
}

ShmMapping::ShmMapping(ShmgetKeySpec *spec)
{
	int shmid = _gsfree_syscall(
		__NR_ipc, IPCOP_shmget, spec->key, spec->length, 0600);
	gsfree_assert((shmid | 0x3ff) != -1);

	void *mapping = gsfree_shmat(shmid, NULL, 0);

	// now that we've got it, mark it for deletion
	if (g_shm_unlink)
	{
		int rc;
		rc = _gsfree_syscall(__NR_ipc, IPCOP_shmctl, shmid, IPC_RMID, NULL);
		gsfree_assert(rc==0);
	}

	this->xpb = (XaxPortBuffer*) mapping;
	this->handle = xpb->buffer_handle;
}

ShmMapping::~ShmMapping()
{
	int rc = _gsfree_syscall(__NR_ipc, IPCOP_shmdt, 0, 0, 0, xpb);
	gsfree_assert(rc==0);
}

XPBTable::XPBTable(MallocFactory *mf)
{
	hash_table_init(&ht, mf, XPBMapping::hash, XPBMapping::cmp);
	cheesy_lock_init(&cheesy_lock);
	high_water_mark = 0;
}

XPBMapping *XPBTable::_lookup_nolock(uint32_t handle)
{
	XPBMappingKey key(handle);
	XPBMapping *mapping = (XPBMapping *) hash_table_lookup(&ht, &key);
	return mapping;
}

XaxPortBuffer *XPBTable::lookup(uint32_t handle)
{
	cheesy_lock_acquire(&cheesy_lock);
	XPBMapping *mapping = _lookup_nolock(handle);
	cheesy_lock_release(&cheesy_lock);
	return mapping->xpb;
}

void XPBTable::_insert(XPBMapping *mapping)
{
	cheesy_lock_acquire(&cheesy_lock);
	gsfree_assert(hash_table_lookup(&ht, mapping)==NULL);
	hash_table_insert(&ht, mapping);
	if (ht.count > high_water_mark)
	{
		high_water_mark = ht.count;
		char buf[1000];
		cheesy_snprintf(buf, sizeof(buf), "XPBTable bloats to %d\n", high_water_mark);
		safewrite_str(buf);
	}
	cheesy_lock_release(&cheesy_lock);
}

XaxPortBuffer *XPBTable::insert(XPBMapping *mapping, uint32_t old_handle)
{
	if (old_handle!=INVALID_HANDLE)
	{
		_discard(old_handle);
	}
	_insert(mapping);
	return mapping->xpb;
}

void XPBTable::discard(XaxPortBuffer *xpb)
{
	_discard(xpb->buffer_handle);
}

void XPBTable::_discard(uint32_t handle)
{
	if (handle==INVALID_HANDLE)
	{
		return;
	}

	cheesy_lock_acquire(&cheesy_lock);
	XPBMapping *mapping = _lookup_nolock(handle);
	gsfree_assert(mapping!=NULL);
	hash_table_remove(&ht, mapping);
	delete mapping;
	cheesy_lock_release(&cheesy_lock);
}

bool g_shm_unlink = true;
