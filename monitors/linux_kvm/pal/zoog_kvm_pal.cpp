#include "pal_abi/pal_extensions.h"
#include "zoog_kvm_pal.h"
#include "KvmPal.h"

extern "C" { void kvm_start(); }

void kvm_start()
{
	KvmPal pal;
	ZoogDispatchTable_v1 *dispatch_table = pal.get_dispatch_table();
	
#if 0
	debug_logfile_append_f *debug_log = (debug_logfile_append_f *)
		(dispatch_table->xax_lookup_extension)(DEBUG_LOGFILE_APPEND_NAME);
	if (debug_log != NULL)
	{
		(debug_log)("stderr", "It's aalliiiiive!\n");
	}
#endif

	void *app_code_start = pal.get_app_code_start();

	__asm__(
		"movl	%0,%%ebx;"
		"nop;"
		"movl	%1,%%eax;"
		"jmp	*%%eax;"
		:	// out
		:	"r"(dispatch_table), // in
			"r"(app_code_start)
		);
}
