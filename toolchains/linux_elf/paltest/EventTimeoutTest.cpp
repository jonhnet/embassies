#include "EventTimeoutTest.h"
#include "zemit.h"

EventTimeoutTest::EventTimeoutTest(XaxSkinnyNetwork *xsn, ZLCEmit *ze)
	: xsn(xsn),
	  ze(ze)
{
}

void EventTimeoutTest::run()
{
	SyncFactoryEvent *evt = xsn->get_timer_sf()->new_event(false);
	for (int i=0; i<4; i++)
	{
		evt->wait(1500);	
		ZLC_TERSE(ze,, "Waited 1500ms, event timed out.\n");
	}
}
