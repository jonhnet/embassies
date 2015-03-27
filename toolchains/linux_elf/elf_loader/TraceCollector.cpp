#include <alloca.h>

#include "TraceCollector.h"
#include "cheesy_snprintf.h"
#include "LiteLib.h"

TraceCollectorHandle::TraceCollectorHandle(TraceCollector* collector, uint32_t id)
	: collector(collector),
	  id(id)
{
}

TraceCollectorHandle::~TraceCollectorHandle()
{
	// this is where we'd emit a CLOSE record, if we wanted that sort of thing.
}

void TraceCollectorHandle::read(size_t len, uint64_t offset)
{
	collector->read(id, len, offset);
}

void TraceCollectorHandle::mmap(size_t len, uint64_t offset, bool fast)
{
	collector->mmap(id, len, offset, fast);
}

//////////////////////////////////////////////////////////////////////////////

void TraceCollector::read(uint32_t id, size_t len, uint64_t offset)
{
	mutex->lock();
	char buf[128];
	cheesy_snprintf(buf, sizeof(buf), "READ %08x %08x %08x\n", id, len, offset);
	_emit(buf);
	mutex->unlock();
}

void TraceCollector::mmap(uint32_t id, size_t len, uint64_t offset, bool fast)
{
	mutex->lock();
	char buf[128];
	lite_assert(offset < 0xffffffff);	// TODO not actually 64-bit-ready
	cheesy_snprintf(buf, sizeof(buf), "MMAP %08x %08x %08x %08x\n",
		id, len, (uint32_t) offset, fast);
	_emit(buf);
	mutex->unlock();
}

TraceCollectorHandle* TraceCollector::open(const char* path)
{
	if (lite_starts_with("stat:", path))
	{
		return NULL;
	}

	mutex->lock();
	uint32_t id = _next_id;
	_next_id += 1;
	int len = lite_strlen(path)+20;
	char* buf = (char*) alloca(len);
	cheesy_snprintf(buf, len, "OPEN %08x /%s\n", id, path);
		// NB ZLCVFS calls us with the / mount point trimmed off; I put it back.
	_emit(buf);
	mutex->unlock();
	return new TraceCollectorHandle(this, id);
}

void TraceCollector::_emit(const char* buf)
{
	if (logfile_append!=NULL)
	{
		logfile_append("trace", buf);
	}
}

TraceCollector::TraceCollector(SyncFactory* sf, ZoogDispatchTable_v1* zdt)
	: _next_id(1),
	  mutex(sf->new_mutex(false))
{
	logfile_append = (debug_logfile_append_f*)
		((zdt->zoog_lookup_extension)(DEBUG_LOGFILE_APPEND_NAME));
}

void TraceCollector::terminate_trace()
{
	// Called when we've stopped doing anything perf-related; so we
	// should stop recording the trace, to get fairly consistent-looking
	// traces across (say) hot, warm variations.
	logfile_append = NULL;
}
