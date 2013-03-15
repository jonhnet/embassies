#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "standard_malloc_factory.h"
#include "corefile.h"

/*
* Using this demo:
* 
* make	# builds & runs core_file_test, generating "build/my-core"
* gdb build/core_file_test build/my-core
* 
* In gdb, you'll notice that $eip has been set to "label:"
* (see regs.ip below):
* 
* 	#0  coredump () at core_file_test.c:70
* 	70		regs.ip = (uint32_t) &&label;
* 
* And eax has been set to a demo value:
* 
* 	(gdb) p ((char*)$eax)
* 	$8 = 0x8049d33 "hi, mom!"
* 
* And, since we stored the real $ebp (and had an $eip in a post-prologue
* part of the function), we get a sane stack trace:
* 
* 	(gdb) where
* 	#0  coredump () at core_file_test.c:70
* 	#1  0x08048961 in baz () at core_file_test.c:118
* 	#2  0x0804896e in bar () at core_file_test.c:119
* 	#3  0x0804897b in foo () at core_file_test.c:120
* 	#4  0x08048988 in main () at core_file_test.c:124
* 
*/

static uint32_t decode_nybble(char v)
{
	if (v>='0' && v<='9') { return v-'0'; }
	if (v>='a' && v<='f') { return v-'a'+10; }
	assert(0);
}

static uint32_t decode_hex(char *buf)
{
	uint32_t val = 0;
	while (buf[0]!='\0')
	{
		val = (val<<4) | decode_nybble(buf[0]);
		buf++;
	}
	return val;
}

void coredump()
{
	// How to generate a core dump:

	// Declare a CoreFile object.
	CoreFile c;

	// Create and populate a structure capturing registers.
	// (if you don't know 'em, leave it empty.)
	Core_x86_registers regs;
	memset(&regs, 0, sizeof(regs));
#if 0 // jonh debugging
	int j;
	for (j=0; j<sizeof(regs); j++)
	{
		((char*)&regs)[j] = j;
	}
#endif


	MallocFactory *mf = standard_malloc_factory_init();
	// Initialize the corefile structure, passing your registers object.
	corefile_init(&c, mf);

	// more jonh debugging: put some recognizable values in a couple
	// registers, for the test.
label:
	regs.ip = (uint32_t) &&label;
	char *msg = "hi, mom!";
	regs.ax = (uint32_t) msg;

	uint32_t ebp;
	__asm__("movl %%ebp,%0" : "=m"(ebp) : /**/ : "%eax");
	regs.bp = ebp;
	regs.sp = ebp-8;
	corefile_add_thread(&c, &regs, 17);

	// Call corefile_add_segment once for each memory region
	// you know about. (Please, oh please, include the stack. :v)
	// Everything in this block is linux extraction code for this
	// demo, except the actual corefile_add_segment() line.
	FILE *mfp = fopen("/proc/self/maps", "r");
	while (1)
	{
		char linebuf[200];	// I <3 buffer overfloze
		char *rc;
		rc = fgets(linebuf, sizeof(linebuf), mfp);
		if (rc==NULL)
		{
			break;
		}
		assert(strlen(linebuf)>20);
		linebuf[8] = '\0';
		linebuf[17] = '\0';
		if (linebuf[18]!='r')
		{
			continue;
		}
		uint32_t start = decode_hex(&linebuf[0]);
		uint32_t end = decode_hex(&linebuf[9]);

		// The interesting line.
		// Notice that, since I'm slurping out my own memory, vaddr==bytes.
		// But 'bytes' is where CoreFile will look to read the bytes;
		// vaddr is where they appear in the virtual memory map of the
		// target program.
		corefile_add_segment(&c, (uint32_t) start, (void*) start, end-start);
		//fprintf(stderr, "added segment %08x %08x\n", start, end-start);
	}

	// add some dummy segments to test header stretching
	char dummy[0x1000];
	int i;
	for (i=0; i<200; i++)
	{
		corefile_add_segment(&c, (uint32_t) 0xc0000000+0x1000*i, dummy, 0x1000);
	}

	corefile_set_bootblock_info(&c, (void*) 0xabcdef01, "fake/path");

	// Open an output file and call corefile_write to emit the core file.
	FILE *fp = fopen("build/my-core", "w");
	corefile_write(fp, &c);
	fclose(fp);
}

void baz() { coredump(); }
void bar() { baz(); }
void foo() { bar(); }

int main()
{
	foo();
	return 0;
}
