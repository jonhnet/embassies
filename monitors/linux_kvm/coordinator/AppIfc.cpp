#include "LiteLib.h"
#include "AppIfc.h"
#include "App.h"

AppIfc::AppIfc(XIPAddr addr, App *app)
{
	this->addr = addr;
	this->app = app;
}

AppIfc::AppIfc(XIPAddr addr)
{
	this->addr = addr;
	this->app = NULL;
}

uint32_t AppIfc::hash(const void *v_a)
{
	return hash_xip(&((AppIfc*)v_a)->addr);
}

int AppIfc::cmp(const void *v_a, const void *v_b)
{
	return cmp_xip(
		&((AppIfc*)v_a)->addr,
		&((AppIfc*)v_b)->addr);
}
