#include <util/string.h>
#include "LiteLib.h"
#include "common/Thread_id.h"

using namespace Genode;

Thread_id::Thread_id()
{
	memset(space, 0, sizeof(space));
}

Thread_id Thread_id::get_my()
{
	Thread_id out;
	lite_assert(Thread_id_size() <= sizeof(out.space));
	Thread_id_my(out.space);
	return out;
}

Thread_id Thread_id::get_invalid()
{
	Thread_id out;
	lite_assert(Thread_id_size() <= sizeof(out.space));
	Thread_id_invalid(out.space);
	return out;
}

bool Thread_id::operator==(Thread_id& other)
{
	return memcmp(space, other.space, sizeof(space))==0;
}
