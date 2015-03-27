/*
 * \brief  Zoog PAL: genode-specific bootstrap layer for Zoog ABI
 * \author Jon Howell
 * \date   2011-12-22
 */

#include <base/env.h>
#include <base/printf.h>
#include <zoog_monitor_session/client.h>
#include <timer_session/connection.h>
#include <zoog_monitor_session/connection.h>
#include <dataspace/client.h>
#include <base/sleep.h>

#include "Genode_pal.h"
#include "pal_abi/pal_abi.h"
#include "pal_abi/pal_extensions.h"
#include "genode_thread_test.h"

using namespace Genode;

int main(void)
{
	Timer::Connection timer;
	if (0) {
		PDBG("Waiting for debugger");
		timer.msleep(5000);
		PDBG("Starting");
	}

//	genode_thread_test();

	PDBG("calling GenodePAL ctor\n");
	GenodePAL pal;
	return 0;
}
