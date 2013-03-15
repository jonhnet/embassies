#include "genode_thread_test.h"

#include <base/env.h>
#include <base/printf.h>
#include <zoog_monitor_session/client.h>
#include <timer_session/connection.h>
#include <base/sleep.h>
#include <base/thread.h>

using namespace Genode;

class TestThread : public Thread<128>
{
private:
	int tid;
	Timer::Connection *timer;

public:
	TestThread(int tid, Timer::Connection *timer)
		: tid(tid), timer(timer) {}

	void entry() {
		timer = new Timer::Connection();
		while (true) {
			PDBG("Thread %d", tid);
			timer->msleep(1000);
		}
	}
};

void genode_thread_test()
{
	Timer::Connection timer;

	TestThread *t;
	t = new TestThread(1, &timer);
	t->start();
	t = new TestThread(2, &timer);
	t->start();
	t = new TestThread(6, &timer);
	t->start();
	while (true) {
		PDBG("root");
		timer.msleep(10000);
	}
}
