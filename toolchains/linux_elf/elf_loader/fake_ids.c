#include "fake_ids.h"

void fake_ids_init(FakeIds *fid)
{
	fid->uid = 1001;
	fid->gid = 1002;
	fid->ppid = 768;
	fid->pid = 769;
}
