#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
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

#include <sys/mman.h>

#include <sys/ipc.h>
#include <sys/shm.h>
//#include <linux/shm.h>

#include "xpbpool.h"

enum SharingMode {USE_MMAP, USE_SHM};
#define PAGE_SIZE (1<<12)
#define DEBUG_XPBPOOLELT_BUG 0

SharingMode mode = USE_SHM;

XPBPoolElt::XPBPoolElt(XaxPortBuffer *xpb, uint32_t mapping_size, BufferReplyType map_type, MapSpec *map_spec)
{
	this->mapped_in_client = false;
	this->owned_by_client = false;
	this->frozen_for_send = false;
	this->map_type = map_type;
	this->map_spec = *map_spec;
	this->xpb = xpb;
	this->mapping_size = mapping_size;
	this->identity = xpb->buffer_handle;
	sync_dbg();
}

XPBPoolElt::XPBPoolElt(uint32_t identity)
{
	this->identity = identity;
	this->xpb = NULL;	// key
}

XPBPoolElt::~XPBPoolElt()
{
	if (xpb!=NULL)
	{
		if (mode==USE_MMAP)
		{
			assert(false);	// unimpl
		}
		else
		{
			ShmMapping shmm((void*) xpb);
			shmm.unmap();
		}
	}
}

bool XPBPoolElt::busy()
{
	return owned_by_client || frozen_for_send;
}

void XPBPoolElt::freeze()
{
	assert(!frozen_for_send);	// need refcounts, I guess
	frozen_for_send = true;
}

void XPBPoolElt::unfreeze()
{
	assert(frozen_for_send);
	frozen_for_send = false;
}

void XPBPoolElt::sync_dbg()
{
	xpb->dbg_mapped_in_client = mapped_in_client;
	xpb->dbg_owned_by_client = owned_by_client;
	xpb->dbg_mapping_size = mapping_size;
}

uint32_t XPBPoolElt::hash(const void *v_a)
{
	XPBPoolElt *a = (XPBPoolElt *) v_a;
	return a->identity;
}

int XPBPoolElt::cmp(const void *v_a, const void *v_b)
{
	XPBPoolElt *a = (XPBPoolElt *) v_a;
	XPBPoolElt *b = (XPBPoolElt *) v_b;
	return a->identity - b->identity;
}
	
//////////////////////////////////////////////////////////////////////////////

XPBPool::XPBPool(MallocFactory *mf, int tunid)
	: shm(tunid)
{
	hash_table_init(&this->ht, mf, XPBPoolElt::hash, XPBPoolElt::cmp);
	this->name_counter = 'H'<<24;
	pthread_mutex_init(&this->mutex, NULL);
}

void XPBPool::_free_slot(void *v_this, void *v_obj)
{
#if DEBUG_XPBPOOLELT_BUG
	fprintf(stderr, "XPBPoolElt free %08x @_free_slot\n", (uint32_t) v_obj);
#endif // DEBUG_XPBPOOLELT_BUG
	delete (XPBPoolElt *) v_obj;
}

XPBPool::~XPBPool()
{
	hash_table_remove_every(&ht, _free_slot, this);
	int rc = pthread_mutex_destroy(&mutex);
	assert(rc==0);
	hash_table_free(&ht);
}

// TODO just store a tree, sortedy by capacity,
// alongside the hashtable, already.
class FreeScanner {
private:
	uint32_t payload_capacity;
	XPBPoolElt *result;

	void _visit(XPBPoolElt *elt) {
		if (!elt->busy()
			&& elt->xpb->capacity >= payload_capacity
			&& (result==NULL
				|| (elt->xpb->capacity > result->xpb->capacity)))
		{
			this->result = elt;
		}
	}

public:
	FreeScanner(uint32_t payload_capacity)
		: payload_capacity(payload_capacity), result(NULL) { }
	static void visit(void *v_this, void *v_obj)
	{ ((FreeScanner *) v_this)->_visit((XPBPoolElt *) v_obj); }
	XPBPoolElt *get_result() { return result; }
};

XPBPoolElt *XPBPool::_find_available(uint32_t payload_capacity)
{
	FreeScanner scanner(payload_capacity);
	hash_table_visit_every(&ht, FreeScanner::visit, &scanner);
	return scanner.get_result();
}

XPBPoolElt *XPBPool::_create_slot(uint32_t payload_capacity)
{
	name_counter++;
	uint32_t identity = name_counter;

	int map_size = sizeof(XaxPortBuffer) + payload_capacity;
	map_size = ((map_size-1) & (~PAGE_SIZE))+PAGE_SIZE;
	void *mapping;

	BufferReplyType map_type;
	MapSpec map_spec;

	if (mode==USE_MMAP)
	{
		char filename[256];
		snprintf(
			filename,
			sizeof(filename),
			"/tmp/xax.%08x.%08x.%x",
			getpid(),
			(unsigned int) this,
			identity);

		int rc;
		rc = unlink(filename);
		assert(rc==0 || errno==ENOENT);

	//	fprintf(stderr, "xpbpool creates %s\n", filename);
		int fd = open(filename, O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO);
		assert(fd>=0);

		// umask keeps group/other permissions from working; since
		// we run monitor as root (for tun), we need to fix them.
		// TODO or chown.
		// TODO or fix tun.
		rc = chmod(filename, S_IRWXU|S_IRWXG|S_IRWXO);
		assert(rc==0);

		rc = ftruncate(fd, map_size);
		assert(rc==0);

	//	fprintf(stderr, "xpbpool allocates frame, 0x%08x\n", map_size);
		mapping = mmap(
			(void*) 0,
			map_size,
			PROT_READ|PROT_WRITE,
			MAP_SHARED,
			fd,
			(off_t) 0);
		assert(mapping != MAP_FAILED && mapping!=NULL);

		close(fd);

		MapFileSpec *mspec = &map_spec.map_file_spec;
		assert(sizeof(mspec->path)==sizeof(filename));
		map_type = use_map_file_spec;
		memcpy(&mspec->path, filename, sizeof(filename));
		mspec->length = map_size;
		mspec->deprecates_old_handle = INVALID_HANDLE;
	}
	else if (mode==USE_SHM)
	{
		ShmMapping shmm = shm.allocate(identity, map_size);

		map_type = use_shmget_key;
		ShmgetKeySpec *sspec = &map_spec.shmget_key_spec;
		sspec->key = shmm.get_key();
		sspec->length = map_size;
		sspec->deprecates_old_handle = INVALID_HANDLE;
		fprintf(stderr, "shmget key is %x counter %x\n", shmm.get_key(), name_counter);
		mapping = shmm.map();
	}
	else
	{
		assert(false);	// invalid sharing mode
	}

	XaxPortBuffer *xpb = (XaxPortBuffer *) mapping;
	xpb->buffer_handle = identity;
	xpb->capacity = payload_capacity;

	XPBPoolElt *elt = new XPBPoolElt(xpb, map_size, map_type, &map_spec);
#if DEBUG_XPBPOOLELT_BUG
	fprintf(stderr, "XPBPoolElt new %08x @_create_slot\n", (uint32_t) elt);
#endif // DEBUG_XPBPOOLELT_BUG
	hash_table_insert(&ht, elt);
	return elt;
}

XaxPortBuffer *XPBPool::get_buffer(uint32_t payload_capacity, XpBufferReply *buffer_reply)
	// Updates buffer_reply to tell client how to access the buffer we're
	// returning here.
{
	XaxPortBuffer *result = NULL;

	pthread_mutex_lock(&mutex);

	XPBPoolElt *elt = _find_available(payload_capacity);
	if (elt==NULL)
	{
		elt = _create_slot(payload_capacity);
	}

	assert(elt->xpb->capacity >= payload_capacity);
	assert(!elt->busy());
	elt->owned_by_client = true;

	assert(elt->xpb->buffer_handle == elt->identity);
	// we're now using non-recyclable handle names, because we weren't
	// reclaiming the old names at the client, which REALLY confused him
	// when he eventually ended up seeing a recycled name.

	if (elt->mapped_in_client)
	{
		buffer_reply->type = use_open_buffer;
		buffer_reply->map_spec.open_buffer_handle = elt->xpb->buffer_handle;
	}
	else
	{
//		fprintf(stderr, "offering client new buffer %08x; mapped %d\n",
//			elt->xpb->buffer_handle, elt->mapped_in_client);
		buffer_reply->type = elt->map_type;
		buffer_reply->map_spec = elt->map_spec;
	}

/*
	fprintf(stderr,
		"xpbpool_get_buffer: Returning %s buffer size %d at hdl %08x '%s'\n",
		elt->mapped_in_client ? "OLD" : "NEW",
		elt->xpb->xnb_header.capacity,
		elt->xpb->buffer_handle,
		elt->map_file_spec.path);
*/

	//fprintf(stderr, "xpbpool_get_buffer: pool size %d\n", ht.count);
	result = elt->xpb;

	pthread_mutex_unlock(&mutex);
	return result;
}

ZoogNetBuffer *XPBPool::get_net_buffer(uint32_t payload_capacity, XpBufferReply *buffer_reply)
{
	XaxPortBuffer *xpb = get_buffer(payload_capacity, buffer_reply);
	if (xpb==NULL)
	{
		return NULL;
	}
	else
	{
		xpb->un.znb.znb_header.capacity = xpb->capacity;
		assert(payload_capacity <= xpb->un.znb.znb_header.capacity);
		return &xpb->un.znb.znb_header;
	}
}

XPBPoolElt *XPBPool::_lookup_elt_nolock(uint32_t handle)
{
	XPBPoolElt key(handle);
	XPBPoolElt *elt = (XPBPoolElt *) hash_table_lookup(&ht, &handle);
	if (elt != NULL)
	{
		assert(handle == elt->identity);
	}
	return elt;
}

XPBPoolElt *XPBPool::lookup_elt(uint32_t handle)
{
	pthread_mutex_lock(&mutex);
	XPBPoolElt *elt = _lookup_elt_nolock(handle);
	pthread_mutex_unlock(&mutex);
	return elt;
}

XaxPortBuffer *XPBPool::lookup_buffer(uint32_t handle)
{
	return lookup_elt(handle)->xpb;
}

void XPBPool::release_buffer(uint32_t handle, bool discard)
{
	pthread_mutex_lock(&mutex);
	if (discard)
	{
		XPBPoolElt key(handle);
		XPBPoolElt *elt = (XPBPoolElt *) hash_table_lookup(&ht, &handle);
		hash_table_remove(&ht, elt);
#if DEBUG_XPBPOOLELT_BUG
		fprintf(stderr, "XPBPoolElt free %08x @release_buffer\n", (uint32_t) elt);
#endif // DEBUG_XPBPOOLELT_BUG
		delete elt;
	}
	else
	{
		XPBPoolElt *elt = _lookup_elt_nolock(handle);
		elt->mapped_in_client = true;
			// Any buffer released by the client (via send or release)
			// must have been accepted via recv or alloc, and thus has
			// been mapped by the client. Handy for later!
		assert(elt->owned_by_client);
		elt->owned_by_client = false;
			// and now we have it back, so it's available to alloc/recv.
		elt->sync_dbg();
	//	fprintf(stderr, "xpbpool_release_buffer: releasing hdl %08x\n",
	//		(uint32_t) elt->xpb->buffer_handle);
	}
	pthread_mutex_unlock(&mutex);
}
