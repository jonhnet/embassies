#include "bogus_ebp_stack_sentinel.h"

.globl _gsfree_syscall
.type _gsfree_syscall,function
_gsfree_syscall:
	push   %edi
	push   %esi
	push   %ebx
	mov    0x24(%esp),%edi
	mov    0x20(%esp),%esi
	mov    0x1c(%esp),%edx
	mov    0x18(%esp),%ecx
	mov    0x14(%esp),%ebx
	mov    0x10(%esp),%eax
	int    $0x80
	pop    %ebx
	pop    %esi
	pop    %edi
	ret

# The sixth argument horques the frame pointer chain, so we only use this
# call if we actually need all six args.
.globl _gsfree_syscall6
.type _gsfree_syscall6,function
_gsfree_syscall6:
	push   %edi
	push   %esi
	push   %ebx
	push   %ebp
	push   $BOGUS_EBP_STACK_SENTINEL
	mov    0x30(%esp),%ebp
	mov    0x2c(%esp),%edi
	mov    0x28(%esp),%esi
	mov    0x24(%esp),%edx
	mov    0x20(%esp),%ecx
	mov    0x1c(%esp),%ebx
	mov    0x18(%esp),%eax
	int    $0x80
	pop    %ebx		# BOGUS_EBP_STACK_SENTINEL
	pop    %ebp
	movl   $0,-8(%esp)	# clear sentinel to avoid accidental confusion
	pop    %ebx
	pop    %esi
	pop    %edi
	ret

