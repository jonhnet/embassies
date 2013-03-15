#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

#include "LiteLib.h"
#include "ZFSOrigin.h"
#include "ZFSReader.h"
#include "ZOriginFile.h"

#ifdef _WIN32
#include <Windows.h>
#define PATH_MAX            512
#endif

ZFSOrigin::ZFSOrigin(ZCache *zcache)
	: zcache(zcache),
	  file_system_view(zcache->mf, zcache->sf)
{
	file_system_view.insert_overlay("");
}

bool ZFSOrigin::lookup_new(const char *url)
{
	bool result;
	const char *path = _url_to_path(url);
	if (path==NULL)
	{
		return false;
	}

	ZFSReader *zfsr = file_system_view.open_path(path);

	ZNameRecord *znr;
	FreshnessSeal seal;

	if (zfsr==NULL) {
		// file not found
		seal = FreshnessSeal(1);
		hash_t zh = get_zero_hash();
		znr = new ZNameRecord(this, &zh, &seal);
	}
	else
	{
		bool rc;
		uint32_t filelen;
		uint8_t buf[ZFTP_BLOCK_SIZE];
		rc = zfsr->get_filelen(&filelen);
		assert(rc); if (!rc) { goto fail; }

		bool isdir;
		rc = zfsr->is_dir(&isdir);
		assert(rc); if (!rc) { goto fail; }

		int flags = 0;
		if (isdir)
		{
			flags = ZFTP_METADATA_FLAG_ISDIR;
		}

		ZOriginFile *origin_file;
		origin_file = new ZOriginFile(zcache, filelen, flags);
		uint32_t offset;
		uint32_t blk;
		for (offset=0, blk=0;
			offset<filelen;
			blk+=1)
		{
			// TODO copy-tastic
			uint32_t bytes_read = zfsr->get_data(offset, buf, ZFTP_BLOCK_SIZE);
			origin_file->origin_load_block(blk, buf, bytes_read);
			offset += bytes_read;
		}
		assert(offset == filelen);
		origin_file->origin_finish_load();

		seal = FreshnessSeal(_all_links_mtime(path));
		znr = new ZNameRecord(this, &origin_file->file_hash, &seal);
		delete origin_file;
	}

	zcache->insert_name(url, znr);
	delete znr;
	result = true;
	goto finally;

fail:
	result = false;

finally:
	if (zfsr!=NULL)
	{
		delete zfsr;
	}
	return result;
}

// TODO: mtime code in ZFSR* is dead code, and doesn't deeply account
// for links. We're using this code instead. Clean up that code.
FreshnessSeal ZFSOrigin::_all_links_mtime(const char *data_path)
{
	time_t latest_mtime = 0;
#ifdef _WIN32
	FILETIME modified_time;
	HANDLE file = CreateFile(data_path, GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		// invalid path
		return FreshnessSeal(1);
	}
	if (GetFileTime(file, NULL, NULL, &modified_time) ) {
		latest_mtime = (time_t) ((modified_time.dwHighDateTime << sizeof(DWORD)) & modified_time.dwLowDateTime);
		CloseHandle(file);
	} else {
		// Failed to retrieve time
		CloseHandle(file);
		return FreshnessSeal(1);
	}
#else
	int rc;	
	char pathbuf[PATH_MAX], newbuf[PATH_MAX];
	lite_strncpy(pathbuf, data_path, sizeof(pathbuf));
	int limit = 100;

	// TODO there's a bug in here: we need to walk the whole path
	// checking for links, as we might read right through one

	while (true)
	{
		struct stat statbuf;
		rc = lstat(pathbuf, &statbuf);
		if (rc!=0)
		{
			// invalid mtime in symlink chain
			// fprintf(stderr, "unreadable mtime '%s'\n", pathbuf);
			return FreshnessSeal(1);
		}
		if (statbuf.st_mtime > latest_mtime)
		{
			latest_mtime = statbuf.st_mtime;
		}
		if (!S_ISLNK(statbuf.st_mode))
		{
			break;
		}
		// this is a symlink; deref and check.
		rc = readlink(pathbuf, newbuf, sizeof(newbuf));
		if (rc<0 || rc>=(int)sizeof(newbuf))
		{
			fprintf(stderr, "unreadable symlink '%s'\n", pathbuf);
			return FreshnessSeal(1);
		}
		newbuf[rc] = '\0';

		if (newbuf[0]=='/')
		{
			lite_strncpy(pathbuf, newbuf, sizeof(pathbuf));
		}
		else
		{
			// truncate pathbuf to dir part
			char *last_slash = lite_rindex(pathbuf, '/');
			if (last_slash==NULL)
			{
				pathbuf[0] = '\0';
			}
			else
			{
				last_slash[1] = '\0';
			}
			// and append link contents
			lite_strncat(pathbuf, newbuf, sizeof(pathbuf));
		}

		limit -= 1;
		if (limit==0)
		{
			fprintf(stderr, "symlink loop.\n");
			return FreshnessSeal(1);
		}
	}
#endif // _WIN32
	return FreshnessSeal(latest_mtime);
}

const char *ZFSOrigin::_url_to_path(const char *url)
{
	if (lite_starts_with(FILE_SCHEME, url))
	{
		return &url[lite_strlen(FILE_SCHEME)];
	}
	return NULL;
}

bool ZFSOrigin::validate(const char *url, FreshnessSeal *seal)
{
	const char *path = _url_to_path(url);
	if (path==NULL) {
		return false;
	}
	FreshnessSeal new_seal = _all_links_mtime(path);
	return (seal->cmp(&new_seal)==0);
}
