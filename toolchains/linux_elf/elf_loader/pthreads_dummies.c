/*
 These dummy funcs I created in the process of trying to get exceptions
 working in the elf_loader context. I stubbed these out because we don't
 actually want to call pthreads (with its attendant TLS refs) from
 the elf_loader zone; that can't end well.

 As of this writing, we ended up not finishing that task, because
 something was failing in the bowels of the unwind code. So these don't
 really do anything useful right now, since exceptions still explode.
*/

void __pthread_mutex_lock() {}
void pthread_mutex_lock() {}
void __pthread_mutex_unlock() {}
void pthread_mutex_unlock() {}
