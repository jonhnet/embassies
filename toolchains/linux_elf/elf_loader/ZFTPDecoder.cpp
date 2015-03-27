#include <errno.h>

#include "LiteLib.h"
#include "ZFTPDecoder.h"
#include "zftp_unix_metadata_format.h"
#include "xax_posix_emulation.h"	// xnull_fstat64

ZFTPDecoder::ZFTPDecoder(MallocFactory *mf, XaxVFSIfc *xvfs_for_stat, char *url_for_stat, TraceCollectorHandle* traceHandle)
{
	this->mf = mf;
	this->xvfs_for_stat = xvfs_for_stat;
	this->url_for_stat = url_for_stat;
	this->traceHandle = traceHandle;

	dir_loaded = false;
	dirent_count = -1;
	dirent_offsets = NULL;
	stat_loaded = false;
}

ZFTPDecoder::~ZFTPDecoder()
{
	if (dirent_offsets != NULL)
	{
		mf_free(mf, dirent_offsets);
	}
	if (url_for_stat != NULL)
	{
		mf_free(mf, url_for_stat);
	}
	delete traceHandle;
}

ZFTPDirectoryRecord *ZFTPDecoder::_read_record(uint32_t offset)
{
	uint32_t reclen;
	_internal_read((uint8_t*) &reclen, sizeof(reclen), offset);
	lite_assert(reclen>=sizeof(ZFTPDirectoryRecord));
	lite_assert(reclen<1024); // sanity-check
	ZFTPDirectoryRecord *zdr = (ZFTPDirectoryRecord *)
		mf_malloc(mf, reclen);
	zdr->reclen = reclen;
	// read rest of record
	uint32_t remainder_len = reclen-sizeof(uint32_t);
	_internal_read(((uint8_t*) zdr)+sizeof(uint32_t),
		remainder_len, offset+sizeof(reclen));
	return zdr;
}

uint32_t ZFTPDecoder::_scan_dir_index()
{
	uint64_t file_len = get_file_len();
	uint32_t offset = 0;
	uint32_t index = 0;
	while (offset < file_len)
	{
		ZFTPDirectoryRecord *zdr = _read_record(offset);
		if (dirent_offsets!=NULL)
		{
			dirent_offsets[index] = offset;
		}
		offset += zdr->reclen;
		mf_free(mf, zdr);
		index += 1;
	}
	return index;
}

void ZFTPDecoder::_init_dir_index()
{
	// someday maybe the wire format will include an index, so we can
	// read directories some clevererer way.
	if (dir_loaded)
	{
		return;
	}

	dirent_count = _scan_dir_index();
	dirent_offsets = (uint32_t*) mf_malloc(
		mf, dirent_count*sizeof(uint32_t));
	uint32_t count2 = _scan_dir_index();
	lite_assert(count2==dirent_count);
	dir_loaded = true;
}

void ZFTPDecoder::read(
	XfsErr *err, void *dst, size_t len, uint64_t offset)
{
	if (is_dir())
	{
		*err = (XfsErr) EBADF;
		return;
	}

	if (traceHandle!=NULL)
	{
		traceHandle->read(len, offset);
	}
	_internal_read(dst, len, offset);

	*err = XFS_NO_ERROR;
}

void ZFTPDecoder::trace_mmap(size_t len, uint64_t offset, bool fast)
{
	if (traceHandle!=NULL)
	{
		traceHandle->mmap(len, offset, fast);
	}
}

void ZFTPDecoder::xvfs_fstat64(
	XfsErr *err, struct stat64 *buf)
{
	_zlcfs_fill_statbuf();

	lite_memcpy(buf, &this->statbuf, sizeof(*buf));
	*err = XFS_NO_ERROR;
}

uint32_t ZFTPDecoder::get_dir_len(XfsErr *err)
{
	if (!is_dir())
	{
		*err = (XfsErr) EBADF;
		return 0;
	}

	_init_dir_index();

	*err = XFS_NO_ERROR;
	return dirent_count;
}

uint32_t ZFTPDecoder::getdent64(
	XfsErr *err, struct xi_kernel_dirent64 *dirp, uint32_t item_offset, uint32_t item_count)
{
	if (!is_dir())
	{
		*err = (XfsErr) EBADF;
		return 0;
	}

	_init_dir_index();

	uint32_t item, di;
	for (item=item_offset, di=0; item<dirent_count && di<item_count; item++, di++)
	{
		uint32_t offset = dirent_offsets[item];
		ZFTPDirectoryRecord *zdr = _read_record(offset);

		dirp[di].d_ino = zdr->inode_number;
		dirp[di].d_off = 0;
		dirp[di].d_reclen = sizeof(struct xi_kernel_dirent64);
		dirp[di].d_type = zdr->type;
		lite_memset(dirp[di].d_name, 0, sizeof(dirp[di].d_name));
		char *filename = ZFTPDirectoryRecord_filename(zdr);
		lite_memcpy(dirp[di].d_name,
			filename,
			lite_strnlen(filename, sizeof(dirp[di].d_name)));
	}

	*err = XFS_NO_ERROR;
	return di;
}

void ZFTPDecoder::_zlcfs_fill_statbuf()
{
	if (stat_loaded)
	{
		return;
	}

#if 0
#endif
#define MAX_REASONABLE_STAT_STREAM_LEN 4096
	XaxVFSHandleIfc *hdl = NULL;
	ZFTPUnixMetadata zum;
	bool zum_valid = false;
	do {	/* once */
		if (url_for_stat==NULL)
			{ break; }

		XfsPath path = { (char*) RAW_URL_SENTINEL, url_for_stat };
		XfsErr err;
		XaxVFSHandleIfc *hdl = xvfs_for_stat->open(&err, &path, O_RDONLY);
		if (err!=XFS_NO_ERROR)
			{ break; }

		uint64_t file_len = hdl->get_file_len();
		if (file_len > MAX_REASONABLE_STAT_STREAM_LEN)
			{ break; }
		if (file_len < sizeof(ZFTPUnixMetadata))
			{ break; }

		hdl->read(&err, &zum, sizeof(zum), /*offset*/ 0);
		if (err!=XFS_NO_ERROR)
			{ break; }

		zum_valid = true;
	} while (0);
	if (hdl!=NULL)
	{
		delete hdl;
	}

	XfsErr dummy;
	xnull_fstat64(&dummy, &statbuf, get_file_len());

	if (zum_valid)
	{
		statbuf.st_dev = z_ntohw(zum.dev);
		statbuf.st_ino = z_ntohw(zum.ino);
		statbuf.st_atime = z_ntohw(zum.mtime);
		statbuf.st_mtime = z_ntohw(zum.mtime);
		statbuf.st_ctime = z_ntohw(zum.mtime);
	} else {
		static int fake_ino_supply = 1800;
		statbuf.st_dev = 17;
		statbuf.st_ino = fake_ino_supply++;
		statbuf.st_atime = 1272062690;
		statbuf.st_mtime = 1272062690;
		statbuf.st_ctime = 1272062690;
	}

	if (is_dir())
	{
		statbuf.st_mode = S_IFDIR | S_IRUSR | S_IXUSR;
	}
	else
	{
		statbuf.st_mode = S_IFREG | S_IRUSR | S_IXUSR;
	}
	stat_loaded = true;
}


