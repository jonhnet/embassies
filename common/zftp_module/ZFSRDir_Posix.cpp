#include <dirent.h>
#include <malloc.h>
#include <sys/stat.h>

#include "LiteLib.h"
#include "ZFSRDir_Posix.h"
#include "standard_malloc_factory.h"

ZFSRDir_Posix::ZFSRDir_Posix(const char *path)
	: _path(path)
{
}

bool ZFSRDir_Posix::fetch_stat()
{
	struct stat statbuf;
	int rc = stat(_path, &statbuf);
	if (rc!=0)
	{
		return false;
	}
	fill_from_stat(&statbuf);
	return true;
}

bool ZFSRDir_Posix::scan()
{
	MallocFactory *mf = standard_malloc_factory_init();
	bool result = false;
	DIR *dir = opendir(_path);
	if (dir==NULL)
	{
		result = false;
		goto exit;
	}

	while (1)
	{
		struct dirent entry, *result;

		int rc = readdir_r(dir, &entry, &result);
		if (rc!=0)
		{
			fprintf(stderr, "WARNING _zfdir_scan readdir failure\n");
			perror("readdir");
			result = false;
			goto exit;
		}
		if (result==NULL)
		{
			break;
		}

		ZFTPDirectoryRecord zdr;
		zdr.type = result->d_type;
		zdr.inode_number = result->d_ino;
		HandyDirRec *hdr = new HandyDirRec(mf, &zdr, result->d_name);
		scan_add_one(hdr);
	}

	result = true;

exit:
	if (dir!=NULL)
	{
		closedir(dir);
	}
	return result;
}

