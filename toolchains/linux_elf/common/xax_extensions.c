#include <unistd.h>	//syscall
#include "xax_extensions.h"

LegacyZClock *xe_get_system_clock(void)
{
	int rc = syscall(__NR_xe_get_system_clock);
	return (LegacyZClock *) rc;
}

#if 0
uint16_t xe_allocate_port_number(void)
{
	int rc = syscall(__NR_xe_allocate_port_number);
	return (uint16_t) rc;
}
#endif

LegacyPF *xe_network_add_default_handler()
{
	int rc = syscall(__NR_xe_network_add_default_handler);
	return (LegacyPF *) rc;
}

int xe_xpe_mount_vfs(const char *prefix, void /*XaxFileSystem*/ *xfs)
{
	return syscall(__NR_xe_xpe_mount_vfs, prefix, xfs);
}

void xe_allow_new_brk(uint32_t size, const char *dbg_desc)
{
	syscall(__NR_xe_allow_new_brk, size, dbg_desc);
}

void xe_wait_until_all_brks_claimed(void)
{
	syscall(__NR_xe_wait_until_all_brks_claimed);
}

/*
void xe_mount_fuse_client(const char *mount_point, consume_method_f *consume_method, ZQueue **out_queue, ZoogDispatchTable_v1 **out_xdt)
{
	syscall(__NR_xe_mount_fuse_client, mount_point, consume_method, out_queue, out_xdt);
}
*/

void xe_register_exit_zutex(uint32_t *zutex)
{
	syscall(__NR_xe_register_exit_zutex, zutex);
}

ZoogDispatchTable_v1 *xe_get_dispatch_table()
{
	return (ZoogDispatchTable_v1 *) syscall(__NR_xe_get_dispatch_table);
}

int xe_open_zutex_as_fd(uint32_t *zutex)
{
	return (int) syscall(__NR_xe_open_zutex_as_fd, zutex);
}
