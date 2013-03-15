#include "LiteLib.h"
#include "pal_abi/pal_abi.h"
#include "pal_abi/pal_extensions.h"
#include "xax_posix_emulation.h"
#include "linux_syscall_ifc.h"
#include "xinterpose.h"
#include "bogus_ebp_stack_sentinel.h"

typedef struct {
	ZoogDispatchTable_v1 *xdt;
	debug_logfile_append_f *debug_log;
	XaxPosixEmulation xpe;
} Xinterpose;

static Xinterpose g_xint;

void xint_msg(char *s)
{
	(g_xint.debug_log)("stderr", s);
}

void xinterpose_init(ZoogDispatchTable_v1 *xdt, uint32_t stack_base, StartupContextFactory *scf)
{
	g_xint.xdt = xdt;
	g_xint.debug_log = (debug_logfile_append_f*) (g_xint.xdt->zoog_lookup_extension)("debug_logfile_append");
	xpe_init(&g_xint.xpe, xdt, stack_base, scf);
}

#define SAVE_USER_REGS(ur)                                                   \
    __asm__(                                                                 \
        "movl   %%eax,%0;"                                                   \
        "movl   %%ebx,%1;"      /* stow PIC %ebx out of the way */           \
        "movl   -0xc(%%ebp),%%eax;" /* go get the real ebx */                \
        "movl   %%eax,%2;"  /* store it where we can get it before syscall */\
        "movl   %%ecx,%3;"                                                   \
        "movl   %%edx,%4;"                                                   \
        "movl   %%esi,%5;"                                                   \
        "movl   %%edi,%6;"                                                   \
        "movl   0(%%ebp),%%eax;" /* go get the real $ebp */                  \
        "movl   %%eax,%7;"                                                   \
        "movl   4(%%ebp),%%eax;" /* go get caller's $eip */                  \
        "movl   %%eax,%8;"                                                   \
        : /*no output registers*/                                            \
        : "m"(ur.eax),                                                       \
          "m"(pic_tmp),                                                      \
          "m"(ur.ebx),                                                       \
          "m"(ur.ecx),                                                       \
          "m"(ur.edx),                                                       \
          "m"(ur.esi),                                                       \
          "m"(ur.edi),                                                       \
          "m"(ur.ebp),                                                       \
          "m"(ur.eip)                                                        \
        : "eax"                                                              \
        );                                                                   \

uint32_t dispatch_via_int80(UserRegs ur)
{
	uint32_t syscall_result;

	// put registers back exactly as we found them for an int 0x80
	// (I mean, except for $esp and $eip. We assume those are
	// never significant.)

	// We store ebp on the stack here, because if we stored it via
	// an "=m" reference to an auto variable, we couldn't restore it
	// later: the compiler would need the restored ebp to find it!
	// This trick won't work if the kernel uses our stack for something
	// else, e.g. signal delivery.
	__asm__("push	%ebp;");

	// NB the last step in RESTORE_USER_REGS sequence stomps %ebp
	// (it's the 6th syscall arg slot).
	// After that, we can't reference it until we restore it.
	RESTORE_USER_REGS(ur);
	__asm__("int $0x80;");

	__asm__("pop	%ebp;");

	__asm__(
		"movl	%%eax,%0;"
		: "=m"(syscall_result)
		: /* no input registers */
		: "%esp" /* no clobber */
		);
	
	return syscall_result;
}

void xinterpose(void)
{
	UserRegs ur;
	uint32_t pic_tmp;

	SAVE_USER_REGS(ur);

	//xint_msg("Doing a syscall.\n");

//	uint32_t syscall_result = dispatch_via_int80(ur);
	uint32_t syscall_result = xpe_dispatch(&g_xint.xpe, &ur);

	// Now put the PIC register back, in case we decide to do any C
	// post-processing after this point.
	__asm__(
		"movl	%0,%%ebx;"
		: /* don't tell compiler we're un-stomping PIC register */
		: "m"(pic_tmp)
		: "esp" /* no clobbers */
		);

	// put everything back how it came, except with result in $eax
	ur.eax = syscall_result;
	RESTORE_USER_REGS(ur);
}
