#include "debug.h"
#include "debug_util.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include "cheesylock.h"
#include "debug_hash.h"
#include "cheesy_snprintf.h"

extern int _xaxunder_write(int fd, const char *buf, int len);

void
debug(const char *msg, ...)
{
#ifdef DEBUG
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
#endif // DEBUG
#ifdef XAX_CHEESY
	write(2, msg, strlen(msg));
#endif
}

void
debug_hash(hash_t hash)
{
#ifdef DEBUG
	int i;
	uint8_t *raw = hash.bytes;
	for(i = 0; i < sizeof(hash_t); i++)
		debug("%02X", raw[i] & 0xFF);
#endif
}

void
debug_print_packet(uint8_t *buf, uint32_t len)
{
// can't call printf inside ld.so; causes multiple-definition of itoa
#ifdef DEBUG
	FILE *fp = fopen("debug.packet", "w");
	int i;
	for (i=0; i<len; i++)
	{
		fprintf(fp, "%02x ", buf[i]);
		if (i==len-1)
		{
			fprintf(fp, "(%d bytes)", len);
		}
		if (((i+1)& 0x0f)==0)
		{
			fprintf(fp, "\n");
		}
	}
	if (((i+1)& 0x0f)!=0)
	{
		fprintf(fp, "\n");
	}
	fclose(fp);
#endif //DEBUG
}

void debug_write_sync(const char *msg)
{
#if STANDALONE
	fprintf(stderr, "%s", msg);
#else // STANDALONE
#if 0
	static CheesyLock write_sync_lock;
	static bool lock_inited = false;
	// Gee, hope we don't race on init...
	if (!lock_inited)
	{
		cheesy_lock_init(&write_sync_lock);
		lock_inited= true;
	}

	cheesy_lock_acquire(&write_sync_lock);
	_xaxunder_write(2, msg, strlen(msg));
	cheesy_lock_release(&write_sync_lock);
#endif
#endif // STANDALONE
}
