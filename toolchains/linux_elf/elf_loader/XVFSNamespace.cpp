#include "XVFSNamespace.h"
#include "LiteLib.h"

XVFSMount::XVFSMount(const char *prefix, XaxVFSIfc *xfs)
{
	this->prefix = prefix;
	this->xfs = xfs;
}

XVFSNamespace::XVFSNamespace(MallocFactory *mf)
{
	linked_list_init(&mounts, mf);
}

void XVFSNamespace::install(XVFSMount *mount)
{
	linked_list_insert_head(&mounts, mount);
}

// Can't use strcmp (doesn't tell you why they differ, if it's just a prefix)
// and can't use memcmp (because we might read past the end of the source
// string) without extra work. So we write our own compare.
int XVFSNamespace::_prefix_match(const char *long_str, const char *prefix)
{
	const char *lp, *pp;
	for (lp=long_str, pp=prefix; ; lp++, pp++)
	{
		if (*lp!=*pp)
		{
			// a difference, possibly including one being zero (end of string)
			if (*pp==0)
			{
				// if the first difference was that the prefix ended,
				// then this is a match.
				return 1;
			}

			// otherwise, lp ended too soon for a match, or they just plain
			// disagree.
			return 0;
		}
		// they're equal. Check if both have ended.
		if (*lp==0 && *pp==0)
		{
			return 1;
		}
	}
}

XVFSMount *XVFSNamespace::lookup(XfsPath *path)
{
	LinkedListIterator lli;
	for (ll_start(&mounts, &lli); ll_has_more(&lli); ll_advance(&lli))
	{
		XVFSMount *mount = (XVFSMount *) ll_read(&lli);
		if (_prefix_match(path->pathstr, mount->prefix))
		{
			path->suffix = path->pathstr + lite_strlen(mount->prefix);
			return mount;
		}
	}
	return NULL;
}
