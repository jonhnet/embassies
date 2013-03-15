#include <string.h>
#include <linux/unistd.h>

#include "gsfree_lib.h"
#include "gsfree_syscall.h"
#include "xax_util.h"
#include "cheesy_snprintf.h"


void __gsfree_perf(const char* msg) {
	char buf[500];
	struct timeval t;
	memset(&t, 0, sizeof(struct timeval));
	memset(buf, 0, sizeof(buf));
	_gsfree_syscall(__NR_gettimeofday, &t, NULL);
	cheesy_snprintf(buf, sizeof(buf), "%d.%03d %s", (int) t.tv_sec, (int) t.tv_usec/1000, msg);
	safewrite_str(buf);
}

void __gsfree_assert(const char *file, int line, const char *cond)
{
	char buf[500];
	strcpy(buf, "assertion failed @");
	strcat(buf, file);
	strcat(buf, ":");
	char ibuf[50];
	_gsfree_itoa(ibuf, line);
	strcat(buf, ibuf);
	strcat(buf, ": \"");
	strcat(buf, cond);
	strcat(buf, "\"\n");
	_gsfree_syscall(__NR_write, 2, buf, strlen(buf));
	__asm__("int $0x3");
	__asm__("int $0x3");
	char *x = NULL;
	*x = 'a';
}

void _gsfree_itoa(char *buf, int val)
{
	if (val==0)
	{
		strcpy(buf, "0");
		return;
	}
	if (val<0)
	{
		buf[0] = '-';
		buf++;
		val = -val;
	}
	int digits=0;
	int valcopy;
	for (valcopy=val; valcopy>0; valcopy=valcopy/10)
	{
		digits += 1;
	}

	int digiti;
	valcopy = val;
	for (digiti = digits-1; digiti>=0; digiti--)
	{
		buf[digiti] = '0'+(valcopy%10);
		valcopy = valcopy / 10;
	}
	buf[digits] = '\0';
}

void safewrite_str(const char *str)
{
	_gsfree_syscall(__NR_write, 2, str, strlen(str));
}

void safewrite_hex(uint32_t val)
{
	char buf[20];
	buf[0] = '0';
	buf[1] = 'x';
	hexstr_08x(&buf[2], val);
	buf[10] = '\0';
	safewrite_str(buf);
}

void safewrite_dec(uint32_t val)
{
	char buf[20];
	utoa(buf, val);
	safewrite_str(buf);
}
