#include "base/allocator_avl.h"
#include "util/misc_math.h"
#include "CoreWriter.h"

using namespace Genode;

CoreWriter::CoreWriter(ChannelWriterClient *cw, uint32_t size)
	: _cw(cw),
	  dbg_size(size>0 ? size : 1),
	  dbg_count(0),
	  dbg_ui_units(0)
{
	_cw->open("core", size);
}

CoreWriter::~CoreWriter()
{
	_cw->close();
}

void CoreWriter::write(void *ptr, int len)
{
	_cw->write((uint8_t*) ptr, len);
	dbg_count += len;
	uint32_t new_dbg_ui_units = ((dbg_count>>10) * 100 / (dbg_size>>10));
	if (new_dbg_ui_units != dbg_ui_units)
	{
		dbg_ui_units = new_dbg_ui_units;
		PDBG("Core: %d%% len=%dkB count %dkB, total %dkB", dbg_ui_units, len>>10, dbg_count>>10, dbg_size>>10);
	}
}

bool CoreWriter::static_write(void *v_core_writer, void *ptr, int len)
{
	((CoreWriter*) v_core_writer)->write(ptr, len);
	return true;
}
