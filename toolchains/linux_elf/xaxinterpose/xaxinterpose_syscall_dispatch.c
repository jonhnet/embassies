#include <stdint.h>

uint32_t xaxinterpose_syscall_dispatch(
	uint32_t a0,
	uint32_t a1,
	uint32_t a2,
	uint32_t a3,
	uint32_t a4,
	uint32_t a5)
{
	__asm__("int $0x80;");
	return 0;
}
