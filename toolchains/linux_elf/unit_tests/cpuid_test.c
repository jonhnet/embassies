#include <sys/time.h>
#include <string.h>
#include <malloc.h>

#define NUMBUFS	1
#define BUFSIZE	(140*1024*1024)

int main()
{
	int flags = 0;
	asm (
		 "movl $0x1, %%eax;"
		 "pushl %%ebx;"
		 "cpuid;"
		 "popl %%ebx;"
		 "movl %%edx, %0;"
		 : "=g" (flags)
		 :
		 : "%eax", "%ecx", "%edx"
		 );
	fprintf(stderr, "cpuid flags (edx) == %08x\n", flags);
	fprintf(stderr, "in particular, bit 26 %d\n", (flags&(1<<26))?1:0);
	return 0;
}
