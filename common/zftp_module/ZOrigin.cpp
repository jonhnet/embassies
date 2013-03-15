#include <string.h>

#include "LiteLib.h"
#include "hash_table.h"
#include "FreshnessSeal.h"
#include "ZOrigin.h"

ZOrigin::~ZOrigin()
{
}

uint32_t ZOrigin::__hash__(const void *v_a)
{
	ZOrigin *a = (ZOrigin *) v_a;
	const char *name = a->get_origin_name();
	return hash_buf((void*) name, lite_strlen(name));
}

int ZOrigin::__cmp__(const void *v_a, const void *v_b)
{
	ZOrigin *a = (ZOrigin *) v_a;
	ZOrigin *b = (ZOrigin *) v_b;
	return lite_strcmp(a->get_origin_name(), b->get_origin_name());
}

ZOriginKey::ZOriginKey(const char *name)
{
	this->name = name;
}

bool ZOriginKey::lookup_new(const char *url)
{
	lite_assert(false);
	return false;
}

bool ZOriginKey::validate(const char *url, FreshnessSeal *seal)
{
	lite_assert(false);
	return false;
}

const char *ZOriginKey::get_origin_name()
{
	return name;
}
