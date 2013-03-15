#include <errno.h>
#include "LiteLib.h"
#include "UnionVFS.h"

UnionVFS::UnionVFS(MallocFactory *mf)
{
	linked_list_init(&layers, mf);
}

UnionVFS::~UnionVFS()
{
	// linked_list_free(&layers);
	lite_assert(false);	// never deleted in practice
}

void UnionVFS::push_layer(XaxVFSIfc *vfs)
{
	linked_list_insert_head(&layers, vfs);
}

XaxVFSIfc *UnionVFS::pop_layer()
{
	return (XaxVFSIfc*) linked_list_remove_head(&layers);
}

XaxVFSHandleIfc *UnionVFS::open(
	XfsErr *err, XfsPath *path, int oflag, XVOpenHooks *open_hooks)
{
	LinkedListIterator li;
	for (ll_start(&layers, &li); ll_has_more(&li); ll_advance(&li))
	{
		XaxVFSIfc *vfs = (XaxVFSIfc *) ll_read(&li);
		XaxVFSHandleIfc *result = vfs->open(err, path, oflag);
		if (*err!=XFS_NO_INFO)
		{
			return result;
		}
		lite_assert(result==NULL);
	}
	// fell through with no replies.
	*err = (XfsErr) ENOENT;
	return NULL;
}

void UnionVFS::mkdir(
	XfsErr *err, XfsPath *path, const char *new_name, mode_t mode)
{
	LinkedListIterator li;
	for (ll_start(&layers, &li); ll_has_more(&li); ll_advance(&li))
	{
		XaxVFSIfc *vfs = (XaxVFSIfc *) ll_read(&li);
		vfs->mkdir(err, path, new_name, mode);
		if (*err!=XFS_NO_INFO)
		{
			return;
		}
	}
	*err = (XfsErr) ENOENT;
}

void UnionVFS::stat64(
	XfsErr *err,
	XfsPath *path, struct stat64 *buf)
{
	LinkedListIterator li;
	for (ll_start(&layers, &li); ll_has_more(&li); ll_advance(&li))
	{
		XaxVFSIfc *vfs = (XaxVFSIfc *) ll_read(&li);
		vfs->stat64(err, path, buf);
		if (*err!=XFS_NO_INFO)
		{
			return;
		}
	}
	*err = (XfsErr) ENOENT;
}
