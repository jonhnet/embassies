#include <alloca.h>

#include "LiteLib.h"
#include "FilterVFS.h"
#include "PatchPogoFilter.h"
#include "VFSElfReader.h"

#include "xinterpose.h"


inline static int32_t bound(int32_t v, int32_t lo, int32_t hi)
{
	if (v<lo) { v = lo; }
	if (v>hi) { v = hi; }
	return v;
}

void PatchPogoHandle::copy_in(
	void *dst_buf, uint32_t dst_offset, uint32_t dst_len,
	void *src_buf, uint32_t src_offset, uint32_t src_len)
{
	// we lose a bit of length because we need to do all this
	// math with signed ints. dst_start can be negative, depending
	// on the offset relationship.

	// move from src coords through logical (file) coords to dst coords
	int32_t logical_start = src_offset;
	int32_t logical_end = src_offset+src_len;

	int32_t dst_start = logical_start - (int32_t) dst_offset;
	int32_t dst_end = logical_end - (int32_t) dst_offset;

	// bound copy according to dst_len
	int32_t visible_dst_start = bound(dst_start, 0, dst_len);
	int32_t visible_dst_end = bound(dst_end, 0, dst_len);
	int32_t visible_dst_len = visible_dst_end - visible_dst_start;

	// move back through logical to src coords to compute src start offset
	int32_t visible_logical_start = visible_dst_start + dst_offset;
	int32_t visible_src_start = visible_logical_start - src_offset;

	lite_assert(visible_dst_start + visible_dst_len <= (int32_t) dst_len);
	lite_assert(visible_src_start + visible_dst_len <= (int32_t) src_len);

	// NB that the content may fall entirely before dst_offset
	// or after dst_offset+len,
	// in which case we'll compute visible_target_len==0, and
	// not disturb the buffer.

	lite_memcpy(
		((uint8_t*) dst_buf)+visible_dst_start,
		((uint8_t*) src_buf)+visible_src_start,
		visible_dst_len);
}

PatchPogoHandle::PatchPogoHandle(XaxVFSHandleIfc *base_handle, Elf32_Shdr *pogo_shdr)
	: FilterVFSHandle(base_handle)
{
	uint8_t *patch_ptr = patch_bytes;
#define SPLAT(v)	{ lite_memcpy(patch_ptr, &v, sizeof(v)); patch_ptr+=sizeof(v); }
	SPLAT(x86_push_immediate);
	uint32_t jump_destination = (uint32_t) &xinterpose;
	SPLAT(jump_destination);
	SPLAT(x86_ret);
	lite_assert((patch_ptr - patch_bytes) == POGO_PATCH_SIZE);

	patch_offset = pogo_shdr->sh_offset;
}

PatchPogoHandle::~PatchPogoHandle()
{
}

void PatchPogoHandle::read(
	XfsErr *err, void *dst, size_t len, uint64_t offset)
{
	FilterVFSHandle::read(err, dst, len, offset);
	copy_in(dst, offset, len,
		patch_bytes, patch_offset, POGO_PATCH_SIZE);
}

//////////////////////////////////////////////////////////////////////////////

PatchPogoVFS::PatchPogoVFS(XaxVFSIfc *base_vfs, MallocFactory *mf)
	: FilterVFS(base_vfs)
{
	this->mf = mf;
}

XaxVFSHandleIfc *PatchPogoVFS::open(
	XfsErr *err, XfsPath *path, int oflag, XVOpenHooks *open_hooks)
	
{
	ElfObj *eo = NULL;
	XaxVFSHandleIfc *result = NULL;
	XaxVFSHandleIfc *base_handle = base_vfs->open(err, path, oflag);
	if (base_handle==NULL)
	{
		result = NULL;
		goto exit;
	}

	eo = (ElfObj*) alloca(sizeof(ElfObj));
	ElfReaderIfc *eri;
	bool is_elf;
	eri = vfs_elf_reader_init(base_handle, mf);
	is_elf = elfobj_scan(eo, eri);

	if (!is_elf)
	{
		eo = NULL;	// false => object already 'freed'.
		result = base_handle;
		goto exit;
	}

	Elf32_Shdr *pogo_shdr;
	pogo_shdr = elfobj_find_shdr_by_name(eo, "pogo", NULL);
	if (pogo_shdr==NULL)
	{
		result = base_handle;
		goto exit;
	}

	result = new PatchPogoHandle(base_handle, pogo_shdr);

exit:
	if (eo!=NULL)
	{
		elfobj_free(eo);
	}
	return result;
}

