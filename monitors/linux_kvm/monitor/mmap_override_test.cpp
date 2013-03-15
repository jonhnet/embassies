#include <malloc.h>
#include "MmapOverride.h"

int main()
{
	MmapOverride mmapOverride;
	
	for (int i=0; i<10000; i++)
	{
		malloc(0x8000);
	}
}
