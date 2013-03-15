#include <alloca.h>
#include <stdio.h>

#include "cheesy_snprintf.h"

void test(int i, int s_len)
{
	char *str = alloca(s_len);
	cheesy_snprintf(str, s_len,
		"i is %4d (hex %x) tilde %x floating %.2f frac %.4f\n",
		i, i, ~i,
		((double) i),
		2.5 - i/3.0);
	printf("*%s*\n", str);
}

void testf(int width, double v, char *str)
{
	char fmt[60];
	snprintf(fmt, sizeof(fmt), "default %%f smallish %%0.%df %%s\n", width);
	char buf[60];
	cheesy_snprintf(buf, sizeof(buf), fmt, v, v, str, 4, 5, 6);
	printf("#%s#", buf);
}

int main()
{
	testf(3, 0.000030000, "0.000030000");

	int i;
	for (i=0; i<20; i++)
	{
		test(i, 3*i+1);
		test(i, 100);
	}

	for (i=0; i<8; i++)
	{
		testf(i, 0.000030000, "0.000030000");
		testf(i, 0.000030001, "0.000030001");
		testf(i, 0.600030000, "0.600030000");
		testf(i, 0.600030001, "0.600030001");
		testf(i, 1.600030000, "1.600030000");
		testf(i, 1.600030001, "1.600030001");
	}

	return 0;
}
