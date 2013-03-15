// We carefully use different string funcs, so it's obvious that
// we're not colliding with libc.
// But we want to link -lcrypto's SHA256, which depends on these
// names.

#include "LiteLib.h"

#if STRING_COMPAT_WANT_MEMCPY
void *memcpy(void *dst, const void *src, size_t n)
{
	lite_memcpy(dst, src, n);
	return dst;
}
#endif // STRING_COMPAT_WANT_MEMCPY

void *memset(void *start, int c, int size)
{
	lite_memset(start, c, size);
	return start;
}

void *memchr(const void *start, int c, size_t n)
{
	return lite_memchr(start, c, n);
}
