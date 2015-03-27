#include <string.h>
#include <errno.h>

// System includes needed to define posix-interface argument types.
#include <sys/uio.h>			// struct iovec
#include <sys/poll.h>			// struct pollfds
#include <signal.h>				// struct sigaction
#include <sched.h>				// struct sched_param
#include <sys/resource.h>		// struct rlimit
#include <sys/times.h>			// struct tms
#include <sys/statfs.h>			// struct statfs
#include <sys/mman.h>			// MAP_FIXED
#include <utime.h>				// struct utimbuf

#include <sys/utsname.h>	// struct utsname
#include <fcntl.h>			// O_DIRECT
#include <sys/time.h>		// TIMEVAL_TO_TIMESPEC
#include <linux/futex.h>	// FUTEX_WAKE, FUTEX_PRIVATE_FLAG...
#include <time.h>	// CLOCK_REALTIME, CLOCK_MONOTONIC

#include <linux/unistd.h>	// syscall __NR_* definitions
#include <asm/ldt.h>		// struct user_desc
#include <dirent.h>			// struct dirent

#include "LiteLib.h"
#include "xax_posix_emulation.h"
#include "handle_table.h"
#include "xax_util.h"
#include "xpe_route_macros.h"
#include "simpipe.h"
#include "XRamFS.h"
#include "xprocfakeryfs.h"
#include "xinterpose.h"		// RESTORE_USER_REGS
#include "xi_network.h"
#include "xax_unixdomainsockets.h"
#include "xoverlayfs.h"
#include "xrandomfs.h"
#include "xax_extensions.h"
#include "invoke_debugger.h"
#include "pal_abi/pal_extensions.h"
#include "malloc_factory_operator_new.h"
#include "zlcvfs_init.h"
#include "cheesy_snprintf.h"
#include "SyncFactory_Zutex.h"
#include "XVFSHandleWrapper.h"
#include "math_util.h"
#include "DecomposePath.h"
#include "ZutexHandle.h"
#include "perf_measure.h"
#include "KeyValFS.h"

//#define COUNT_TOUCHED_PAGES 1

void install_default_filesystems(XaxPosixEmulation *xpe, StartupContextFactory *scf);
uint32_t _get_gs_descriptor_idx(void);
static int fake_times(struct tms *buffer);
static void strace_time_ifc(void *v_this, int* out_real_ms, int* out_user_ms);

void xpe_init(XaxPosixEmulation *xpe, ZoogDispatchTable_v1 *zdt, uint32_t stack_base, StartupContextFactory *scf)
{
	fake_ids_init(&xpe->fake_ids);

	xpe->zdt = zdt;

	// Using cheesy_malloc because libc malloc depends
	// on a sane TLS (%gs) pointer, which not all of our
	// threads have, and even when they do, they
	// disagree about which universe they're in.
	zoog_malloc_factory_init(&xpe->zmf, xpe->zdt);
	xpe->mf = zmf_get_mf(&xpe->zmf);

	xpe->handle_table = new HandleTable(xpe->mf);

	xpe->mtf = NULL;
	xpe->xvfs_namespace = new XVFSNamespace(xpe->mf);

	xpe->debug_cpu_times = (debug_cpu_times_f *)
		(xpe->zdt->zoog_lookup_extension)(DEBUG_CPU_TIMES);
	if (xpe->debug_cpu_times==NULL)
	{
		xpe->debug_cpu_times = fake_times;
	}

	// Reset xaxPortMonitor.
	if (xpe->zdt != NULL)
	{
		typedef void (*debug_reset_port_t)(void);
		debug_reset_port_t debug_reset_port_f = (debug_reset_port_t)
			((xpe->zdt->zoog_lookup_extension)("debug_reset_port"));
		if (debug_reset_port_f != NULL)
		{
			debug_reset_port_f();
		}

		xpe->mtf = (MallocTraceFuncs*)
			((xpe->zdt->zoog_lookup_extension)("malloc_trace_funcs"));
//		xpe->ptf = (PageTraceFuncs*)
//			((xpe->zdt->zoog_lookup_extension)(DEBUG_TRACE_TOUCHED_PAGES));

	}

	xpe->thread0_initial_gs = &xpe->thread0_initial_gs;
	(xpe->zdt->zoog_x86_set_segments)(0, (uint32_t) xpe->thread0_initial_gs);

	thread_table_init(&xpe->thread_table, xpe->mf, xpe->zdt);

	int first_thread_id = thread_table_allocate_pseudotid(&xpe->thread_table);
	xst_init(&xpe->stack_table, xpe->mf, xpe->zdt);

	XaxStackTableEntry xte;
	xte.gs_thread_handle = get_gs_base();
	xte.stack_base = (void*) stack_base;
	xte.tid = first_thread_id;
	xst_insert(&xpe->stack_table, &xte);

	xpe->perf_measure = new PerfMeasure(xpe->zdt);

	xpe->xax_skinny_network = new XaxSkinnyNetwork(xpe->zdt, xpe->mf);

//	xpe->xax_skinny_network->enable_random_drops(true);

	strace_init(&xpe->strace, xpe->mf, xpe->zdt, strace_time_ifc, xpe);

	xpe->xbrk.init(xpe->zdt, xpe->zmf.brk_mmap_ifc, xpe->mtf);

	// allocate brks for initial ld.so & libc, for until then,
	// it's pretty hard to call down into here.
	xpe->xbrk.allow_new_brk(1<<12, "zguest:ld.so");
	xpe->xbrk.allow_new_brk(0x23<<12, "zguest:libc");

	xpe->strace_flag = scf_lookup_env_bool(scf, "ZOOG_STRACE");
	xpe->mmap_flag = scf_lookup_env_bool(scf, "ZOOG_MMAP");
	xpe->trace_flag = scf_lookup_env_bool(scf, "ZOOG_TRACE");

	install_default_filesystems(xpe, scf);

	xpe->_initted = 1;
}

void install_default_filesystems(XaxPosixEmulation *xpe, StartupContextFactory *scf)
{
	xpe->stdout_handle = new XerrHandle(xpe->zdt, "stdout");
	xpe->stderr_handle = new XerrHandle(xpe->zdt, "stderr");

	int fd;

	fd = xpe->handle_table->allocate_filenum();
	lite_assert(fd==0);	// stdin.

	fd = xpe->handle_table->allocate_filenum();
	lite_assert(fd==1);
	xpe->handle_table->assign_filenum(1, new XVFSHandleWrapper(xpe->stdout_handle));

	fd = xpe->handle_table->allocate_filenum();
	lite_assert(fd==2);
	xpe->handle_table->assign_filenum(2, new XVFSHandleWrapper(xpe->stderr_handle));

	if (xpe->_initted)
	{
		return;
	}

	// This needs to come before creating ZLCVFS
	// (so the fast fetch knows the vendor name to use for keyval lookups)
	xpe->vendor_name_string = scf_lookup_env(scf, "VENDOR_ID");
#if 0
	XfsErr err = XFS_DIDNT_SET_ERROR;
	xpe_mount(xpe, &err, "", &xintFileSystem, sizeof(xintFileSystem));
#endif

	// mounts are overlaid as last-mounted-first-priority,
	// so mount the satisfies-everything filesystem first:
//	xzftpfs_init(xpe, "/");
//	zlcfs_init(xpe, "/");
	const char *zarfile_path = scf_lookup_env(scf, "ZARFILE_PATH");
	xpe->zlcvfs_wrapper = new ZLCVFS_Wrapper(xpe, "/", zarfile_path, xpe->trace_flag);

	xax_create_ramfs(xpe, "/tmp");
	xax_create_ramfs(xpe, "/home/jonh/.gnome2");
	xax_create_ramfs(xpe, "/home/jonh/.gnome2_private");
	xax_create_ramfs(xpe, "/home/jonh/.gimp-2.4");
		// TODO mkdir("/home/jonh", 0700);
		// ...conflicts with the location of my lib_links. :v)
	xax_create_ramfs(xpe, "/home/webguest");

// TODO jonh temporarily neuters keyvalfs to see if the rest of the web
// browser is working okay.
//	xax_create_keyvalfs(xpe, "/home/webguest/.config/midori/cookies.txt", true); // true = encrypted

	xax_create_ramfs(xpe, "/var/tmp");

	xax_create_ramfs(xpe, "/zoog/env");
	{
		XfsErr err;
		XVFSHandleWrapper *hw = xpe_open_hook_handle(xpe, &err, "/zoog/env/identity", O_RDWR|O_CREAT, NULL);
		int name_len = lite_strlen(xpe->vendor_name_string);
		hw->write(&err, xpe->vendor_name_string, name_len);
		hw->drop_ref();
	}

	xoverlayfs_init(xpe, ZOOG_ROOT "/pseudofiles/etc/passwd", "/etc/passwd");
	xoverlayfs_init(xpe, ZOOG_ROOT "/pseudofiles/etc/hosts", "/etc/hosts");
	xoverlayfs_init(xpe, ZOOG_ROOT "/pseudofiles/etc/cups/client.conf", "/etc/cups/client.conf");
	xoverlayfs_init(xpe, ZOOG_ROOT "/toolchains/linux_elf/lib_links", "/home/webguest/.gtk-2.0/printbackends");
	xoverlayfs_init(xpe, ZOOG_ROOT "/pseudofiles/home/webguest/.config/inkscape/keys/default.xml", "/home/webguest/.config/inkscape/keys/default.xml");
	xoverlayfs_init(xpe, ZOOG_ROOT "/pseudofiles/home/webguest/files", "/home/webguest/files");

	// Overlay the icon-theme.cache with a smaller one.
	xoverlayfs_init(xpe, ZOOG_ROOT "/toolchains/linux_elf/apps/midori/icon-prune/build/overlay/usr/share/icons/gnome", "/usr/share/icons/gnome");
	// neuter DNS until we sort out network delays
	xoverlayfs_init(xpe, ZOOG_ROOT "/pseudofiles/etc/resolv.conf", "/etc/resolv.conf");

	for (int i=0; ; i++)
	{
		char overlay_key_name[80];
		cheesy_snprintf(overlay_key_name, sizeof(overlay_key_name),
			"ZOOG_OVERLAY_%d", i);
		const char* overlay_spec = scf_lookup_env(scf, overlay_key_name);
		if (overlay_spec==NULL)
		{
			break;
		}
		char* split = lite_index(overlay_spec, '=');
		lite_assert(split!=NULL);
		int len = split - overlay_spec;
		char* from_path = mf_strndup(xpe->mf, overlay_spec, len);
		char* to_path = mf_strdup(xpe->mf, split+1);
		// NB we're giving ownership of strings to xoverlayfs
		xoverlayfs_init(xpe, to_path, from_path);
	}
	
	// hide a library that's exploding epiphany
	xax_create_ramfs(xpe, "/usr/lib/gstreamer-0.10/libgstfrei0r.so");

	// hide /dev/urandom from cheaty (no-manifest) zftp filesys
	xmask_init(xpe);

	xprocfakeryfs_init(xpe);

	xrandomfs_init(xpe, "/dev/random");
	xrandomfs_init(xpe, "/dev/urandom");

//#include <xax_leaky_sockets.h>
	// implement leaky /_socket path
	//xax_leaky_sockets_init();

	xax_unixdomainsockets_init(xpe);

	xpe->_initted = 1;
}

// libc points at ld.so's copy of the state.
//void synchronize_xax_interpose(void *v_xfsstate)
//{
//	xpe = (XfsState *) v_xfsstate;
//	lite_assert(xpe->_initted);
//	// copy configuration bits from ld.so, too.
//	_xax_config = xpe->config;
//}

//////////////////////////////////////////////////////////////////////////////

void xpe_mount(XaxPosixEmulation *xpe, const char *prefix, XaxVFSIfc *xfs)
{
	xpe->xvfs_namespace->install(new XVFSMount(prefix, xfs));
}

void _xpe_path_init(XaxPosixEmulation *xpe, XfsPath *path, const char *file)
{
	path->pathstr = (char*) cheesy_malloc(&xpe->zmf.cheesy_arena, lite_strlen(file)+2);
	path->pathstr[0] = '\0';
	// Current working directory is ALWAYS /, for now...
	if (file[0]!='/')
	{
		lite_strcpy(path->pathstr, "/");
	}
	lite_strcat(path->pathstr, file);
	// TODO better normalization, eliminating ../*'s and //'s and such.
}

void _xpe_path_free(XfsPath *path)
{
	cheesy_free(path->pathstr);
	path->pathstr = NULL;
	path->suffix = NULL;
}

XVFSMount *_xpe_map_path_to_mount(XaxPosixEmulation *xpe, const char *file, XfsPath *out_path)
{
	if (file[0]=='\0')
	{
		// We came across a program that does stat("")
		// (probably unintentionally), and whose correct behavior
		// seems to depend on the result being ENOENT!
		return NULL;
	}

	_xpe_path_init(xpe, out_path, file);
	XVFSMount *mount = xpe->xvfs_namespace->lookup(out_path);
	if (mount==NULL)
	{
		_xpe_path_free(out_path);
	}
	return mount;
}

XVFSHandleWrapper *xpe_open_hook_handle(XaxPosixEmulation *xpe, XfsErr *err, const char *path, int oflag, XVOpenHooks *xoh)
{
	XfsPath xpath;
	XVFSMount *mount = _xpe_map_path_to_mount(xpe, path, &xpath);

	if (mount==NULL)
	{
		*err = (XfsErr) ENOENT;
		return NULL;
	}

	*err = XFS_DIDNT_SET_ERROR;

	XVFSHandleWrapper *wrapper = NULL;
	XaxVFSHandleIfc *hdl = mount->xfs->open(err, &xpath, oflag, xoh);
	if ((*err)!=XFS_NO_ERROR)
	{
		lite_assert(hdl == NULL);
	}
	else
	{
		wrapper = new XVFSHandleWrapper(hdl);
	}

	_xpe_path_free(&xpath);
	return wrapper;
}

uint32_t xpe_open_hook_fd(XaxPosixEmulation *xpe, const char *path, int oflag, XVOpenHooks *xoh)
{
	int result = -1;

	// TODO: concurrency. Yeah.

	int filenum = xpe->handle_table->allocate_filenum();
	if (filenum == -1)
	{
		result = -EMFILE;
		goto exit;
	}

	XfsErr err;
	err = XFS_DIDNT_SET_ERROR;
	XVFSHandleWrapper *wrapper;
	wrapper = xpe_open_hook_handle(xpe, &err, path, oflag, xoh);

	if (err!=XFS_NO_ERROR)
	{
		lite_assert(wrapper==NULL);
		result = -err;
		xpe->handle_table->free_filenum(filenum);
		goto exit;
	}
	xpe->handle_table->assign_filenum(filenum, wrapper);

	if (xpe->mmap_flag)
	{
		char buf[512], hexbuf[16];
		lite_strcpy(buf, "F ");
		lite_strcat(buf, hexstr(hexbuf, filenum));
		lite_strcat(buf, " ");
		lite_strcat(buf, path);
		lite_strcat(buf, "\n");
		debug_logfile_append(xpe->zdt, "mmap", buf);
	}

	result = filenum;

exit:
	return result;
}

class ModeOpenHook : public XVOpenHooks
{
public:
	ModeOpenHook(mode_t mode) : mode(mode) {}
	virtual XRNode *create_node() { return NULL; }
	virtual mode_t get_mode() { return mode; }
	virtual void *cheesy_rtti() { static int rtti; return &rtti; }
private:
	mode_t mode;
};

uint32_t xi_open(XaxPosixEmulation *xpe, const char *path, int oflag, mode_t mode)
{
	ModeOpenHook xoh(mode);
	return xpe_open_hook_fd(xpe, path, oflag, &xoh);
}

int xi_link(XaxPosixEmulation *xpe, const char *oldpath, const char *newpath)
{
	int result;
	XfsPath old_xpath;
	XVFSMount *old_mount = _xpe_map_path_to_mount(xpe, oldpath, &old_xpath);
	XfsPath new_xpath;
	XVFSMount *new_mount = _xpe_map_path_to_mount(xpe, newpath, &new_xpath);
	if (old_mount!=new_mount)
	{
		result = -EXDEV;	// Invalid cross-device link
		goto done;
	}

	XfsErr err;
	err = XFS_DIDNT_SET_ERROR;
	new_mount->xfs->hard_link(&err, &old_xpath, &new_xpath);
	lite_assert(err != XFS_DIDNT_SET_ERROR);
	result = -err;

done:
	_xpe_path_free(&old_xpath);
	_xpe_path_free(&new_xpath);
	return result;
}

int xi_rename(XaxPosixEmulation *xpe, const char *oldpath, const char *newpath)
{
	// epiphany wants this to rename a /tmp file; pass through to xramfs?
	return 0;
}

int xi_readlink(XaxPosixEmulation *xpe, const char *path, char *buf, size_t bufsiz)
{
	return -EINVAL;
}

int xi_close(XaxPosixEmulation *xpe, int filenum)
{
	int result;

	XVFSHandleWrapper *wrapper = xpe->handle_table->lookup(filenum);
	if (wrapper==NULL)
	{
		result = -EBADF;
	}
	else
	{
		// Ugh, my XFS_SILENT_FAIL expedient, to protect stderr, doesn't work
		// now that we have dup, since we'd like to allow the close(2)
		// if 2 has been duped. So we hard-code this case for now.
		bool sneaky_protect_stdio = (filenum >= 0 && filenum <=2);

		if (sneaky_protect_stdio)
		{
			result = 0;
		}
		else
		{
			xpe->handle_table->free_filenum(filenum);
			wrapper->drop_ref();
			result = 0;
		}
	}

	return result;
}

ssize_t xi_read(XaxPosixEmulation *xpe, int filenum, void *buf, size_t count)
{	ROUTE_NONBLOCKING(read, zas_read, buf, count); }

ssize_t xi_write(XaxPosixEmulation *xpe, int filenum, const void *buf, size_t count)
{	ROUTE_NONBLOCKING(write, zas_write, buf, count); }

ssize_t xi_writev(XaxPosixEmulation *xpe, int filenum, const struct iovec *iov, int iovcnt)
{
	// TODO fails atomicity (see man 2 writev); probably gets error conditions
	// wrong, too.
	int i;
	ssize_t rc = 0;
	for (i=0; i<iovcnt; i++)
	{
		if (iov[i].iov_len==0)
		{
			// xax_unixdomainsockets doesn't take too kindly to 0-byte writes.
			continue;
		}

		ssize_t sub_rc = xi_write(xpe, filenum, iov[i].iov_base, iov[i].iov_len);
		if (sub_rc<0)
		{
			return sub_rc;
		}
		rc += sub_rc;
	}
	return rc;
}

static void stat_via_fstat(XaxPosixEmulation *xpe, XfsErr *err, XfsPath *xpath, XaxVFSIfc *vfs, struct stat64 *buf)
{
	XfsErr terr = XFS_DIDNT_SET_ERROR;
	ModeOpenHook xoh(0);
	XaxVFSHandleIfc *hdl = vfs->open(&terr, xpath, O_RDONLY, &xoh);
	if (terr != XFS_NO_ERROR)
	{
		*err = terr;
	} 
	else
	{
		// don't need a wrapper; we won't be hanging around long enough
		// to look at it.
		hdl->xvfs_fstat64(err, buf);
		delete(hdl);
	}
}

int xi_fstat64(XaxPosixEmulation *xpe, int filenum, struct stat64 *buf)
{	ROUTE_VOID_FCN(xvfs_fstat64, buf); }

void xpe_stat64(XaxPosixEmulation *xpe, XfsErr *err, const char *path, struct stat64 *buf)
{
	XfsPath xpath;

	XVFSMount *mount = _xpe_map_path_to_mount(xpe, path, &xpath);

	if (mount==NULL)
	{
#if 0
		if (_xax_config->strace)
		{
			XAX_CHEESY_DEBUG(":xi_stat64: no mount");
			XAX_CHEESY_DEBUG(path);
			XAX_CHEESY_DEBUG("\n");
		}
#endif
		*err = (XfsErr) -ENOENT;
	}
	else
	{
		*err = XFS_DIDNT_SET_ERROR;

		mount->xfs->stat64(err, &xpath, buf);
		if (*err==XFS_ERR_USE_FSTAT64)
		{
			stat_via_fstat(xpe, err, &xpath, mount->xfs, buf);
		}
		_xpe_path_free(&xpath);
	}
}

int xi_stat64(XaxPosixEmulation *xpe, const char *path, struct stat64 *buf)
{
	XfsErr err;
	xpe_stat64(xpe, &err, path, buf);
	if (err!=XFS_NO_ERROR)
	{
		return -err;
	}
	else
	{
		return 0;
	}
}

int xi_lstat64(XaxPosixEmulation *xpe, const char *path, struct stat64 *buf)
{
	// sorry, I've never heard of a symbolic link before. drool drool.
	return xi_stat64(xpe, path, buf);
}

void *xi_brk(XaxPosixEmulation *xpe, void *reqaddr)
{
	void *rc;

	if (reqaddr==0)
	{
		rc = xpe->xbrk.new_brk();
	}
	else
	{
		rc = xpe->xbrk.more_brk(reqaddr);
	}

	return rc;
}

void *xi_mmap64(XaxPosixEmulation *xpe, void *addr, size_t len, int prot, int flags, int fd, uint32_t offset_in_pages)
{
	// strategy: allocate the memory either way using the only primitive
	// we've got; then, fill with zeros or with file contents + zeros.

	if ((flags & MAP_FIXED)==0)
	{
		// We allow nonzero addr as a heuristic to capture the case
		// where ld.so is using mmap to blast blank pages over a
		// file's .bss segment; this fcn turns that req into a memset(0)
		// operation. (That is, it explodes if there's no memory already
		// there from mapping the file.)
		// BUT, malloc will sometimes offer a helpful addr suggestion
		// in an attempt to reduce heap fragmentation.
		// We can distinguish this latter case by the absence of a
		// MAP_FIXED flag, which hint we use to null out the addr hint.
		addr = NULL;
	}

	void *region = NULL;
	uint32_t errorcode;
	const char *dbg_disposition = NULL;
#if 0	// dead code
	if (addr==NULL)
	{
		region = (xpe->zmf.guest_mmap_ifc->mmap_f)(xpe->zmf.guest_mmap_ifc, len);
	}
	else
	{
		// not allocating; assuming ld.so is just mapping-over some
		// file contents (or nulls)
		region = addr;
	}
#endif

#if DEBUG_BROKEN_MMAP
	{
		char buf[250];
		char hexbuf[16];
		strcpy(buf, "xi_mmap(A ");
		strcat(buf, hexlong(hexbuf, (uint32_t) addr));
		strcat(buf, ", L ");
		strcat(buf, hexlong(hexbuf, len));
		strcat(buf, ", fd ");
		strcat(buf, hexlong(hexbuf, fd));
		strcat(buf, ") = ");
		strcat(buf, hexlong(hexbuf, (uint32_t) region));
		strcat(buf, "\n");
		debug_logfile_append("mmap", buf, lite_strlen(buf));
	}
#endif //DEBUG_BROKEN_MMAP

	if (fd>=0)
	{
		XVFSHandleWrapper *wrapper = xpe->handle_table->lookup(fd);
		if (wrapper==NULL)
		{
			errorcode = EBADF;
			goto explode;
		}
		XfsErr err = XFS_DIDNT_SET_ERROR;

		uint32_t offset_in_bytes = offset_in_pages << 12;
		lite_assert(offset_in_bytes>>12 == offset_in_pages);	// shifted something off the end; file > 4GB!

		XaxVFSHandleIfc *hdl = wrapper->get_underlying_handle();
		if (hdl->is_stream())
		{
			errorcode = EINVAL;	// tried to mmap a stream-type fd
			goto explode;
		}
		// file_avail_len may be shorter than request, if we're going
		// to memcpy in bytes from the file and zero the rest.
		uint32_t file_avail_len = min(len, hdl->get_file_len()-offset_in_bytes);
		// fast_mmap_len may be longer than request, as it gets rounded
		// up to a page. (Apallingly, the ld-linux loader will ask for
		// a non-round length, then *assume* it gets the whole page.
		// Why not just specify that in the mmap() call!? Aieee.)
		// TODO I wonder if we should go ahead and enforce page rounding
		// even on fd==-1 requests, in case some stupid library is as
		// insane as the loader.
#define PAGE_SIZE	(1<<12)
		uint32_t mmap_len = ((len-1)&(~(PAGE_SIZE-1)))+PAGE_SIZE;

		if (addr==NULL)
		{
			region = hdl->fast_mmap(mmap_len, offset_in_bytes);
			dbg_disposition = "fast";
#if 0
			// debugging code, used to see when fast_mmap was returning
			// regions that didn't match reference versions.
			if (region!=NULL)
			{
				char buf[256];
				cheesy_snprintf(buf, sizeof(buf),
					"H hash of fast mmap at %08x length %08x == %08x\n",
					region, actual, hash_buf(region, actual));
				debug_logfile_append(xpe->zdt, "mmap", buf);
			}
#endif
		}

		if (region!=NULL)
		{
			// fast_mmap worked
			err = XFS_NO_ERROR;
		}
		else
		{
			// nope, gotta (maybe allocate &) memcpy-read
			if (addr==NULL)
			{
				region = (xpe->zmf.guest_mmap_ifc->mmap_f)(
					xpe->zmf.guest_mmap_ifc, mmap_len);
			}
			else
			{
				region = addr;
			}
			hdl->read(&err, region, file_avail_len, offset_in_bytes);
			if (dbg_disposition==NULL)
			{
				dbg_disposition = "memcpy_read";
			}

			if (xpe->mmap_flag)
			{
				// hash the mapped region -- useful for debugging zarfile
				// construction & client code.
				char buf[1024];
				uint32_t hashval = false ? hash_buf(region, file_avail_len) : -1;
				cheesy_snprintf(buf, sizeof(buf),
					"H hash of memcpy mmap at %08x length %08x == %08x\n",
					region, file_avail_len, hashval);
				debug_logfile_append(xpe->zdt, "mmap", buf);
			}
		}

		// we mark the mmap as "fast" in the trace if it *could* have
		// been served by a fast request (not whether it actually was),
		// since the trace is captured before a (fast-enabled) zarfile
		// is available.
		// We trace the mmap after the read so that the absorption logic
		// won't also insert precious regions for the whole .text segment.
		// (In a real fast run, the read would not actually occur, so
		// zarfile space would be wasted.)
		hdl->trace_mmap(mmap_len, offset_in_bytes, addr==NULL);

		if (err!=XFS_NO_ERROR)
		{
			errorcode = err;
			goto explode;
		}

#ifdef COUNT_TOUCHED_PAGES 
		debug_trace_touched_pages_f	protect_pages = (debug_trace_touched_pages_f)((xpe->zdt->zoog_lookup_extension)(DEBUG_TRACE_TOUCHED_PAGES));
		protect_pages((uint8_t*)region, actual);
		//xpe->ptf->protect(region, actual);
#endif // COUNT_TOUCHED_PAGES 

		if (dbg_disposition==NULL)
		{
			dbg_disposition="unset";
		}

		if (xpe->mmap_flag)
		{
			char buf[512], hexbuf[16];
			lite_strcpy(buf, "M ");
			lite_strcat(buf, hexstr(hexbuf, fd));
			lite_strcat(buf, " ");
			lite_strcat(buf, hexstr(hexbuf, (uint32_t) region));
			lite_strcat(buf, " ");
			lite_strcat(buf, hexstr(hexbuf, ((uint32_t) region)+len));
			lite_strcat(buf, " ");
			lite_strcat(buf, hexstr(hexbuf, (uint32_t) offset_in_bytes));
			lite_strcat(buf, " ");
			lite_strcat(buf, dbg_disposition);
			lite_strcat(buf, "\n");
			debug_logfile_append(xpe->zdt, "mmap", buf);
		}
	}
	else if (addr==NULL)
	{
		region = (xpe->zmf.guest_mmap_ifc->mmap_f)(
			xpe->zmf.guest_mmap_ifc, len);
	}
	else if (addr!=NULL)
	{
		// In the case where addr!=NULL, that's dl-load asking
		// us to zero .bss for some already-allocated memory, that we did
		// NOT pass down to xax_allocate_memory, and memset is required.
		region = addr;
		lite_memset(region, 0, len);
	}
	else
	{
#if 0	
		// verify that PAL is handing us zeroed memory.
		// (monitor should make this true, for security reasons!)
		// disabled: needlessly slow; make it part of paltest!
		// or at most a random check
		uint32_t *p;
		for (p = (uint32_t*) region;
			p<((uint32_t*) (((uint32_t)region)+len));
			p++)
		{
			lite_assert((*p) == 0);
		}
#endif
	}

#if MALLOC_TRACE_MMAP 
	xi_trace_alloc(xpe->mtf, tl_mmap, region, len);
#endif // MALLOC_TRACE_MMAP
	lite_assert(addr==NULL || addr==region);
	lite_assert((((uint32_t)region) & 0xfff)==0);
	return region;

explode:
	lite_assert(0);	// unimpl -- undo allocation.
	return (void*) (-errorcode);
}

int xi_munmap(XaxPosixEmulation *xpe, void *addr, size_t len)
{
	(xpe->zmf.guest_mmap_ifc->munmap_f)(xpe->zmf.guest_mmap_ifc, addr, len);
#if MALLOC_TRACE_MMAP 
	xi_trace_free(xpe->mtf, tl_mmap, addr);
#endif // MALLOC_TRACE_MMAP
	return 0;
}

#define NULLPROOF_FD_ISSET(n, fds)	((fds)!=NULL && (FD_ISSET(n, fds)))
#define NULLPROOF_FD_ZERO(fds)	{if ((fds)!=NULL) { FD_ZERO(fds); }}
#define NULLPROOF_FD_SET(n, fds)	{if ((fds)!=NULL) { FD_SET(n, fds); }}

typedef struct {
	int fdi;
	union {
		ZASChannel select_field;
		int poll_mask;
	};
	ZASChannel dbg_channel;
} SelectMap;

typedef struct {
	ZAS **producers;
	SelectMap *map;
	int producer_count;
	int max_producers;
} Selectinator;

void xi_init_selectinator(CheesyMallocArena *cma, Selectinator *sel, int max_producers)
{
	sel->max_producers = max_producers;
	sel->producers = (ZAS**) cheesy_malloc(cma, sizeof(ZAS*)*max_producers);
	sel->map = (SelectMap*) cheesy_malloc(cma, sizeof(SelectMap)*max_producers);
	sel->producer_count = 0;
}

void xi_free_selectinator(XaxPosixEmulation *xpe, Selectinator *sel)
{
	int i;
	for (i=0; i<sel->producer_count; i++)
	{
		zas_virtual_destructor_t dtor =
			sel->producers[i]->method_table->virtual_destructor;
		if (dtor!=NULL)
		{
			dtor(xpe->zdt, sel->producers[i]);
		}
	}

	cheesy_free(sel->producers);
	cheesy_free(sel->map);
}

int xi_select_check_channel(XaxPosixEmulation *xpe, Selectinator *sel, ZASChannel channel, int fd, fd_set *channelfds)
{
	if (NULLPROOF_FD_ISSET(fd, channelfds))
	{
		XVFSHandleWrapper *wrapper = xpe->handle_table->lookup(fd);
		if (wrapper==NULL)
		{
			return -EBADF;
		}
		ZAS *zas = wrapper->get_zas(channel);
		sel->producers[sel->producer_count] = zas;
		sel->map[sel->producer_count].fdi = fd;
		sel->map[sel->producer_count].select_field = channel;
		sel->map[sel->producer_count].dbg_channel = channel;
		sel->producer_count+=1;
	}
	return 0;
}

typedef struct {
	ZAlwaysReady zeroTimeout;
	bool usingZero;
	ZTimer *ztimer;
	bool usingTimer;
} TimeoutManager;

void init_timeout_manager_ts(XaxPosixEmulation *xpe, TimeoutManager *tm, const struct timespec *ts)
{
	tm->usingZero = false;
	tm->usingTimer = false;

	if (ts==NULL)
	{
		// infinite timeout
		return;
	}
	if (ts->tv_sec == 0 && ts->tv_nsec == 0)
	{
		// zero timeout
		tm->usingZero = true;
		zar_init(&tm->zeroTimeout);
		return;
	}

	// real timeout
	ZClock *zclock = xpe->xax_skinny_network->get_clock();

	uint64_t absolute_time = zclock->get_time_zoog();
#if 0	// dead code
	// TODO: hey, did anyone notice we're ignoring all timeouts
	// and replacing them with exactly 1 second? That's pretty stupid.
	absolute_time += 1000000000LL;
#endif
	absolute_time += ((uint64_t)ts->tv_nsec) + ts->tv_sec*1000000000LL;

	tm->usingTimer = true;
	tm->ztimer = new ZTimer(zclock, absolute_time);
}

void init_timeout_manager_tv(XaxPosixEmulation *xpe, TimeoutManager *tm, struct timeval *tv)
{
	if (tv==NULL)
	{
		init_timeout_manager_ts(xpe, tm, NULL);
	}
	else
	{
		struct timespec ts;
		TIMEVAL_TO_TIMESPEC(tv, &ts);
		init_timeout_manager_ts(xpe, tm, &ts);
	}
}

void init_timeout_manager_ms(XaxPosixEmulation *xpe, TimeoutManager *tm, int timeout_ms)
{
	if (timeout_ms<0)
	{
		init_timeout_manager_ts(xpe, tm, NULL);
	}
	else if (timeout_ms==0)
	{
		struct timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = 0;
		init_timeout_manager_ts(xpe, tm, &ts);
	}
	else
	{
		struct timespec ts;
		ts.tv_sec = timeout_ms / 1000;
		ts.tv_nsec = (timeout_ms - ts.tv_sec * 1000) * 1000000;
		init_timeout_manager_ts(xpe, tm, &ts);
	}
}

ZAS *timeout_manager_get_zas(TimeoutManager *tm)
{
	// xi_futex's timeout case uses TimeoutManagers to set up timer,
	// but then skips the Selectinator interface to reach down to the
	// ZAS level.
	ZAS *zas = NULL;
	if (tm->usingZero)
	{
		zas = &tm->zeroTimeout.zas;
	}
	else if (tm->usingTimer)
	{
		zas = tm->ztimer->get_zas();
	}
	else
	{
		lite_assert(0);
	}
	return zas;
}

void timeout_manager_apply(TimeoutManager *tm, Selectinator *sel)
{
	ZAS *zas = NULL;
	if (tm->usingZero)
	{
		zas = &tm->zeroTimeout.zas;
	}
	else if (tm->usingTimer)
	{
		zas = tm->ztimer->get_zas();
	}
	else
	{
		return;
	}

	sel->producers[sel->producer_count] = zas;
	sel->map[sel->producer_count].fdi = -1;
	sel->map[sel->producer_count].select_field = zas_except;
	sel->map[sel->producer_count].dbg_channel = zas_except;
	sel->producer_count+=1;
}

void free_timeout_manager(TimeoutManager *tm)
{
	if (tm->usingZero)
	{
		zar_free(&tm->zeroTimeout);
	}
	if (tm->usingTimer)
	{
		delete tm->ztimer;
	}
}

int xi_select(XaxPosixEmulation *xpe, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	int ret = -1;
	Selectinator sel;
	xi_init_selectinator(&xpe->zmf.cheesy_arena, &sel, 3*nfds+1);
	TimeoutManager timeout_manager;
	bool timeout_manager_allocated = false;

	int rc;
	int fdi;
	for (fdi=0; fdi<nfds; fdi++)
	{
		rc = xi_select_check_channel(xpe, &sel, zas_read, fdi, readfds);
		if (rc!=0) { goto fail; }
		rc = xi_select_check_channel(xpe, &sel, zas_write, fdi, writefds);
		if (rc!=0) { goto fail; }
		rc = xi_select_check_channel(xpe, &sel, zas_except, fdi, exceptfds);
		if (rc!=0) { goto fail; }
	}

	init_timeout_manager_tv(xpe, &timeout_manager, timeout);
	timeout_manager_allocated = true;
	timeout_manager_apply(&timeout_manager, &sel);
	lite_assert(sel.producer_count < sel.max_producers);

	int ready_idx;
	ready_idx = zas_wait_any(xpe->zdt, sel.producers, sel.producer_count);

	ret = 0;
	int fd;
	fd = sel.map[ready_idx].fdi;
	NULLPROOF_FD_ZERO(readfds);
	NULLPROOF_FD_ZERO(writefds);
	NULLPROOF_FD_ZERO(exceptfds);
	if (fd>=0)
	{
		switch (sel.map[ready_idx].select_field)
		{
		case zas_read:
			NULLPROOF_FD_SET(fd, readfds);
			ret+=1;
			break;
		case zas_write:
			NULLPROOF_FD_SET(fd, writefds);
			ret+=1;
			break;
		case zas_except:
			NULLPROOF_FD_SET(fd, exceptfds);
			ret+=1;
			break;
		}

#if DEBUG_SELECT
		{
			char buf[100];
			strcpy(buf, "xi_select: releases ");
			const char *display = "RWxxE";
			char v[3];
			v[0] = display[sel.map[ready_idx].dbg_channel & 0x0f];
			v[1] = ' ';
			v[1] = '\0';
			strcat(buf, v);
			char hexbuf[15];
			hexlong(hexbuf, fd);
			strcat(buf, hexbuf);
			strcat(buf, "!\n");
			debug_write_sync(buf);
		}
#endif // DEBUG_SELECT
	}
	else
	{
#if DEBUG_SELECT
		debug_write_sync("select timeout\n");
#endif // DEBUG_SELECT
	}

fail:
	xi_free_selectinator(xpe, &sel);
	if (timeout_manager_allocated)
	{
		free_timeout_manager(&timeout_manager);
	}
	return ret;
}

int xi_newselect(XaxPosixEmulation *xpe)
{
	lite_assert(false);
	return -1;
}

int xi_poll_check_channel(XaxPosixEmulation *xpe, Selectinator *sel, int poll_mask, ZASChannel channel, int fdi, struct pollfd *fds)
{
	if ((fds[fdi].events & poll_mask)!=0)
	{
		int filenum = fds[fdi].fd;
		XVFSHandleWrapper *wrapper = xpe->handle_table->lookup(filenum);
		if (wrapper==NULL)
		{
			return -EBADF;
		}
		ZAS *zas = wrapper->get_zas(channel);
		sel->producers[sel->producer_count] = zas;
		sel->map[sel->producer_count].fdi = fdi;
		sel->map[sel->producer_count].poll_mask = poll_mask;
		sel->map[sel->producer_count].dbg_channel = channel;
		sel->producer_count+=1;
	}
	return 0;
}

int xi_poll(XaxPosixEmulation *xpe, struct pollfd *fds, nfds_t nfds, int timeout_ms)
{
	Selectinator sel;
	xi_init_selectinator(&xpe->zmf.cheesy_arena, &sel, 3*nfds+1);
	int ret = 0;

	nfds_t fdi;
	for (fdi=0; fdi<nfds; fdi++)
	{
		ret = xi_poll_check_channel(xpe, &sel, POLLIN, zas_read, fdi, fds);
		if (ret!=0) { goto fail; }
		ret = xi_poll_check_channel(xpe, &sel, POLLOUT, zas_write, fdi, fds);
		if (ret!=0) { goto fail; }
		ret = xi_poll_check_channel(xpe, &sel, POLLERR, zas_except, fdi, fds);
		if (ret!=0) { goto fail; }

		// init out parameters to empty; we'll or in results when
		// poll is complete.
		fds[fdi].revents = 0;
	}

	TimeoutManager timeout_manager;
	init_timeout_manager_ms(xpe, &timeout_manager, timeout_ms);
	timeout_manager_apply(&timeout_manager, &sel);
	lite_assert(sel.producer_count < sel.max_producers);

	int ready_idx;
	ready_idx = zas_wait_any(xpe->zdt, sel.producers, sel.producer_count);

	int filenum;
	filenum = sel.map[ready_idx].fdi;
	if (filenum >= 0)
	{
		fds[filenum].revents |= sel.map[ready_idx].poll_mask;
#if DEBUG_SELECT
		debug_write_sync("poll releases fd\n");
#endif // DEBUG_SELECT
		ret = 1;
	}
	else
	{
#if DEBUG_SELECT
		debug_write_sync("poll timeout\n");
		ret = 0;
#endif // DEBUG_SELECT
	}

fail:
	xi_free_selectinator(xpe, &sel);
	free_timeout_manager(&timeout_manager);
	return ret;
}

int xi_nanosleep(XaxPosixEmulation *xpe, const struct timespec *req, struct timespec *rem)
{
	TimeoutManager timeout_manager;
	init_timeout_manager_ts(xpe, &timeout_manager, req);
	ZAS *zas = timeout_manager_get_zas(&timeout_manager);
	int ready_idx = zas_wait_any(xpe->zdt, &zas, 1);
	lite_assert(ready_idx==0);
	free_timeout_manager(&timeout_manager);
	// Right now, we have no such thing as being "interrupted"
	return 0;
}

int xi_mkdir(XaxPosixEmulation *xpe, const char *path, mode_t mode)
{
	int result = -1;
	bool path_allocated = false;
	XfsPath xprefix;

	// be sure we're not trying to create an existing mountpoint:
	// that would trick us into trying to do a mkdir on the parent
	// mount, which would disappear (or crash if the parent isn't mkdirable)
	XfsPath xpath_ck;
	_xpe_map_path_to_mount(xpe, path, &xpath_ck);
	bool is_mountpoint = (xpath_ck.suffix[0]=='\0');
	_xpe_path_free(&xpath_ck);
	if (is_mountpoint)
	{
		// hey, this is a mountpoint.
		result = -EEXIST;
		goto exit;
	}

	{
		DecomposePath dpath(xpe->mf, path);
		if (dpath.err!=XFS_NO_ERROR)
		{
			result = -dpath.err;
			goto exit;
		}

		XVFSMount *mount;
		mount = _xpe_map_path_to_mount(xpe, dpath.prefix, &xprefix);

		if (mount==NULL)
		{
			result = -ENOENT;
			goto exit;
		}
		path_allocated = true;

		XfsErr err;
		err = XFS_DIDNT_SET_ERROR;
		mount->xfs->mkdir(&err, &xprefix, dpath.tail, mode);
		if (err!=XFS_NO_ERROR)
		{
			result = -err;
			goto exit;
		}
		else
		{
			result = 0;
			goto exit;
		}
	}

exit:
	if (path_allocated)
	{
		_xpe_path_free(&xprefix);
	}
	return result;
}

int xi_dup(XaxPosixEmulation *xpe, int fd)
{
	// TODO not sure we need this anymore, now that we're properly
	// refcounting.
	if (fd==2)
	{
		// perror will just fall through happy if dup fails.
		return -ENOSYS;
	}

	XVFSHandleWrapper *wrapper = xpe->handle_table->lookup(fd);
	if (wrapper==NULL) { return -EBADF; }

	int dup_fd = xpe->handle_table->allocate_filenum();
	wrapper->add_ref();
	xpe->handle_table->assign_filenum(dup_fd, wrapper);
	
	return dup_fd;
}

int xi_dup2(XaxPosixEmulation *xpe, int fd, int fd2)
{
	xax_unimpl_assert();
	return -ENOSYS;
}

int xi_fcntl(XaxPosixEmulation *xpe, int filenum, int cmd, long arg)
{
	XVFSHandleWrapper *wrapper = xpe->handle_table->lookup(filenum);
	if (wrapper==NULL) { return -EBADF; }

	switch (cmd)
	{
		case F_GETFL:
			return wrapper->status_flags;
		case F_SETFL:
			if ((O_NONBLOCK & arg) != 0)
			{
				wrapper->status_flags |= O_NONBLOCK;
			}
			if (((O_APPEND|O_ASYNC|O_DIRECT|O_NOATIME) & arg) != 0)
			{
				lite_assert(0);	// unimplemented.
			}
			return 0;
		case F_GETFD:
		case F_SETFD:
			// FD_CLOEXEC ignored.
			return 0;
		case F_SETLK64:
			// fake sqlite3 into thinking we did a range lock.
			return 0;
		case F_SETLKW:
			// fake libbonobo into thinking we did a range lock.
			return 0;
		case F_DUPFD:
		case F_DUPFD_CLOEXEC:
		{
			int rc;
			rc = xi_dup(xpe, filenum);
			lite_assert(rc < 0 || rc>=arg);
				// see man fcntl /DUPFD -- a semantic constraint we're supposed
				// to enforce but we don't; so we'll explode if we ever accidentally
				// violate it.
			return rc;
		}
		case F_SETSIG:
			// lie to libqtcore4 about setting up signals
			return 0;
		case F_NOTIFY:
			// sorry, libqtcore4: we don't actually do notifications.
			// (or, we could claim we do, and just not.)
			return -EINVAL;
		default:
			lite_assert(0);	// unimplemented.
	}
	return -EINVAL;
}

int xi_getdents64(XaxPosixEmulation *xpe, unsigned int filenum, struct xi_kernel_dirent64 *dirp, unsigned int dirpsize)
{
	XVFSHandleWrapper *wrapper = xpe->handle_table->lookup(filenum);
	if (wrapper==NULL) { return -EBADF; }

	XfsErr err = XFS_DIDNT_SET_ERROR;
	int result;
	result = (wrapper->getdent64)(&err, dirp, dirpsize);
	lite_assert(err != XFS_DIDNT_SET_ERROR);
	if (err!=XFS_NO_ERROR)
	{
		return -err;
	}
	return result;
}

struct xi_kernel_dirent
  {
    int32_t d_ino;
    uint32_t d_off;
    uint16_t d_reclen;
    char d_name[256];
	uint8_t zero;
	uint8_t d_type;
  };

int xi_getdents(XaxPosixEmulation *xpe, unsigned int filenum, struct xi_kernel_dirent *kdirp, unsigned int dirpsize)
{
	// TODO limiting count to 1 to make allocation for translation easier.
	// (we probably shouldn't actually translate; should pass in a polymorphic
	// self-setting object.)
	struct xi_kernel_dirent64 dirp64;

	XVFSHandleWrapper *wrapper = xpe->handle_table->lookup(filenum);
	if (wrapper==NULL) { return -EBADF; }

	XfsErr err = XFS_DIDNT_SET_ERROR;
	int result;
	result = (wrapper->getdent64)(&err, &dirp64, sizeof(dirp64));
	lite_assert(err != XFS_DIDNT_SET_ERROR);
	if (err != XFS_NO_ERROR)
	{
		return -err;
	}

	if (result>0)
	{
		lite_assert(result==sizeof(dirp64));
		lite_assert(dirp64.d_ino < 0x7fffffff);
		kdirp->d_ino = dirp64.d_ino;
		lite_assert(dirp64.d_off < 0x7fffffff);
		kdirp->d_off = dirp64.d_off;
		kdirp->d_reclen = sizeof(*kdirp);
		lite_assert(dirp64.d_type <= 0xff);
		kdirp->zero = 0;
		kdirp->d_type = dirp64.d_type;
		lite_memcpy(kdirp->d_name, dirp64.d_name, sizeof(kdirp->d_name));
		// tell caller that, once he's read this one record, he's done.
		result = 268;
	}
	else
	{
		lite_assert(result==0);
	}
	return result;
}


// error in result, actual value in retval
uint32_t xi_llseek(XaxPosixEmulation *xpe, int filenum, int64_t offset, int64_t *retval, int whence)
{	ROUTE_VOID_FCN(llseek, offset, whence, retval); }

int xi_ftruncate(XaxPosixEmulation *xpe, int filenum, int64_t length)
{	ROUTE_VOID_FCN(ftruncate, length); }

uint32_t xi_lseek(XaxPosixEmulation *xpe, int fd, off_t offset, int whence)
{
	int64_t result;
	uint32_t rc = xi_llseek(xpe, fd, offset, &result, whence);
	if (rc!=0)
	{
		return rc;
	}
	if ((result > 0xfffff000) || result<0)
	{
		return -EOVERFLOW;
	}
	return (uint32_t) result;
}

void xnull_fstat64(XfsErr *err, struct stat64 *buf, size_t size)
{
	// hey, if it's open, it must be a file!
	buf->st_dev = 17;
	buf->st_ino = 17;	// strange. Every file has the same inode!
	buf->st_mode = S_IRWXU | S_IFREG;
	buf->st_nlink = 1;
	buf->st_uid = 0;
	buf->st_gid = 0;
	buf->st_rdev = 0;
	buf->st_size = size;
	buf->st_blksize = 512;
	buf->st_blocks = (size+buf->st_blksize-1) / buf->st_blksize;
	buf->st_atime = 1272062690;	// and was created at the same moment,
	buf->st_mtime = 1272062690;	// coincidentally contemporaneous with the
	buf->st_ctime = 1272062690;	// moment I wrote this function! wild!
	*err = XFS_NO_ERROR;
}

int xi_statfs64(XaxPosixEmulation *xpe, const char *path, struct statfs *buf)
{
	// poked at by libselinux's init sequence.
	// and libqt's (32-bit version)
	return -ENOSYS;
}

int xi_fstatfs64(XaxPosixEmulation *xpe, int fd, struct statfs *buf)
{
	// poked by libQt
	return -ENOSYS;
}

uid_t xi_getuid(XaxPosixEmulation *xpe)
{
	return xpe->fake_ids.uid;
}

gid_t xi_getgid(XaxPosixEmulation *xpe)
{
	return xpe->fake_ids.gid;
}

int xi_pipe2(XaxPosixEmulation *xpe, int pipefd[2], int flags)
{
	SimPipe *simpipe = new SimPipe(xpe->mf, xpe->zdt);

	int i;
	for (i=0; i<2; i++)
	{
		pipefd[i] = xpe->handle_table->allocate_filenum();
		if (pipefd[i]==-1)
		{
			goto fail;
		}
		XaxVFSHandleIfc *hdl = (i==0)
			? simpipe->readhdl
			: simpipe->writehdl;
		XVFSHandleWrapper *wrapper = new XVFSHandleWrapper(hdl);
		xpe->handle_table->assign_filenum(pipefd[i], wrapper);
		if ((flags & O_NONBLOCK) != 0)
		{
			wrapper->status_flags |= O_NONBLOCK;
		}
	}

	flags &= ~O_CLOEXEC;	// ignored flag
	flags &= ~O_NONBLOCK;	// handled flag
	lite_assert(flags==0);	// um, wasn't expecting any other flags.

	return 0;

fail:
	return -EMFILE;
}

// TODO file pointer maintenance is every filesystem's job; should factor?

int
xi_sigaction(XaxPosixEmulation *xpe, int sig, const struct sigaction *act, struct sigaction *oact)
{
	if (sig==0x18 || sig==0x1e)
	{
		// These are libgc's idea of SIG_SUSPEND and SIG_THR_RESTART.
		// Lie to sedate libgc:pthread_stop_world:GC_stop_init().
		return 0;
	}

	if (sig==SIGHUP || sig==SIGINT || sig==SIGQUIT ||
		sig==SIGABRT || sig==SIGTERM || sig==SIGBUS ||
		sig==SIGSEGV || sig==SIGFPE || sig==SIGCHLD ||
		sig==SIGPIPE)
	{
		// lying to gimp
		return 0;
	}

    if (sig==SIGALRM)
    {
        // lying to ntpdate
        return 0;
    }

	// Xvnc's os/connection.c calls os/utils.c,
	// which calls sigaction and assumes success.
	// So we need to supply it with a suitable lie.
	if (sig==SIGUSR1 && act!=NULL && act->sa_handler==SIG_IGN)
	{
		if (oact!=NULL)
		{
			// pretend we complied, even though we're returning -1
			// and an error.
			oact->sa_handler = 0;
		}
	}

	return -EINVAL;
}

int xi_sigprocmask(XaxPosixEmulation *xpe, int how, const sigset_t *set, sigset_t * oldset)
{
	return 0;
}

int xi_kill(XaxPosixEmulation *xpe, int pid, int sig)
{
	xax_unimpl_assert();
	return 0;
}

void xpe_install_tls(XaxPosixEmulation *xpe, uint32_t new_gs)
{
	uint32_t old_gs = get_gs_base();
	XaxStackTableEntry xte;
	xst_remove(&xpe->stack_table, old_gs, &xte);

	(xpe->zdt->zoog_x86_set_segments)(0, new_gs);

	if (xpe->strace_flag)
	{
		char msg[256];
		cheesy_snprintf(msg, sizeof(msg),
		"  :-- xpe_install_tls(old_base=%08x, new_base=%08x, selector %d)\n",
			old_gs,
			new_gs,
			_get_gs_descriptor_idx());
		debug_logfile_append(xpe->zdt, "strace", msg);
	}

	lite_assert(xte.stack_base != NULL);	// kind of expecting useful data in here

	xte.gs_thread_handle = new_gs;	// update key
	// re-record it.
	xst_insert(&xpe->stack_table, &xte);

	// NB one might be concerned about a race vulnerability
	// between remove and insert.
	// The other things that can happen in there are xprocfakeyfs stack
	// base lookups and xi_exit, which both come from this same thread.
}

#if 0
struct x_user_desc {
	uint32_t entry_number;
	uint32_t base_addr;
	uint32_t limit;
	uint32_t flags;
	// and some more empty fields
};
#endif

uint32_t _get_gs_descriptor_idx(void)
	// This fcn returns the user-mode index into the LDT;
	// not to be confused with the actual base_addr associated
	// with that index (visible only in ring 0).
{
	uint32_t _result;
	asm volatile (
		"movl	%%gs, %%eax\n\t"
		"movl	%%eax, %0"
		: "=m"(_result)
		: "m"(_result)
		: "%eax"
	);
	return _result;
}

static uint32_t _get_gs(void)
{
	uint32_t _result;
	asm volatile (
		"movl	%%gs, %%eax\n\t"
		"movl	%%eax, %0"
		: "=m"(_result)
		: "m"(_result)
		: "%eax"
	);
	return _result;
}

uint32_t xi_set_thread_area(XaxPosixEmulation *xpe, struct user_desc *user_desc)
{
	uint32_t entry_number = (_get_gs_descriptor_idx()) >> 3;
            // the caller has a libc without hack_patch_init_tls applied.
            // That won't run on CEIs on other hosts (e.g. Windows),
            // so we fail it early here, too, to make the error obvious.
	lite_assert(user_desc->entry_number != (uint32_t) -1);

	lite_assert(user_desc->entry_number == (uint32_t) -2
		|| user_desc->entry_number==entry_number);

	lite_assert(user_desc->limit == 0xfffff);
		// whole address space. (measured in pages)
	lite_assert(((uint32_t*) user_desc)[3] == 0x51);
		// seg_32bit, limit_in_pages, useable;
		// !contents, !read_exec_only, !seg_not_present
	xpe_install_tls(xpe, user_desc->base_addr);

	// hack_patch_init_tls expects us to give back the very value that
	// should be stuffed into gs. So we'll just read it *out* of gs.
	user_desc->entry_number = _get_gs();
	return 0;
}

#define TRAMPOLINE_STACK_SIZE 0x1000
	// since the underlying PAL implementation borrows some of my
	// stack, I need to make enough for him to use.
	// TODO we should either figure out how to keep him from doing
	// that, or carefully specify the amount a PAL is allowed to use,
	// or have it be something you can ask a PAL dynamically.
	// Or maybe this is all just space we're allocating automatically
	// in xax_thread_trampoline... yup, that's what it is. Maybe.
	// Argh, we need enough stack for the call graph of things like
	// xst_insert.
	// Yeah. Got burned by this. See 2011.03.12.

struct s_thread_start_block {
	void *bootstrap_stack;
	int flags;
	uint32_t *user_stack;
	uint32_t tls_base_addr;
	pid_t *ctid;
	UserRegs user_regs;
	uint32_t start_zutex;
	XaxPosixEmulation *xpe;
	pid_t tid;
	void *initial_gs;
	bool dbg_stack_allocated_with_protected_memory;
};

void xax_thread_trampoline(void *v_tsb)
{
	ThreadStartBlock *tsb = (ThreadStartBlock *) v_tsb;

	// This thread doesn't have a gs installed yet; give it one
	// so that
	// zoog_zutex_wait won't crash, and so
	// xpe_install_tls won't crash when xi_set_thread_area calls it.
	tsb->initial_gs = &tsb->initial_gs;
	(tsb->xpe->zdt->zoog_x86_set_segments)(0, (uint32_t) tsb->initial_gs);

	// wait for the signal to go.
	while (tsb->start_zutex==0)
	{
		ZutexWaitSpec ws;
		ws.zutex = &tsb->start_zutex;
		ws.match_val = 0;
		(tsb->xpe->zdt->zoog_zutex_wait)(&ws, 1);
	}

	debug_announce_thread_f *announce_thread =
		(debug_announce_thread_f *)
		tsb->xpe->zdt->zoog_lookup_extension("debug_announce_thread");
	if (announce_thread!=NULL)
	{
		(*announce_thread)();
		// perhaps there's a way to tell gdb about multiple threads
		// in a core file?
	}

	// Note the stack base in case someone asks for it in /proc/stat
	// This call will attach the stack base to the fake gs we just
	// supplied; when an xpe_install_tls comes along later (perhaps
	// in merely four lines...), it'll be able to find and inherit the
	// stack base info.
	XaxStackTableEntry xte;
	xte.gs_thread_handle = (uint32_t) tsb->initial_gs;
	xte.stack_base = tsb->user_stack;
	xte.tid = tsb->tid;
	xst_insert(&tsb->xpe->stack_table, &xte);

	if (tsb->flags & CLONE_SETTLS)
	{
		xpe_install_tls(tsb->xpe, tsb->tls_base_addr);
	}

	// assume clone thinks everything below the stack pointer
	// is undefined, and borrow some of that space to return to.
	tsb->user_stack[-1] = tsb->user_regs.eip;

	uint32_t user_stack_addr = (uint32_t) (&tsb->user_stack[-1]);

	// Push user stack on bootstrap_stack
	__asm__(
		"push %0;"
		: /* no outputs */
		: "m"(user_stack_addr)
		: "%esp" /* no clobbers */
	);

	UserRegs ur = tsb->user_regs;

	// restore user regs (other than esp)
	// to the state they were in when we were called.
	RESTORE_USER_REGS(ur);

	// switch to user stack
	__asm__("pop %esp");

	// and return to the user eip
	__asm__("ret");
	// we'll never return to here; instead, xi_exit will find
	// the tsb in a hash table and do cleanup.
}

uint32_t xi_clone(XaxPosixEmulation *xpe, int flags, void *user_stack, pid_t *ptid, struct user_desc *tls, pid_t *ctid, UserRegs *user_regs)
{
	if ((flags & CLONE_VM)==0)
	{
		// Looks like fork().
		return -ENOSYS;
	}

	lite_assert(flags & CLONE_VM);
	lite_assert(flags & CLONE_FS);
	lite_assert(flags & CLONE_FILES);
	lite_assert(flags & CLONE_SIGHAND);
	lite_assert(flags & CLONE_THREAD);
	lite_assert(flags & CLONE_SYSVSEM);
	lite_assert(!(flags & CLONE_CHILD_SETTID));	// yet unimpl
	
	// For some reason, the previous version of this code over-allocated
	// extra space; sizeof(struct pthread)=0x470 bytes. What was it
	// looking for? Probably because I wasn't at the kernel interface.
	ThreadStartBlock *tsb;
	tsb = (ThreadStartBlock *) mf_malloc(xpe->mf, sizeof(ThreadStartBlock));
	lite_memset(tsb, 0x69, sizeof(ThreadStartBlock));

	tsb->bootstrap_stack = NULL;
#if 0
	debug_alloc_protected_memory_f *dapm = (debug_alloc_protected_memory_f*)
		((xpe->zdt->zoog_lookup_extension)("debug_alloc_protected_memory"));
	if (dapm!=NULL)
	{
		tsb->bootstrap_stack = (dapm)(TRAMPOLINE_STACK_SIZE);
		tsb->dbg_stack_allocated_with_protected_memory = true;
	}
#endif
	if (tsb->bootstrap_stack == NULL)
	{
		tsb->bootstrap_stack = mf_malloc(xpe->mf, TRAMPOLINE_STACK_SIZE);
		tsb->dbg_stack_allocated_with_protected_memory = false;
	}
	lite_memset(tsb->bootstrap_stack, 0x71, TRAMPOLINE_STACK_SIZE);

	tsb->flags = flags;
	tsb->user_stack = (uint32_t*) user_stack;

	// Here we apply some white-box knowledge about how libc uses clone:
	// clone assumes that the TLS setup has already been done; that GS
	// points at a sane row of the (linux) GDT. It supplies a new base
	// value. We'll assert that's what's going on, and then just set
	// up the base value directly using xax calls.
	// TODO the right long-term fix is a port of libc that provides
	// different sysdeps for createthread.c.

	lite_assert(tls->entry_number == _get_gs()>>3);
	lite_assert(tls->limit == 0xfffff);
	lite_assert(((uint32_t*) tls)[3] == 0x51);
	tsb->tls_base_addr = tls->base_addr;

	tsb->ctid = ctid;
	tsb->user_regs = *(user_regs);
	tsb->start_zutex = 0;
	tsb->xpe = xpe;

	uint32_t *stackptr = (uint32_t*)(((uint32_t) tsb->bootstrap_stack) + TRAMPOLINE_STACK_SIZE);
	stackptr[-1] = (uint32_t) tsb;
	void *trampoline_stack = (void*) &stackptr[-2];

	bool rc = (xpe->zdt->zoog_thread_create)(
		(zoog_thread_start_f*) xax_thread_trampoline, trampoline_stack);
	if (!rc)
	{
		mf_free(xpe->mf, tsb);
		return -ENOMEM;
	}

	// when this code was first written, we provided a tid (via an underlying
	// OS). At cleanup time, we got rid of that call. So for now, we use the
	// tsb->tls address as the tid, which we can discover later through
	// gs:0, and track via the stack_table.
	int tid = thread_table_allocate_pseudotid(&xpe->thread_table);
	tsb->tid = tid;

	thread_table_insert(&xpe->thread_table, tid, tsb);

	// tell child he's the child.
	tsb->user_regs.eax = 0;

	if (flags & CLONE_PARENT_SETTID)
	{
		(*ptid) = tid;
	}

	// Tell the xax_thread_trampoline it's time to go.
	// (This ensures his tsb is in the hash table, so he can't
	// return before we know how to clean him up.)
	tsb->start_zutex = 1;
	(tsb->xpe->zdt->zoog_zutex_wake)(&tsb->start_zutex, tsb->start_zutex, ZUTEX_ALL, 0, NULL, 0);

	return tid;
}

int xi_exit(XaxPosixEmulation *xpe)
{
	ThreadStartBlock *tsb;

	uint32_t thread_gs = get_gs_base();
	XaxStackTableEntry xte;
	xst_remove(&xpe->stack_table, thread_gs, &xte);

	bool rc = thread_table_remove(&xpe->thread_table, xte.tid, &tsb);
	lite_assert(rc);

	if (tsb->flags & CLONE_CHILD_CLEARTID)
	{
		*(tsb->ctid) = 0;
		// "do a wakeup on the futex at that location"
		(tsb->xpe->zdt->zoog_zutex_wake)((uint32_t*) tsb->ctid, *(tsb->ctid), ZUTEX_ALL, 0,
			NULL, 0);
	}

	if (!tsb->dbg_stack_allocated_with_protected_memory)
	{
		mf_free(tsb->xpe->mf, tsb->bootstrap_stack);
	}
	mf_free(tsb->xpe->mf, tsb);

	(xpe->zdt->zoog_thread_exit)();

	lite_assert(0);
	return -1;
}

void xi_xe_register_exit_zutex(XaxPosixEmulation *xpe, uint32_t *zutex)
{
	lite_assert(xpe->exit_zutex == NULL);
	xpe->exit_zutex = zutex;
}

int xi_exit_group(XaxPosixEmulation *xpe, int status)
{
	// We're not really doing this right; we assume that the
	// caller is a single-threaded zguest zone, and pass an out-of-band
	// signal up to a waiting zutex.
	// So which thing do we wake up? For now, we'll just have room for
	// one thing. At some point, we'll need to figure out how to label them.
	if (xpe->exit_zutex != NULL)
	{
		*(xpe->exit_zutex) += 1;
		(xpe->zdt->zoog_zutex_wake)(xpe->exit_zutex, *(xpe->exit_zutex), ZUTEX_ALL, 0, NULL, 0);
	}
	else
	{
		lite_assert(0); // zguest wasn't expecting an exit().
		// tell operator to debug.
	}

	// and let this zone die.
	xi_exit(xpe);
	// Note xi_exit doesn't return.
	return -EINVAL;	// make compiler happy.
}

int xi_uname(XaxPosixEmulation *xpe, struct utsname *name)
{
	lite_strncpy(name->sysname, "Xax-Zoog", sizeof(name->sysname));
	lite_strncpy(name->nodename, "nodename", sizeof(name->nodename));
	lite_strncpy(name->release, "2.6.26-2-686", sizeof(name->release));
		// NB a string glibc is parsing at sysdeps/unix/sysv/linux/dl-osinfo.h;
		// let it cogitate on this.
	lite_strncpy(name->version, "version_0", sizeof(name->version));
	lite_strncpy(name->machine, "i686", sizeof(name->machine));
	lite_strncpy(name->domainname, "xax_unknown_domain", sizeof(name->machine));
	return 0;
}

int xi_madvise(XaxPosixEmulation *xpe, void *addr, size_t length, int advice)
{
	// mmm. Right. So what.
	return 0;
}

int xi_mprotect(XaxPosixEmulation *xpe, const void *addr, size_t len, int prot)
{
	// yup. Totally gonna get right on that request.
	return 0;
}

int xi_msync(XaxPosixEmulation *xpe, void *addr, size_t length, int flags)
{
	// look at me go! Waaay msync'd. No memory's going to slip by my
	// roadblock of synciness!
	return 0;
}

int xi_fsync(XaxPosixEmulation *xpe, int fd)
{
	// An fsync as syncy as msync.
	return 0;
}

int xi_fdatasync(XaxPosixEmulation *xpe, int fd)
{
	return 0;
}

long xi_set_tid_address(XaxPosixEmulation *xpe, int *tidptr)
{
	// TODO ignore tidptr (we don't keep the clear_child_tid promise yet);
	// return a fake pid/tid.
	return xpe->fake_ids.pid;
}

#ifndef FUTEX_CLOCK_REALTIME
#define FUTEX_CLOCK_REALTIME 256
#endif // FUTEX_CLOCK_REALTIME

uint32_t xi_futex(XaxPosixEmulation *xpe, int *uaddr, int op, int val, const struct timespec *timeout, int *uaddr2, int val3)
{
	uint32_t result;

	int flag_mask = (FUTEX_PRIVATE_FLAG|FUTEX_CLOCK_REALTIME);
	int cmd = op & (~flag_mask);
	int flags = op & (flag_mask);
	// NB don't care about PRIVATE_FLAG, since Xax doesn't allow cross-
	// address-space communication -- ALL "futices" are PRIVATE.


	if (!xpe->_initted)
	{
		// getting called during __pthread_initialize_minimal_internal,
		// just as a probe.
		lite_assert(cmd==FUTEX_WAKE);
		lite_assert(flags==FUTEX_PRIVATE_FLAG);
		//lite_assert(val==XI_FUTEX_PROBE_MAGIC);
		lite_assert(0);	// TODO figure out what probe magic was
		result = 0;
	}
	else if (cmd==FUTEX_WAIT)
	{
		ZutexWaitSpec specs[2];
		int nspecs = 1;
		specs[0].zutex = (uint32_t *) uaddr;
		specs[0].match_val = val;

		TimeoutManager timeout_manager;
		bool using_timeout = false;
		bool already_timed_out = false;
		if (timeout!=NULL)
		{
			using_timeout = true;
			init_timeout_manager_ts(xpe, &timeout_manager, timeout);
			ZAS *zas = timeout_manager_get_zas(&timeout_manager);
			bool rc = (zas->method_table->check)(xpe->zdt, zas, &specs[1]);
			if (rc)
			{
				// timeout has already fired.
				// We can't proceed into PAL, because check only populates spec
				// if it returts false (willing to block).
				already_timed_out = true;
			}

			nspecs+=1;
		}

		if (already_timed_out)
		{
			result = -ETIMEDOUT;
		}
		else
		{
			(xpe->zdt->zoog_zutex_wait)(specs, nspecs);

			result = 0;
			if (using_timeout)
			{
				ZAS *zas = timeout_manager_get_zas(&timeout_manager);
				bool rc = (zas->method_table->check)(xpe->zdt, zas, &specs[1]);
				if (rc)
				{
					result = -ETIMEDOUT;
				}
			}
		}

		if (using_timeout)
		{
			free_timeout_manager(&timeout_manager);
		}
	}
	else if (cmd==FUTEX_WAKE)
	{
		result = (xpe->zdt->zoog_zutex_wake)(
			(uint32_t*) uaddr,
			/* match_val */ *uaddr,
			/* n_wake */ val,
			/* n_requeue */ 0,
			/* requeue_zutex */ NULL,
			/* requeue_match_val */ 0);
	}
	else if (cmd==FUTEX_WAKE_OP)
	{
		// Unimplemented, but pthread_cond_signal will fall back on
		// explicit wake & unlock steps, annoying waiters with extra
		// wait cals.
		result = -1;
	}
	else if (cmd==FUTEX_CMP_REQUEUE)
	{
		int32_t nr_wake = val;
		int32_t nr_move = (int32_t) timeout;	// Golly what a horrible ABI.
		result = (xpe->zdt->zoog_zutex_wake)(
			(uint32_t*) uaddr,
			/* match_val */ *uaddr,
			nr_wake,
			nr_move,
			/* requeue_zutex */ (uint32_t*) uaddr2,
			/* requeue_match_val */ val3);
	}
	else if (cmd==FUTEX_WAIT_BITSET)
	{
		// nptl-init is calling us to see if we have some features.
		// say we don't, so we don't hear these things anymore.
		lite_assert(flags == (FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME));
		result = -ENOSYS;
	}
	else
	{
		xax_unimpl_assert();
		result = -1;
	}
	return result;
}

uint32_t xi_set_robust_list(XaxPosixEmulation *xpe, /*struct robust_list_head */ void *head, size_t len)
{
	// Oh YEAH, I TOTALLY took care of that, too.
	return 0;
}

int xi_sched_getparam(XaxPosixEmulation *xpe, pid_t pid, struct sched_param *param)
{
	lite_memset(param, 0, sizeof(*param));
	return 0;
}

int xi_sched_setparam(XaxPosixEmulation *xpe, pid_t pid, const struct sched_param *param)
{
	return 0;
}

int xi_sched_getscheduler(XaxPosixEmulation *xpe, pid_t pid)
{
	return SCHED_OTHER;
}

int xi_sched_setscheduler(XaxPosixEmulation *xpe, pid_t pid, int policy, const struct sched_param *param)
{
	return 0;
}

int xi_sched_get_priority_max(XaxPosixEmulation *xpe, int algorithm)
{
	return 0;
}

int xi_sched_get_priority_min(XaxPosixEmulation *xpe, int algorithm)
{
	return 0;
}

int xi_setpriority(XaxPosixEmulation *xpe, int which, int who, int prio)
{
    // Satisfying ntpdate
    return -ENOSYS;
}

int xi_timer_create(XaxPosixEmulation *xpe, clockid_t clockid, struct sigevent *evp, timer_t *timerid)
{
    // Satisfying ntpdate
    return -ENOSYS;
}


int xi_sched_rr_get_interval(XaxPosixEmulation *xpe, pid_t pid, struct timespec *t)
{
	return -ENOSYS;
}

int xi_sched_yield(XaxPosixEmulation *xpe)
{
	return -ENOSYS;
}

int xi_getrlimit(XaxPosixEmulation *xpe, enum __rlimit_resource resource, struct rlimit *rlimits)
{
	if (resource==RLIMIT_STACK)
	{
		// This is part of the fakeoutery against
		// JavaScriptCore/runtime/Collector.cpp via pthread_getattr_np;
		// see docs in xprocfakeryfs.c for an explanation of why
		// claiming an arbitrary stack size is okay -- it cancels out.
		//
		// (If anyone ever trusts this as the "top" end of the stack,
		// and they're using a smaller stack, well, that would end up bad.)
		//
		// However, it has to be a "big enough" size; otherwise
		// nptl-init.c freaks out.
		// 8MB is the value nptl-init sees on linux
		// (strace -e getrlimit ../lib_links/epiphany-pie),
		// so let's return that.
		rlimits->rlim_cur = 0x800000;
		rlimits->rlim_max = 0x800000;
		return 0;
	}
	else
	{
		return -ENOSYS;
	}
}

int xi_setrlimit(XaxPosixEmulation *xpe, enum __rlimit_resource resource, const struct rlimit *rlimits)
{
	return -ENOSYS;
}

int xi_access(XaxPosixEmulation *xpe, const char *file, int type)
{
	int oflag = 0;
	if (type & R_OK) { oflag |= O_RDONLY; }
	if (type & X_OK) { oflag |= O_RDONLY; }
	if (type & F_OK) { oflag |= O_RDONLY; }
	if (type & W_OK) { oflag |= O_WRONLY; }

	ModeOpenHook xoh(0);
	XfsErr err = XFS_DIDNT_SET_ERROR;
	XVFSHandleWrapper *wrapper
		= xpe_open_hook_handle(xpe, &err, file, oflag, &xoh);
	if (err!=XFS_NO_ERROR)
	{
		lite_assert(wrapper==NULL);
		return -err;
	}

	wrapper->drop_ref();
	return 0;
}

int xi_chmod(XaxPosixEmulation *xpe, const char *path, mode_t mode)
{
	return 0;
}

int xi_fchmod(XaxPosixEmulation *xpe, int fd, mode_t mode)
{
	return 0;
}

int xi_chown(XaxPosixEmulation *xpe, const char *path, uid_t owner, uid_t group)
{
	return 0;
}

int xi_fchown(XaxPosixEmulation *xpe, int fd, uid_t owner, uid_t group)
{
	return -ENOSYS;
}

mode_t xi_umask(XaxPosixEmulation *xpe, mode_t mode)
{
	// lies lies lies
	return mode;
}

int xi_unlink(XaxPosixEmulation *xpe, const char *name)
{
	int result;
	bool path_allocated = false;
	XfsPath xpath;
	XVFSMount *mount;
	mount = _xpe_map_path_to_mount(xpe, name, &xpath);

	if (mount==NULL)
	{
		result = -ENOENT;
		goto exit;
	}
	path_allocated = true;

	XfsErr err;
	err = XFS_DIDNT_SET_ERROR;
	mount->xfs->unlink(&err, &xpath);
	lite_assert(err != XFS_DIDNT_SET_ERROR);
	result = -err;

exit:
	if (path_allocated)
	{
		_xpe_path_free(&xpath);
	}
	return result;
}

int xi_time(XaxPosixEmulation *xpe)
{
	ZClock *zclock = xpe->xax_skinny_network->get_clock();
	uint64_t absolute_ns = zclock->get_time_zoog();
	uint32_t absolute_sec = absolute_ns / 1000000000LL;
	return absolute_sec;
}

int xi_gettimeofday(XaxPosixEmulation *xpe, struct timeval *tv, struct timezone *tz)
{
	ZClock *zclock = xpe->xax_skinny_network->get_clock();
	uint64_t absolute_ns = zclock->get_time_zoog();
	tv->tv_sec = absolute_ns / 1000000000LL;
	tv->tv_usec = (absolute_ns - (tv->tv_sec * 1000000000LL)) / 1000L;
	return 0;
}

int xi_clock_getres(XaxPosixEmulation *xpe, clockid_t clk_id, struct timespec *res)
{
	lite_assert(clk_id==CLOCK_REALTIME || clk_id==CLOCK_MONOTONIC);
	res->tv_sec = 0;
	res->tv_nsec = 500000000;
		// TODO We have no idea what the actual resolution is, because
		// we don't have a way to query the underlying host about that.
		// Sorry! How big of a deal is that? How to applications use
		// this information?
	return 0;
}

int xi_clock_gettime(XaxPosixEmulation *xpe, clockid_t clk_id, struct timespec *tp)
{
	lite_assert(clk_id==CLOCK_REALTIME || clk_id==CLOCK_MONOTONIC);
	ZClock *zclock = xpe->xax_skinny_network->get_clock();
	uint64_t absolute_ns = zclock->get_time_zoog();
	tp->tv_sec = absolute_ns / 1000000000LL;
	tp->tv_nsec = (absolute_ns - (tp->tv_sec * 1000000000LL));
	return 0;
}

static int fake_times(struct tms *buffer)
{
	// version used if no debug func is available.
	return -ENOSYS;
}

clock_t xi_times(XaxPosixEmulation *xpe, struct tms *buffer)
{
	return (xpe->debug_cpu_times)(buffer);
}

static void strace_time_ifc(void *v_this, int* out_real_ms, int* out_user_ms)
{
	XaxPosixEmulation* xpe = (XaxPosixEmulation*) v_this;

	ZClock *zclock = xpe->xax_skinny_network->get_clock();
	uint64_t absolute_ns = zclock->get_time_zoog();
	*out_real_ms = (absolute_ns / 1000000LL);

	struct tms tm;
	xi_times(xpe, &tm);
	const int sysconf_SC_CLK_TCK = 100;	// no clean debug ifc to this yet
	*out_user_ms = tm.tms_utime*1000/sysconf_SC_CLK_TCK;
}

__uid_t xi_geteuid(XaxPosixEmulation *xpe)
{
	return xpe->fake_ids.uid;
}

int xi_getresuid32(XaxPosixEmulation *xpe, uint32_t *ruid, uint32_t *euid, uint32_t *suid)
{
	*ruid = xpe->fake_ids.uid;
	*euid = xpe->fake_ids.uid;
	*suid = xpe->fake_ids.uid;
	return 0;
}

int xi_getresgid32(XaxPosixEmulation *xpe, uint32_t *rgid, uint32_t *egid, uint32_t *sgid)
{
	*rgid = xpe->fake_ids.gid;
	*egid = xpe->fake_ids.gid;
	*sgid = xpe->fake_ids.gid;
	return 0;
}

__gid_t xi_getegid(XaxPosixEmulation *xpe)
{
	return xpe->fake_ids.gid;
}

pid_t xi_getpid(XaxPosixEmulation *xpe)
{
	return xpe->fake_ids.pid;
}

pid_t xi_getpgrp(XaxPosixEmulation *xpe)
{
	return xpe->fake_ids.pid;
}

pid_t xi_getppid(XaxPosixEmulation *xpe)
{
	return xpe->fake_ids.ppid;
}

int xi_ioctl(XaxPosixEmulation *xpe, int fd, unsigned long int request, char *arg)
{
	return -EINVAL;
}

int xi_shmget(XaxPosixEmulation *xpe, key_t key, size_t size, int shmflg)
{
	return -ENOMEM;
}

int xi_mremap(XaxPosixEmulation *xpe, void *old_addr, size_t old_size, size_t new_size, int flags)
{
	return -ENOSYS;
}

LegacyZClock *xi_xe_get_system_clock(XaxPosixEmulation *xpe)
{
	return xpe->xax_skinny_network->get_clock()->get_legacy_zclock();
}

LegacyPF *xi_xe_network_add_default_handler(XaxPosixEmulation *xpe)
{
	PFHandler *pfh = xsn_network_add_default_handler(xpe->xax_skinny_network);
	return legacy_pf_init(pfh);
}

int xi_xe_xpe_mount_vfs(XaxPosixEmulation *xpe, const char *prefix, XaxVFSIfc *xfs)
{
	xpe_mount(xpe, prefix, xfs);
	return 0;
}

void xi_xe_mount_fuse_client(XaxPosixEmulation *xpe, const char *mount_point, consume_method_f consume_method, ZQueue **out_queue, ZoogDispatchTable_v1 **out_xdt)
{
	lite_assert(false);	// need to un-rot xfusefs
#if 0
	XFuseFS *xfusefs = (XFuseFS *) mf_malloc(xpe->mf, sizeof(XFuseFS));
	xfusefs_init(xfusefs, xpe->mf, xpe->zdt, consume_method);
	XfsErr err;
	xpe_mount(xpe, &err, mount_point, &xfusefs->xfs);
	*out_queue = xfusefs->zqueue;
	*out_xdt = xpe->zdt;
#endif
}

uint32_t xi_xe_get_dispatch_table(XaxPosixEmulation *xpe)
{
	return (uint32_t) xpe->zdt;
}

int xi_xe_open_zutex_as_fd(XaxPosixEmulation *xpe, uint32_t *zutex)
{
	ZutexHandle *zh = new ZutexHandle(zutex);
	int fd;
	fd = xpe->handle_table->allocate_filenum();
	xpe->handle_table->assign_filenum(fd, new XVFSHandleWrapper(zh));
	return fd;
}

void xi_xe_mark_perf_point(XaxPosixEmulation *xpe, const char *label)
{
	xpe->perf_measure->mark_time(label);

	debug_new_profiler_epoch_f *dnpe = (debug_new_profiler_epoch_f *)
		(xpe->zdt->zoog_lookup_extension)(DEBUG_NEW_PROFILER_EPOCH_NAME);
	if (dnpe!=NULL)
	{
		(dnpe)(label);
	}

	// Don't throw away zarfile until we're done measuring perf!
	if (lite_strcmp(label, "midori_load_finished")==0)
	{
		xpe->zlcvfs_wrapper->liberate();
	}
}

int xi_getcwd(XaxPosixEmulation *xpe, char *path, int len)
{
	lite_assert(len>=2);
	lite_strcpy(path, "/");
	return lite_strlen(path)+1;
}

int xi_utime(XaxPosixEmulation *xpe, const char *filename, const struct utimbuf *times)
{
	return 0;
}

int xi_getrusage(XaxPosixEmulation *xpe, int who, struct rusage *usage)
{
	lite_memset(usage, 0, sizeof(*usage));
	return 0;
}

uint32_t xi_enosys(XaxPosixEmulation *xpe, uint32_t nr)
{
	return -ENOSYS;
}

uint32_t xpe_dispatch(XaxPosixEmulation *xpe, UserRegs *ur)
{
#define ARG1	(ur->arg1)
#define ARG2	(ur->arg2)
#define ARG3	(ur->arg3)
#define ARG4	(ur->arg4)
#define ARG5	(ur->arg5)
#define ARG6	(ur->arg6)

	//strace_emit_call(&xpe->strace, ur);
	UserRegs ur_strace_copy = *ur;

	uint32_t rc;
	switch (ur->syscall_number)
	{
	case __NR_brk:
		rc = (uint32_t) xi_brk(xpe, (void *) ARG1);
		break;
	case __NR_open:
		rc = xi_open(xpe, (const char *) ARG1, (int) ARG2, (mode_t) ARG3);
		break;
	case __NR_close:
		rc = xi_close(xpe, (int) ARG1);
		break;
	case __NR_read:
		rc = xi_read(xpe, (int) ARG1, (void *) ARG2, (size_t) ARG3);
		break;
	case __NR_write:
		rc = xi_write(xpe, (int) ARG1, (const void *) ARG2, (size_t) ARG3);
		break;
	case __NR_writev:
		rc = xi_writev(xpe, (int) ARG1, (const struct iovec *) ARG2, (int) ARG3);
		break;
	case __NR_fstat64:
		rc = xi_fstat64(xpe, (int) ARG1, (struct stat64 *) ARG2);
		break;
	case __NR_stat64:
		rc = xi_stat64(xpe, (const char *) ARG1, (struct stat64 *) ARG2);
		break;
	case __NR_lstat64:
		rc = xi_lstat64(xpe, (const char *) ARG1, (struct stat64 *) ARG2);
		break;
#define MAKE64(vhi,vlo)	((uint64_t)((((uint64_t) vhi) << 32) | (vlo)))
	case __NR_mmap2:
		rc = (uint32_t) xi_mmap64(xpe, (void *) ARG1, (size_t) ARG2, (int) ARG3, (int) ARG4, (int) ARG5, (uint32_t) ARG6);
		break;
	case __NR_munmap:
		rc = xi_munmap(xpe, (void *) ARG1, (size_t) ARG2);
		break;
	case __NR_select:
		rc = xi_select(xpe, (int) ARG1, (fd_set *) ARG2, (fd_set *) ARG3, (fd_set *) ARG4, (struct timeval *) ARG5);
		break;
	case __NR__newselect:
		// TODO what's the difference between newselect and select?
		rc = xi_select(xpe, (int) ARG1, (fd_set *) ARG2, (fd_set *) ARG3, (fd_set *) ARG4, (struct timeval *) ARG5);
		break;
	case __NR_poll:
		rc = xi_poll(xpe, (struct pollfd *) ARG1, (nfds_t) ARG2, (int) ARG3);
		break;
	case __NR_mkdir:
		rc = xi_mkdir(xpe, (const char *) ARG1, (mode_t) ARG2);
		break;
	case __NR_fcntl:
		rc = xi_fcntl(xpe, (int) ARG1, (int) ARG2, (long) ARG3);
		break;
	case __NR_fcntl64:
		rc = xi_fcntl(xpe, (int) ARG1, (int) ARG2, (long) ARG3);
		break;
	case __NR_getdents:
		rc = xi_getdents(xpe, (unsigned int) ARG1, (struct xi_kernel_dirent *) ARG2, (unsigned int) ARG3);
		break;
	case __NR_getdents64:
		rc = xi_getdents64(xpe, (unsigned int) ARG1, (struct xi_kernel_dirent64 *) ARG2, (unsigned int) ARG3);
		break;
	case __NR_lseek:
		rc = xi_lseek(xpe, (int) ARG1, (off_t) ARG2, (int) ARG3);
		break;
	case __NR__llseek:
		rc = xi_llseek(xpe, (int) ARG1, (int64_t) MAKE64(ARG2,ARG3), (int64_t *) ARG4, (int) ARG5);
		break;
	case __NR_ftruncate:
		rc = xi_ftruncate(xpe, (int) ARG1, (int64_t) ARG2);
		break;
	case __NR_ftruncate64:
		rc = xi_ftruncate(xpe, (int) ARG1, (int64_t) MAKE64(ARG3,ARG2));
			// NB isn't it a lot creepy that the hi/lo arguments are in
			// the opposite order here from llseek? Shudder.
		break;
	case __NR_getuid:
	case __NR_geteuid:
	case __NR_getuid32:
	case __NR_geteuid32:
		rc = xi_getuid(xpe);
		break;
	case __NR_getresuid32:
		rc = xi_getresuid32(xpe, (uint32_t*) ARG1, (uint32_t*) ARG2, (uint32_t*) ARG3);
		break;
	case __NR_getresgid32:
		rc = xi_getresgid32(xpe, (uint32_t*) ARG1, (uint32_t*) ARG2, (uint32_t*) ARG3);
		break;
	case __NR_getgid:
	case __NR_getgid32:
	case __NR_getegid:
	case __NR_getegid32:
		rc = xi_getgid(xpe);
		break;
	case __NR_getpid:
		rc = xi_getpid(xpe);
		break;
	case __NR_getpgrp:
		rc = xi_getpgrp(xpe);
		break;
	case __NR_getppid:
		rc = xi_getppid(xpe);
		break;
	case __NR_pipe:
		rc = xi_pipe2(xpe, (int *) ARG1, 0);
		break;
	case __NR_sigaction:
	case __NR_rt_sigaction:
		rc = xi_sigaction(xpe, (int) ARG1, (const struct sigaction *) ARG2, (struct sigaction *) ARG3);
		break;
	case __NR_kill:
		rc = xi_kill(xpe, (int) ARG1, (int) ARG2);
		break;
	case __NR_uname:
		rc = xi_uname(xpe, (struct utsname *) ARG1);
		break;
	case __NR_madvise:
		rc = xi_madvise(xpe, (void *) ARG1, (size_t) ARG2, (int) ARG3);
		break;
	case __NR_mprotect:
		rc = xi_mprotect(xpe, (const void *) ARG1, (size_t) ARG2, (int) ARG3);
		break;
	case __NR_msync:
		rc = xi_msync(xpe, (void *) ARG1, (size_t) ARG2, (int) ARG3);
		break;
	case __NR_fsync:
		rc = xi_fsync(xpe, (int) ARG1);
		break;
	case __NR_fdatasync:
		rc = xi_fdatasync(xpe, (int) ARG1);
		break;
	case __NR_set_tid_address:
		rc = xi_set_tid_address(xpe, (int *) ARG1);
		break;
	case __NR_futex:
		rc = xi_futex(xpe, (int *) ARG1, (int) ARG2, (int) ARG3, (const struct timespec *) ARG4, (int *) ARG5, (int) ARG6);
		break;
	case __NR_set_robust_list:
		rc = xi_set_robust_list(xpe, /*struct robust_list_head */ (void *) ARG1, (size_t) ARG2);
		break;
	case __NR_sched_getparam:
		rc = xi_sched_getparam(xpe, (pid_t) ARG1, (struct sched_param *) ARG2);
		break;
	case __NR_sched_setparam:
		rc = xi_sched_setparam(xpe, (pid_t) ARG1, (const struct sched_param *) ARG2);
		break;
	case __NR_sched_getscheduler:
		rc = xi_sched_getscheduler(xpe, (pid_t) ARG1);
		break;
	case __NR_sched_setscheduler:
		rc = xi_sched_setscheduler(xpe, (pid_t) ARG1, (int) ARG2, (const struct sched_param *) ARG3);
		break;
	case __NR_sched_get_priority_max:
		rc = xi_sched_get_priority_max(xpe, (int) ARG1);
		break;
	case __NR_sched_get_priority_min:
		rc = xi_sched_get_priority_min(xpe, (int) ARG1);
		break;
    case __NR_setpriority:
        rc = xi_setpriority(xpe, (int) ARG1, (int) ARG2, (int) ARG3);
        break;
      case __NR_timer_create:
        rc = xi_timer_create(xpe, (clockid_t) ARG1, (struct sigevent *) ARG2, (timer_t *) ARG3);
        break;
	case __NR_sched_rr_get_interval:
		rc = xi_sched_rr_get_interval(xpe, (pid_t) ARG1, (struct timespec *) ARG2);
		break;
	case __NR_sched_yield:
		rc = xi_sched_yield(xpe);
		break;
	case __NR_ugetrlimit:
	case __NR_getrlimit:
		rc = xi_getrlimit(xpe, (enum __rlimit_resource) ARG1, (struct rlimit *) ARG2);
		break;
	case __NR_setrlimit:
		rc = xi_setrlimit(xpe, (enum __rlimit_resource) ARG1, (const struct rlimit *) ARG2);
		break;
	case __NR_access:
		rc = xi_access(xpe, (const char *) ARG1, (int) ARG2);
		break;
	case __NR_chmod:
		rc = xi_chmod(xpe, (const char *) ARG1, (mode_t) ARG2);
		break;
	case __NR_fchmod:
		rc = xi_fchmod(xpe, (int) ARG1, (mode_t) ARG2);
		break;
	case __NR_chown32:
		rc = xi_chown(xpe, (const char *) ARG1, (uid_t) ARG2, (gid_t) ARG3);
		break;
	case __NR_fchown32:
		rc = xi_fchown(xpe, (int) ARG1, (uid_t) ARG2, (gid_t) ARG3);
		break;
	case __NR_dup:
		rc = xi_dup(xpe, (int) ARG1);
		break;
	case __NR_dup2:
		rc = xi_dup2(xpe, (int) ARG1, (int) ARG2);
		break;
	case __NR_umask:
		rc = xi_umask(xpe, (mode_t) ARG1);
		break;
	case __NR_unlink:
		rc = xi_unlink(xpe, (const char *) ARG1);
		break;
	case __NR_time:
		rc = xi_time(xpe);
		break;
	case __NR_gettimeofday:
		rc = xi_gettimeofday(xpe, (struct timeval *) ARG1, (struct timezone *) ARG2);
		break;
	case __NR_times:
		rc = xi_times(xpe, (struct tms *) ARG1);
		break;
	case __NR_ioctl:
		rc = xi_ioctl(xpe, (int) ARG1, (unsigned long int) ARG2, (char *) ARG3);
		break;
#if 0
	case __NR_shmget:
		rc = xi_shmget(xpe, (key_t) ARG1, (size_t) ARG2, (int) ARG3);
		break;
#endif
	case __NR_mremap:
		rc = xi_mremap(xpe, (void *) ARG1, (size_t) ARG2, (size_t) ARG3, (int) ARG4);
		break;
#if 0
		rc = xi_install_tls(xpe, uint32_t new_gs);
#endif
	case __NR_exit_group:
		rc = xi_exit_group(xpe, (int) ARG1);
		break;
	case __NR_set_thread_area:
		rc = xi_set_thread_area(xpe, (struct user_desc *) ARG1);
		break;
	case __NR_clone:
		rc = xi_clone(xpe, (int) ARG1, (void*) ARG2, (pid_t*) ARG3, (struct user_desc*) ARG4, (pid_t*) ARG5, ur);
		break;
	case __NR_exit:
		rc = xi_exit(xpe);
		break;
	case __NR_link:
		rc = xi_link(xpe, (const char*) ARG1, (const char*) ARG2);
		break;
	case __NR_socketcall:
		rc = xi_socketcall(xpe, (int) ARG1, (SocketcallArgs *) ARG2);
		break;
	case __NR_statfs:
	case __NR_statfs64:
		rc = xi_statfs64(xpe, (const char *) ARG1, (struct statfs *) ARG2);
		break;
	case __NR_fstatfs:
	case __NR_fstatfs64:
		rc = xi_fstatfs64(xpe, (int) ARG1, (struct statfs *) ARG2);
		break;
	case __NR_clock_getres:
		rc = xi_clock_getres(xpe, ARG1, (struct timespec *) ARG2);
		break;
	case __NR_clock_gettime:
		rc = xi_clock_gettime(xpe, ARG1, (struct timespec *) ARG2);
		break;
	case __NR_getcwd:
		rc = xi_getcwd(xpe, (char*) ARG1, ARG2);
		break;
	case __NR_rt_sigprocmask:
		rc = xi_sigprocmask(xpe, (int) ARG1, (const sigset_t *) ARG2, (sigset_t *) ARG3);
		break;
	case __NR_rename:
		rc = xi_rename(xpe, (const char*) ARG1, (const char *) ARG2);
		break;
	case __NR_nanosleep:
		rc = xi_nanosleep(xpe, (const struct timespec *) ARG1, (struct timespec *) ARG2);
		break;
	case __NR_readlink:
		rc = xi_readlink(xpe, (const char *) ARG1, (char *) ARG2, (size_t) ARG3);
		break;

	case __NR_setitimer:
	case __NR_sched_getaffinity:
	case __NR_ipc:
#ifdef __NR_inotify_init1
	case __NR_inotify_init1:
#endif // __NR_inotify_init1
	case __NR_inotify_init:
	case __NR_sysfs:
	case __NR_flock:
		rc = xi_enosys(xpe, ur->syscall_number);
		break;
	case __NR_utime:
		rc = xi_utime(xpe, (const char *) ARG1, (const struct utimbuf *) ARG2);
		break;
	case __NR_getrusage:
		rc = xi_getrusage(xpe, (int) ARG1, (struct rusage *) ARG2);
		break;
#ifdef __NR_pipe2
	case __NR_pipe2:
		rc = xi_pipe2(xpe, (int *) ARG1, ARG2);
		break;
#endif // __NR_pipe2

	case __NR_xe_get_system_clock:
		rc = (uint32_t) xi_xe_get_system_clock(xpe);
		break;
	case __NR_xe_network_add_default_handler:
		rc = (uint32_t) xi_xe_network_add_default_handler(xpe);
		break;
	case __NR_xe_xpe_mount_vfs:
		rc = xi_xe_xpe_mount_vfs(xpe, (const char *) ARG1, (XaxVFSIfc *) ARG2);
		break;
	case __NR_xe_allow_new_brk:
		xpe->xbrk.allow_new_brk(ARG1, (const char*) ARG2);
		rc = 0;
		break;
	case __NR_xe_wait_until_all_brks_claimed:
		rc = xpe->xbrk.wait_until_all_brks_claimed();
		break;
	case __NR_xe_mount_fuse_client:
		xi_xe_mount_fuse_client(xpe, (const char *) ARG1, (consume_method_f*) ARG2, (ZQueue **) ARG3, (ZoogDispatchTable_v1 **) ARG4);
		rc = 0;
		break;
	case __NR_xe_register_exit_zutex:
		xi_xe_register_exit_zutex(xpe, (uint32_t *) ARG1);
		rc = 0;
		break;
	case __NR_xe_get_dispatch_table:
		rc = xi_xe_get_dispatch_table(xpe);
		break;
	case __NR_xe_open_zutex_as_fd:
		rc = xi_xe_open_zutex_as_fd(xpe, (uint32_t *) ARG1);
		break;
	case __NR_xe_mark_perf_point:
		xi_xe_mark_perf_point(xpe, (const char *) ARG1);
		rc = 0;
		break;
	case __NR_setresuid32:
		rc = 0;
		break;
	case __NR_setresgid32:
		rc = 0;
		break;
	case __NR_fadvise64_64:
		rc = 0;
		break;
	default:
		INVOKE_DEBUGGER_RAW();
		rc = -ENOSYS;
		break;
		lite_assert(0);
	}

	if (xpe->strace_flag)
	{
		strace_emit_trace(&xpe->strace, &ur_strace_copy, rc);
	}

	return rc;
}

uint32_t getCurrentTime() {
	lite_assert(false); // PAL currently doesn't know what time it is!
	return 0;
}
