#pragma once

#include "pal_abi/pal_abi.h"
#include "pal_abi/pal_extensions.h"
#include "SyncFactory.h"

class TraceCollector;

class TraceCollectorHandle
{
private:
	TraceCollector* collector;
	uint32_t id;

public:
	TraceCollectorHandle(TraceCollector* collector, uint32_t id);
	~TraceCollectorHandle();
	void read(size_t len, uint64_t offset);
	void mmap(size_t len, uint64_t offset, bool fast);
};

class TraceCollector
{
private:
	uint32_t _next_id;
	SyncFactoryMutex* mutex;
	debug_logfile_append_f* logfile_append;

	friend class TraceCollectorHandle;
	void _emit(const char* buf);
	void mmap(uint32_t id, size_t len, uint64_t offset, bool fast);
	void read(uint32_t id, size_t len, uint64_t offset);

public:
	TraceCollector(SyncFactory* sf, ZoogDispatchTable_v1* zdt);
	TraceCollectorHandle* open(const char* path);
	void terminate_trace();
};
