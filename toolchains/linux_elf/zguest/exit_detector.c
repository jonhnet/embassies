#include <stdio.h>
#include "exit_detector.h"
#include "xax_extensions.h"

void exit_detector_wait(DetectorIfc *di, ZoogDispatchTable_v1 *xdt);

void exit_detector_init(ExitDetector *ed)
{
	ed->detector_ifc.wait_func = exit_detector_wait;
	ed->zutex = 0;
	xe_register_exit_zutex(&ed->zutex);
}

void exit_detector_wait(DetectorIfc *di, ZoogDispatchTable_v1 *xdt)
{
	ExitDetector *ed = (ExitDetector *) di;

	ZutexWaitSpec spec;
	while (1)
	{
		spec.zutex = &ed->zutex;
		spec.match_val = ed->zutex;
		if (spec.match_val != 0)
		{
			break;
		}

		(xdt->xax_zutex_wait)(&spec, 1);
	}
	fprintf(stderr, "exit_detector_wait detected exit\n");
}
