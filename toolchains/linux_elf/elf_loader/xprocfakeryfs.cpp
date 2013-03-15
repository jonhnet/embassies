#include <errno.h>
#include <string.h>
#include <assert.h>

#include "LiteLib.h"
#include "XRamFS.h"
#include "xprocfakeryfs.h"
#include "xax_stack_table.h"
#include "cheesy_snprintf.h"
#include "XRFileNode.h"
#include "xax_util.h"

// simulate the /proc/stat/self file to fool libgc's search for
// stack base. (gulp)
//
// This module now also simulates /proc/stat/maps to fool libpthreads' search
// for stack base. It returns a file containing only stack sections,
// synthesized from the stack_table.

// Simulate /proc/self/maps to supply a stack top (er, bottom -- highest
// memory, empty stack address) value for the
// JavaScriptCore/runtime/Collector.cpp garbage collector.
// It gets its stack top value via pthread_getattr_np,
// which returns a stackBase and a stackSize.
// pthread_getattr_np itself leafs through /proc/self/maps, looking
// for the memory range that includes __libc_stack_end (a global value
// spirited away elsewhere in the bowels of libc, but that should be in the
// stack somewhere). Then it uses the /maps range to find the upper end
// of that, bounds its size by the end of the next memory region down
// in /maps, and then computes the StackBase from top-size.
//
// Since Collector never actually looks at StackBase, it's sufficient to
// make up a random size, let pthread_getattr_np subtract it to
// create a bogus stackBase, and let Collector add it back in to
// cancel it out.
// 
// Of course it's never quite that easy. pthread_getattr_np begins its
// stack size estimate with a value returned by getrlimit; if that call
// fails, it has a cow and returns a zero-sized stack to Collector,
// who in turn explodes.
// So xax_posix_emulation returns a sorta-arbitrary value for
// ugetrlimit(RLIMIT_STACK) ... enough to keep nptl-init happy (who was,
// quite frankly, pretty happy with -ENOSYS). It also used in the
// pthread_getattr_np computation, affecting stackBase.
// stackBase would also be adjusted by the previous line in the /maps
// file, except that we only provide a one-line /maps file, so the
// RLIMIT_STACK value ends up winning by being the only one present.
// But, as described above, it also just doesn't matter -- it gets
// canceled back out in Collector, who only cares about the stack top value.

class XProcFakeHandle : public XRFileNodeHandle
{
public:
	XProcFakeHandle(XRFileNode *synthetic_file_node);
	~XProcFakeHandle();
private:
	XRFileNode *synthetic_node_to_delete;
};

XProcFakeHandle::XProcFakeHandle(XRFileNode *synthetic_file_node)
	: XRFileNodeHandle(synthetic_file_node)
{
	this->synthetic_node_to_delete = synthetic_file_node;
}

XProcFakeHandle::~XProcFakeHandle()
{
	// this file is *only* visible from this handle; is was
	// rolled up just to please this one thread; not mounted into
	// the RAM FS.
	delete synthetic_node_to_delete;
}

class FakeFileFillerIfc
{
public:
	virtual ~FakeFileFillerIfc() {}
	virtual void fill(XaxVFSHandleIfc *hdl, XaxStackTable *stack_table) = 0;
};

class FakeStatFill : public FakeFileFillerIfc
{
public:
	virtual void fill(XaxVFSHandleIfc *hdl, XaxStackTable *stack_table);
};

void FakeStatFill::fill(XaxVFSHandleIfc *hdl, XaxStackTable *stack_table)
{
	XfsErr err;
	uint64_t offset = 0;
	int dummy_i;
	for (dummy_i=0; dummy_i<27; dummy_i++)
	{
		hdl->write(&err, "0 ", 2, offset);
		lite_assert(err==XFS_NO_ERROR);
		offset += 2;
	}

	XaxStackTableEntry *xte = xst_peek(stack_table, get_gs_base());
	xax_assert(xte->stack_base!=0);

	char buf[80];
	utoa(buf, (uint32_t) xte->stack_base);
	int e = lite_strlen(buf);
	buf[e++] = '\n';

	hdl->write(&err, buf, e, offset);
	lite_assert(err==XFS_NO_ERROR);
	offset += e;
}

class FakeMapsFill : public FakeFileFillerIfc
{
public:
	virtual void fill(XaxVFSHandleIfc *hdl, XaxStackTable *stack_table);
private:
	void _add_line(XaxVFSHandleIfc *hdl, uint64_t *offset, uint32_t lo, uint32_t hi);
};

void FakeMapsFill::_add_line(XaxVFSHandleIfc *hdl, uint64_t *offset, uint32_t lo, uint32_t hi)
{
	char buf[80];
	hexstr_08x(&buf[0], lo);
	buf[8] = '-';
	hexstr_08x(&buf[9], hi);
	buf[17] = ' ';
	buf[18] = '\n';
	buf[19] = '\0';

	uint32_t len = lite_strlen(buf);
	XfsErr err;
	hdl->write(&err, buf, len, *offset);
	lite_assert(err==XFS_NO_ERROR);
	*offset += len;
}

void FakeMapsFill::fill(XaxVFSHandleIfc *hdl, XaxStackTable *stack_table)
{
	uint64_t offset = 0;

	XaxStackTableEntry *xte = xst_peek(stack_table, get_gs_base());
	xax_assert(xte->stack_base!=0);

	_add_line(hdl, &offset, 0, (uint32_t) xte->stack_base);
}

class XFakeProcNode : public XRNode
{
public:
	XFakeProcNode(
		XRamFS *xramfs, XaxPosixEmulation *xpe, const char *path, FakeFileFillerIfc *filler);
	~XFakeProcNode();

	virtual XRNodeHandle *open_node(XfsErr *err, const char *path, int oflag, XVOpenHooks *xoh);
	virtual void mkdir_node(XfsErr *err, const char *new_name, mode_t mode);
	
private:
	FakeFileFillerIfc *filler;
	XaxStackTable *stack_table;

	class InstallFakeProcHook : public XVOpenHooks
	{
	public:
		InstallFakeProcHook(XRNode *node) : node(node) {}
		virtual XRNode *create_node() { return node; }
		virtual mode_t get_mode() { return 0777; }
		virtual void *cheesy_rtti() { static int rtti; return &rtti; }
	private:
		XRNode *node;
	};
};

XFakeProcNode::XFakeProcNode(XRamFS *xramfs, XaxPosixEmulation *xpe, const char *path, FakeFileFillerIfc *filler)
	: XRNode(xramfs)
{
	this->stack_table = &xpe->stack_table;
	this->filler = filler;

	InstallFakeProcHook hook(this);
	int fd = xpe_open_hook_fd(xpe, path, O_WRONLY|O_CREAT, &hook);
	xi_close(xpe, fd);
}

XFakeProcNode::~XFakeProcNode()
{
	delete filler;
}

XRNodeHandle *XFakeProcNode::open_node(XfsErr *err, const char *path, int oflag, XVOpenHooks *xoh)
{
	XRFileNode *file = new XRFileNode(get_xramfs());
	// NB we create a new *file* for every open call, since
	// we want the contents of the file to be specialized to the
	// *thread* that made the call. /proc is funky that way.
	// We'll attach the file to the handle to clean it up when
	// the handle closes.

	XRNodeHandle *hdl = file->open_node(err, "", 0, NULL);
	filler->fill(hdl, stack_table);

	return hdl;

}

void XFakeProcNode::mkdir_node(XfsErr *err, const char *new_name, mode_t mode)
{
	*err = (XfsErr) EINVAL;
}

void xprocfakeryfs_init(XaxPosixEmulation *xpe)
{
	XRamFS *ramfs = xax_create_ramfs(xpe, "/proc/self");

	new XFakeProcNode(ramfs, xpe, "/proc/self/stat", new FakeStatFill());
	new XFakeProcNode(ramfs, xpe, "/proc/self/maps", new FakeMapsFill());
}
