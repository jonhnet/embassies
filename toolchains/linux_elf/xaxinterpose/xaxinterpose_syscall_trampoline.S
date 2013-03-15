.global xaxinterpose_syscall_dispatch
.type xaxinterpose_syscall_dispatch,@function

.text
.global xaxinterpose_syscall_trampoline
.type xaxinterpose_syscall_trampoline,@function
	push %eax
	push %ebx
	push %ecx
	push %edx
	push %esi
	push %edi

	call xaxinterpose_syscall_dispatch
	addl	$0x18, %esp
	ret
