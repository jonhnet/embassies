// A test program to demonstrate the pie init-twice bug.

#include <stdio.h>
#include <unistd.h>

static int init_count = 0;

void __attribute__ ((constructor)) initfoo(void)
{
	write(1, "ctor\n", 5);
	init_count += 1;
}

int main()
{
	printf("init_count is %d\n", init_count);
	return init_count==1 ? 0 : -1;
}
