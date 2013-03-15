
.global elfmain
.type elfmain,@function

.text
.global _elfstart
	.type	_elfstart, @function
_elfstart:
	// reloc offset already on the stack from preamble (push)
	// elfbase already on the stack from preamble (call)
	push	%ebx	/* xdt from PAL */

	call	elfmain
	hlt

.Lstart:
	.size	 _elfstart,.Lstart-_elfstart

