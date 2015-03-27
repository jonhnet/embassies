#include <alloca.h>
#include <errno.h>

extern "C" {
#include <binsearch.h>
}

#include "xax_network_utils.h"
#include "ZarfileVFS.h"
#include "ZLCVFS.h"	// for assemble_urls helper func
#include "math_util.h"
#include "ChunkDecoder.h"
#include "zemit.h"

//////////////////////////////////////////////////////////////////////////////

class ChdrHolder
{
private:
	ZarfileVFS* zvfs;
	uint32_t ci;
	ZF_Chdr* chdr;
public:
	ChdrHolder(ZarfileVFS* zvfs, uint32_t ci) : zvfs(zvfs), ci(ci)
	{
		chdr = zvfs->lock_chdr(ci);
	}
	~ChdrHolder()
	{
		zvfs->unlock_chdr(ci, chdr);
		chdr = NULL;
	}
	ZF_Chdr* get() { return chdr; }
};

//////////////////////////////////////////////////////////////////////////////

class ZarfileHandleChunkDecoder : public ChunkDecoder {
private:
	XaxVFSHandleIfc* zhdl;
	ZarfileVFS* zvfs;	// needed for accessing chdrs
public:
	ZarfileHandleChunkDecoder(ZF_Zhdr* zhdr, ZF_Phdr* phdr, XaxVFSHandleIfc* zhdl, ZarfileVFS* zvfs)
		: ChunkDecoder(zhdr, phdr), zhdl(zhdl), zvfs(zvfs) {}
	virtual void read_chdr(int local_chunk_idx, ZF_Chdr *out_chdr);
	virtual void underlying_zarfile_read(
		void *out_buf, uint32_t len, uint32_t offset);
};

void ZarfileHandleChunkDecoder::read_chdr(
	int local_chunk_idx, ZF_Chdr *out_chdr)
{
	// Not holding the lock long enough; in theory a fast_mmap could
	// come along and invalidate this block after we've decided to read
	// it.
	ChdrHolder holder(zvfs, phdr->z_chunk_idx + local_chunk_idx);
	memcpy(out_chdr, holder.get(), sizeof(ZF_Chdr));
}

void ZarfileHandleChunkDecoder::underlying_zarfile_read(
	void *out_buf, uint32_t len, uint32_t offset)
{
	XfsErr err = XFS_DIDNT_SET_ERROR;
	zhdl->read(&err, out_buf, len, offset);
	lite_assert(err == XFS_NO_ERROR);
}

//////////////////////////////////////////////////////////////////////////////

ZarfileHandle::ZarfileHandle(ZarfileVFS *zvfs, ZF_Phdr *phdr, char *url_for_stat)
	: ZFTPDecoder(zvfs->mf, zvfs, url_for_stat, NULL)
{
	this->zvfs = zvfs;
	this->phdr = phdr;
}

ZarfileHandle::~ZarfileHandle()
{
}

bool ZarfileHandle::is_dir()
{
	// return (Z_NTOHG(this->phdr->protocol_metadata.flags) & ZFTP_METADATA_FLAG_ISDIR)!=0;
	// Ugh -- zarfile contents are little-endian. Puke.
	return (phdr->protocol_metadata.flags & ZFTP_METADATA_FLAG_ISDIR)!=0;
}

void* ZarfileHandle::fast_mmap(size_t len, uint64_t offset)
{
	lite_assert(!is_dir());
	for (uint32_t ci = 0; ci<phdr->z_chunk_count; ci++)
	{
		ChdrHolder holder(zvfs, phdr->z_chunk_idx + ci);
		ZF_Chdr *chdr = holder.get();

		if (!chdr->z_precious
			&& chdr->z_data_off <= offset
			&& offset + len <= chdr->z_data_off+chdr->z_data_len)
		{
			// hey, here's a contiguous chunk we can just hand out!
			uint64_t chunk_offset = offset - chdr->z_data_off;
			uint64_t zarfile_offset =  chdr->z_file_off + chunk_offset;

			// return NULL;	// disabled for debugging

			void* result;
			result = zvfs->zarfile_hdl->fast_mmap(len, zarfile_offset);
			if (result==NULL)
			{
				// hmmm. It's no longer available!?
				// (Maybe this can happen if we don't get a fast fetch;
				// and maybe that's just fine.)
				return NULL;
			}

			if ((((uint32_t) result) & 0xfff) != 0)
			{
				// Hmm, can't really give this out due to alignment.
				// see refactoring-notes 2012.07.06
				zvfs->lost_mmap_opportunities_misaligned += len;
				return NULL;
			}

			// mark our copy of this chunk as "consumed" so we don't try
			// to mmap or read it again, since we've just told the app it's
			// free to rewrite the bytes.
			// We'll save any bytes off the end of the chunk, since this
			// may actually keep some libraries (whose data segs don't
			// overlap a page with text segs) from needing a separate chunk.
			uint32_t old_end = chdr->z_data_off + chdr->z_data_len;
			uint32_t new_start = offset + len;
			uint32_t offset_delta = new_start - chdr->z_data_off;
			chdr->z_file_off += offset_delta;
			chdr->z_data_off += offset_delta;
			chdr->z_data_len = old_end - new_start;
			zvfs->exploited_mmap_opportunities += len;

			if (false)
			{
				char buf[256];
				cheesy_snprintf(buf, sizeof(buf),
					"H ZarfileHandle::fast_mmap at %08x len %08x came from zarfile offset %08x (chunk index %d file_off %08x data_off %08x data_len %08x offset_delta %08x)\n",
					result, len, (uint32_t) zarfile_offset,
					ci,
					chdr->z_file_off,
					chdr->z_data_off,
					chdr->z_data_len,
					offset_delta);
				ZLC_TERSE(zvfs->ze, buf);
			}

			return result;
		}
	}
	return NULL;
}

void ZarfileHandle::_internal_read(void *buf, uint32_t count, uint32_t offset)
{
	ZarfileHandleChunkDecoder cd(&zvfs->zhdr, phdr, zvfs->zarfile_hdl, zvfs);
	cd.read(buf, count, offset);
}

uint64_t ZarfileHandle::get_file_len()
{
	return phdr->z_file_len;
}

//////////////////////////////////////////////////////////////////////////////

#define SANITY(c)	{ if (!(c)) { ZLC_COMPLAIN(ze, "ZarfileVFS failing unexpectedly: " #c "\n"); goto fail; } }

ZarfileVFS::ZarfileVFS(MallocFactory *mf, SyncFactory* sf, XaxVFSHandleIfc *zarfile_hdl, ZLCEmit *ze)
{
	ptable = NULL;
	strtable = NULL;
	chdr = NULL;
	chdr_mutex = NULL;
	this->mf = mf;
	this->zarfile_hdl = zarfile_hdl;
	this->ze = ze;
	lost_mmap_opportunities_misaligned = 0;
	exploited_mmap_opportunities = 0;

	XfsErr err;

	zarfile_hdl->read(&err, &zhdr, sizeof(zhdr), 0);
	SANITY(err==XFS_NO_ERROR);

	SANITY(zhdr.z_magic == Z_MAGIC);
	SANITY(zhdr.z_version == Z_VERSION);
	SANITY(zhdr.z_path_entsz == sizeof(ZF_Phdr));

	uint32_t ptable_size;
	ptable_size = sizeof(ZF_Phdr)*zhdr.z_path_num;
	ptable = (ZF_Phdr*) mf_malloc(mf, ptable_size);
	zarfile_hdl->read(&err, ptable, ptable_size, zhdr.z_path_off);
	SANITY(err==XFS_NO_ERROR);

	strtable = (char*) mf_malloc(mf, zhdr.z_strtab_len);
	zarfile_hdl->read(&err, strtable, zhdr.z_strtab_len, zhdr.z_strtab_off);
	SANITY(err==XFS_NO_ERROR);
	SANITY(strtable[zhdr.z_strtab_len-1]=='\0');

	// Read in an in-memory copy of all the chdrs. We update it when we
	// give away chunks via fast_mmap, so we need to make sure all
	// accesses of this zarfile use the in-memory (updated) chdrs.
	int chdr_len;
	chdr_len = sizeof(ZF_Chdr)*zhdr.z_chunktab_count;
	this->chdr = (ZF_Chdr*) mf_malloc(mf, chdr_len);
	chdr_mutex = sf->new_mutex(false);
	err = XFS_DIDNT_SET_ERROR;
	zarfile_hdl->read(&err, this->chdr, chdr_len, zhdr.z_chunktab_off);
	SANITY(err==XFS_NO_ERROR);

	return;

fail:
	if (ptable!=NULL)
	{
		mf_free(mf, ptable);
		ptable=NULL;
	}
	if (strtable!=NULL)
	{
		mf_free(mf, strtable);
		strtable=NULL;
	}
	zhdr.z_path_num = 0;	// cause lookups to fail.
}

ZarfileVFS::~ZarfileVFS()
{
	if (ptable!=NULL)
	{
		mf_free(mf, ptable);
		ptable=NULL;
	}
	if (strtable!=NULL)
	{
		mf_free(mf, strtable);
		strtable=NULL;
	}
	if (chdr!=NULL) { mf_free(mf, chdr); }
	delete chdr_mutex;
	
	if (exploited_mmap_opportunities == 0)
	{
		delete zarfile_hdl;
	}
	else
	{
		// can't ever give back the jumbo packet, since
		// the user app has tentacles all pointing into it!
	}
}

// Want to pass a "this" into cmp routine, so we can see strtable,
// but binsearch isn't thinking object-orientedly.
typedef struct {
	ZarfileVFS *zvfs;
	const char *key;
} ZarfileAndKey;

int ZarfileVFS::_phdr_key_cmp(const void *v_key, const void *v_member)
{
	ZarfileAndKey *zak = (ZarfileAndKey *) v_key;
	const ZF_Phdr *phdr = (const ZF_Phdr *) v_member;
	return lite_strcmp(zak->key, &zak->zvfs->strtable[phdr->z_path_str_off]);
}

XaxVFSHandleIfc *ZarfileVFS::open(XfsErr *err, XfsPath *path, int oflag, XVOpenHooks *open_hooks)
{
	XaxVFSHandleIfc *result = NULL;

	char *url_hint, *url_for_stat;
	ZLCVFS::assemble_urls(mf, path, &url_hint, &url_for_stat);

	ZarfileAndKey zak;
	zak.zvfs = this;
	zak.key = url_hint;
	ZF_Phdr *phdr = (ZF_Phdr*) binsearch(&zak, ptable, zhdr.z_path_num,
		sizeof(ZF_Phdr), _phdr_key_cmp);
	if (phdr!=NULL)
	{
		if (phdr->protocol_metadata.flags & ZFTP_METADATA_FLAG_ENOENT)
		{
			*err = (XfsErr) ENOENT; // We found the path, and it's ENOENT.
		}
		else
		{
			*err = XFS_NO_ERROR;
			result = new ZarfileHandle(this, phdr, url_for_stat);
			url_for_stat = NULL;
		}
	}
	else if (phdr==NULL)
	{
		*err = XFS_NO_INFO;
	}

	mf_free(mf, url_hint);
	if (url_for_stat!=NULL)
	{
		mf_free(mf, url_for_stat);
	}

	return result;
}

void ZarfileVFS::mkdir(
	XfsErr *err, XfsPath *path, const char *new_name, mode_t mode)
{
	*err = XFS_NO_INFO;
}

void ZarfileVFS::stat64(
	XfsErr *err,
	XfsPath *path, struct stat64 *buf)
{
	*err = XFS_ERR_USE_FSTAT64;
}

ZF_Chdr* ZarfileVFS::lock_chdr(uint32_t ci)
{
	lite_assert(ci < zhdr.z_chunktab_count);
	chdr_mutex->lock();
	return &chdr[ci];
}

void ZarfileVFS::unlock_chdr(uint32_t ci, ZF_Chdr* _chdrp)
{
	lite_assert(ci < zhdr.z_chunktab_count);
	lite_assert(&chdr[ci] == _chdrp);
	chdr_mutex->unlock();
}
