#include "errno.h"
#include "ZLCVFS.h"
#include "ZarfileVFS.h"
#include "UnionVFS.h"
#include "PatchPogoFilter.h"

#include "zlcvfs_init.h"
#include "perf_measure.h"
#include "ZLCEmitXdt.h"

ZLCVFS_Wrapper::ZLCVFS_Wrapper(XaxPosixEmulation *xpe, const char *path, const char *zarfile_path_str, bool trace)
{
	zarfile_vfs = NULL;
	union_vfs = NULL;
	vfs = NULL;
	sf = new SyncFactory_Zutex(xpe->zdt);

	ZLCEmit* ze = new ZLCEmitXdt(xpe->zdt, terse);

	traceCollector = NULL;
	if (trace)
	{
		traceCollector = new TraceCollector(sf, xpe->zdt);
	}

	ZLCVFS *zlc_vfs = new ZLCVFS(xpe, ze, traceCollector);
	XfsErr err = (XfsErr) ENOENT;
	XaxVFSHandleIfc *zarfile_hdl = NULL;
	PerfMeasure *perf_measure = xpe->perf_measure;

	if (zarfile_path_str!=NULL)
	{
		XfsPath zarfile_path = { (char*) zarfile_path_str, (char*) zarfile_path_str };

		perf_measure->mark_time("request_zarfile");
		zarfile_hdl = zlc_vfs->open(
			&err, &zarfile_path, O_RDONLY);
	}

	if (err != XFS_NO_ERROR)
	{
		perf_measure->mark_time("zarfile_absent");
		vfs = zlc_vfs; // Hmm, nothing to layer -- no zarfile present.
	}
	else
	{
		perf_measure->mark_time("zarfile_received");

		debug_new_profiler_epoch_f *dnpe = (debug_new_profiler_epoch_f *)
			(xpe->zdt->zoog_lookup_extension)(DEBUG_NEW_PROFILER_EPOCH_NAME);
		if (dnpe!=NULL)
		{
			(dnpe)("zftp_zarfile_received");
		}

		zarfile_vfs = new ZarfileVFS(xpe->mf, sf, zarfile_hdl, ze);

		// Look in a zarfile first, and then fall back to origin server
		// per-file.
		union_vfs = new UnionVFS(xpe->mf);
		union_vfs->push_layer(zlc_vfs);
		union_vfs->push_layer(zarfile_vfs);
		vfs = union_vfs;
	}

	// Whatever comes out of ZFTP, apply the ELF Pogo patches to ELF files.
	PatchPogoVFS *ppvfs = new PatchPogoVFS(vfs, xpe->mf);

	// ...and mount it.
	xpe_mount(xpe, mf_strdup(xpe->mf, path), ppvfs);
}

void ZLCVFS_Wrapper::liberate()
{
	if (union_vfs!=NULL)
	{
		XaxVFSIfc *ifc = union_vfs->pop_layer();
		lite_assert(ifc == (XaxVFSIfc *) zarfile_vfs);
		delete zarfile_vfs;
		union_vfs = NULL;
	}
	if (traceCollector != NULL)
	{
		traceCollector->terminate_trace();
	}
}

