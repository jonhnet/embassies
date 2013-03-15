/*
This test is here to try to understand why pthread_attr_setstacksize
doesn't always stick. Debugging shows that it's because we're binding to
pthread_create_2_0 instead of 2_1, and 2_0 version assumes the attr struct
has no stacksize info. That version binding is handled via some disgusting
linker symbol gymnastics deep in the bowels of libc that no sane man should
see, should he wish to remain sane.

This test helped me learn that -pie seems to be enough to cause the problem.
That is, it's not the zoog environment that's confusing things, it's that
the linker (ld.so) seems to make the mistake when the referrer is a -pie,
but not when it's an ordinary executable.

At this point, I decided to stop causing self-injury, and instead just
decided to handle the stack size problem in accounting, rather than actually
fix it.
*/
#include <pthread.h>

void *func(void *arg)
{
	return 0;
}

int main()
{
  pthread_t pt;

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 128<<10);

  pthread_create(&pt,
                        NULL /*&attr*/, 
                        func, 
                        NULL);

  pthread_attr_destroy(&attr);
  return 69;
}
