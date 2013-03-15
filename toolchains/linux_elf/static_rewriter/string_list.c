#include <malloc.h>
#include <string.h>

#include "string_list.h"

void string_list_init(StringList *sl)
{
	sl->count = 0;
	sl->strings = malloc(0);
}

void string_list_add(StringList *sl, char *s)
{
	sl->count += 1;
	sl->strings = realloc(sl->strings, sizeof(char*)*sl->count);
	sl->strings[sl->count-1] = s;
}

bool string_list_contains(StringList *sl, const char *s)
{
	int i;
	for (i=0; i<sl->count; i++)
	{
		if (strcmp(sl->strings[i], s)==0)
		{
			return true;
		}
	}
	return false;
}
