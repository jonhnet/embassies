#include <stdlib.h>

#include "FileSystemView.h"

#include "math_util.h"
#include "LiteLib.h"
#include "zftp_protocol.h"
#include "reference_file.h"

#include "ZFSReader.h"
#include "ZFSRFile.h"
#include "ZFSRStat.h"
#include "ZFSReference.h"
#if defined(__USE_POSIX)
#include "ZFSRDir_Posix.h"
#else
#include "ZFSRDir_Win.h"
#endif
#include "ZFSRDir_Union.h"

FileSystemView::FileSystemView(MallocFactory *mf, SyncFactory *sf, bool hide_icons_dir)
	: mf(mf), sf(sf), hide_icons_dir(hide_icons_dir), max_prefix(0)
{
	linked_list_init(&overlays, mf);
}

FileSystemView::~FileSystemView()
{
	while (overlays.count>0)
	{
		char *path = (char*) linked_list_remove_head(&overlays);
		mf_free(mf, path);
	}
}

void FileSystemView::insert_overlay(const char *path)
{
	linked_list_insert_head(&overlays, mf_strdup(mf, path));
	max_prefix = max(max_prefix, lite_strlen(path));
}

ZFSReader *FileSystemView::_open_one_real_path(const char *path)
{
	ZFSRFile *zrf = new ZFSRFile();
	bool rc = zrf->open(path);
	if (rc)
	{
		return zrf;
	}
	delete zrf;

	ZFSRDir *zrd;
#if defined(__USE_POSIX)
	zrd = new ZFSRDir_Posix(path);
#else
	zrd = new ZFSRDir_Win(path);
#endif
	rc = zrd->open();
	if (rc)
	{
		return zrd;
	}
	delete zrd;

	return NULL;
}
	

ZFSReader *FileSystemView::open_path(const char *path)
{
	ZFSReader *zfsr = NULL;

	// Ugly manual neutering, since overlay mechanism doesn't give
	// a way to kill a directory tree.
	if (hide_icons_dir && lite_starts_with("/usr/share/icons/hicolor", path))

	{
		return NULL;
	}

	char *real_path = (char*) mf_malloc(mf, lite_strlen(path)+max_prefix+1);
	LinkedListIterator lli;
	for (ll_start(&overlays, &lli); ll_has_more(&lli); ll_advance(&lli))
	{
		char *prefix = (char*) ll_read(&lli);
		lite_strcpy(real_path, prefix);
		lite_strcat(real_path, path);
		ZFSReader *new_reader = _open_one_real_path(real_path);
		if (new_reader!=NULL)
		{
			bool isdir;
			bool rc = new_reader->is_dir(&isdir);
			lite_assert(rc);
			if (!isdir)
			{
				zfsr = new_reader;
				break;
			}
			else if (zfsr==NULL)
			{
				zfsr = new_reader;
			}
			else
			{
//				fprintf(stderr, "union at %s\n", path);
				ZFSRDir_Union *zru = new ZFSRDir_Union(
					mf, sf, (ZFSRDir *) zfsr, (ZFSRDir *) new_reader);
				rc = zru->open();
				delete zfsr;
				delete new_reader;
				zfsr = zru;
			}
		}
	}
	return zfsr;
}

ZFSReader *FileSystemView::open_url(const char *url_hint)
{
	if (lite_starts_with(FILE_SCHEME, url_hint))
	{
		const char *path = &url_hint[lite_strlen(FILE_SCHEME)];
		return open_path(path);
	}
	else if (lite_starts_with(STAT_SCHEME, url_hint))
	{
		const char *path = &url_hint[lite_strlen(STAT_SCHEME)];
		ZFSReader *backing = open_path(path);
		if (backing==NULL)
			{ return NULL; }
		ZFSRStat *zr = new ZFSRStat();
		bool rc = zr->open(backing);
		lite_assert(rc);
		return zr;
	}
	else if (lite_starts_with(REFERENCE_FILE_SCHEME, url_hint))
	{
		int pattern_selector = atoi(&url_hint[lite_strlen(REFERENCE_FILE_SCHEME)]);
		ZFSReader *zr = new ZFSReference(mf, pattern_selector);
		return zr;
	}
	return NULL;
}


