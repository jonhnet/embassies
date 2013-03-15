#include <linux/unistd.h>

.globl _raw_clone
.type _raw_clone,function
_raw_clone:
	push   %ebp
# arg1 : ebx : flags
# arg2 : ecx : stack
	mov    0x08(%esp),%ebx
	mov    0x0c(%esp),%ecx
	mov    0x10(%esp),%edx
	mov    $__NR_clone,%eax
	int    $0x80
	test   %eax,%eax
	jne    parent
# child
	# we can't terminate the frame stack (since we can't write a
	# zero onto the stack; the guest owns every byte), but we
	# can at least put it in the right neighborhood.
	movl   %esp,%ebp
	jmp    *%edx
parent:
	pop		%ebp
	ret

