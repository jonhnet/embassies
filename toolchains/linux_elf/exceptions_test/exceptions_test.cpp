#include <stdio.h>

class BlueException {
};

void foo()
{
	throw BlueException();
}

int main()
{
	try {
		foo();
	} catch (BlueException ex) {
		printf("Exception.\n");
	}
	return 0;
}
