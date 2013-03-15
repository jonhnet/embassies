#include <fcntl.h>	// O_RDWR
#include <dirent.h>
#include <alloca.h>
#include <errno.h>

#include "LiteLib.h"
#include "XRDirNode.h"
#include "XRFileNode.h"

XRDirEnt::XRDirEnt(char *name, XRNode *xrnode)
{
	this->name = name;
	this->xrnode = xrnode;
}

XRDirNode::XRDirNode(XRamFS *xramfs)
	: XRNode(xramfs)
{
	linked_list_init(&entries, xramfs->get_mf());
}

XRNode *XRDirNode::_create_per_hooks(int oflag, XVOpenHooks *xoh)
{
	lite_assert((oflag&O_CREAT) != 0);

	XRNode *xrnode = NULL;
	if (xoh!=NULL)
	{
		xrnode = xoh->create_node();
	}
	if (xrnode==NULL)
	{
		xrnode = new XRFileNode(get_xramfs());
	}
	return xrnode;
}

XRNodeHandle *XRDirNode::open_node(XfsErr *err, const char *path, int oflag, XVOpenHooks *xoh)
{
	XRNodeHandle *result = NULL;

	if (path[0] == '\0')
	{
		// Hey, this is the dir we want open. Provide a directory
		// handle.
		*err = XFS_NO_ERROR;
		result = new XRDirNodeHandle(this);
	}
	else
	{
		if (path[0] == '/')
		{
			// trim a leading /.
			// If we're mounted at "/tmp", an open of "/tmp/home" will
			// have path->suffix "/home"; "home" is the first name we
			// should open. We don't want to mount at "/tmp/", because then
			// no-one can open "/tmp" and find us.
			path+=1;
		}

		int _len = lite_strlen(path)+1;
		char *next_name = (char*) alloca(_len);
		lite_memcpy(next_name, path, _len);

		char *sep = lite_index(next_name, '/');
		const char *rest;
		if (sep==NULL)
		{
			rest = "";
		}
		else
		{
			if (sep[1]=='\0')
			{
				// found a trailing '/'
				rest = "";
			}
			else
			{
				rest = sep+1;
			}
			sep[0] = '\0';
		}

		LinkedListIterator lli;
		for (ll_start(&entries, &lli); ll_has_more(&lli); ll_advance(&lli))
		{
			XRDirEnt *dirent = (XRDirEnt *) ll_read(&lli);

			if (lite_strcmp(dirent->name, next_name)==0)
			{
				result = dirent->xrnode->open_node(err, rest, oflag, xoh);
				break;
			}
		}
		if (result==NULL)
		{
			if ((oflag&O_CREAT)!=0 && rest[0]=='\0')
			{
				// caller wants to create a file here
				XRNode *child = _create_per_hooks(oflag, xoh);
				_insert_child(mf_strdup(xramfs->get_mf(), next_name), child);
				result = child->open_node(err, rest, oflag, xoh);
			}
			else
			{
				*err = (XfsErr) ENOENT;
				result = NULL;
			}
		}
	}
	return result;
}

void XRDirNode::_insert_child(const char *name, XRNode *child)
{
	XRDirEnt *child_dirent = new XRDirEnt(
		mf_strdup(xramfs->get_mf(), name), child);

	// TODO ought to lock, huh?
	linked_list_insert_tail(&entries, child_dirent);
}

void XRDirNode::mkdir_node(XfsErr *err, const char *new_name, mode_t mode)
{
	LinkedListIterator lli;
	for (ll_start(&entries, &lli); ll_has_more(&lli); ll_advance(&lli))
	{
		XRDirEnt *dirent = (XRDirEnt *) ll_read(&lli);
		if (lite_strcmp(dirent->name, new_name)==0)
		{
			*err = (XfsErr) EEXIST;
			return;
		}
	}

	_insert_child(new_name, new XRDirNode(get_xramfs()));
	*err = XFS_NO_ERROR;
}

void XRDirNode::unlink_node(XfsErr *err, char *child)
{
	LinkedListIterator lli;
	for (ll_start(&entries, &lli); ll_has_more(&lli); ll_advance(&lli))
	{
		XRDirEnt *dirent = (XRDirEnt *) ll_read(&lli);
		if (lite_strcmp(dirent->name, child)==0)
		{
			linked_list_remove(&entries, dirent);
			*err = XFS_NO_ERROR;
			return;
		}
	}
	*err = (XfsErr) ENOENT;
}

uint32_t XRDirNode::get_dir_len(XfsErr *err)
{
	*err = XFS_NO_ERROR;
	return entries.count;
}

uint32_t XRDirNode::getdent64(
		XfsErr *err, struct xi_kernel_dirent64 *dirp,
		uint32_t item_offset, uint32_t item_count)
{
	uint32_t outi = 0;
	uint32_t entryi;
	LinkedListIterator lli;
	for (ll_start(&entries, &lli), entryi=0;
		ll_has_more(&lli);
		ll_advance(&lli), entryi+=1)
	{
		if (entryi >= item_offset
			&& entryi < item_offset+item_count)
		{
			XRDirEnt *dirent = (XRDirEnt *) ll_read(&lli);
			dirp[outi].d_ino = 17;
			dirp[outi].d_off = sizeof(struct xi_kernel_dirent64)*entryi;
			dirp[outi].d_reclen = sizeof(struct xi_kernel_dirent64);
			dirp[outi].d_type = DT_FIFO;
#if 0
			debug_write_sync("Um, TODO: every ramfs file is a FIFO?\n");
#endif
			lite_memset(dirp[outi].d_name, 0, sizeof(dirp[outi].d_name));
			lite_strcpy(dirp[outi].d_name, dirent->name);
			outi += 1;
		}
	}

	*err = XFS_NO_ERROR;
	return outi;
}

XRDirNodeHandle::XRDirNodeHandle(XRDirNode *dir_node)
	: XRNodeHandle(dir_node)
{
}

uint64_t XRDirNodeHandle::get_file_len()
{
	lite_assert(false);	// not sure how to do anything sane here
	return 0;
}

void XRDirNodeHandle::xvfs_fstat64(XfsErr *err, struct stat64 *buf)
{
	uint32_t size = 512;	// booooogus.
	// hey, if it's open, it must be a dir!
	buf->st_dev = 17;
	buf->st_ino = (uint32_t) get_xrnode();
	buf->st_mode = S_IRWXU | S_IFDIR;
	buf->st_nlink = 2;
	buf->st_uid = get_xrnode()->get_xramfs()->get_fake_ids()->uid;
	buf->st_gid = get_xrnode()->get_xramfs()->get_fake_ids()->gid;
	buf->st_rdev = 0;
	buf->st_size = size;
	buf->st_blksize = 512;
	buf->st_blocks = (size+buf->st_blksize-1) / buf->st_blksize;
	buf->st_atime = 1272062690;	// and was created at the same moment,
	buf->st_mtime = 1272062690;	// coincidentally contemporaneous with the
	buf->st_ctime = 1272062690;	// moment I wrote this function! wild!
	*err = XFS_NO_ERROR;
}

uint32_t XRDirNodeHandle::get_dir_len(XfsErr *err)
{
	return _get_dir_node()->get_dir_len(err);
}

uint32_t XRDirNodeHandle::getdent64(
		XfsErr *err, struct xi_kernel_dirent64 *dirp,
		uint32_t item_offset, uint32_t item_count)
{
	return _get_dir_node()->getdent64(err, dirp, item_offset, item_count);
}
