#include "LiteLib.h"

void lite_strcpy(char *dst, const char *src)
{
	const char *sp;
	char *dp;
	for (sp=src, dp=dst; *sp != '\0'; sp++,dp++)
	{
		*dp = *sp;
	}
	*dp = '\0';
}

void lite_strncpy(char *dst, const char *src, int n)
{
	const char *sp;
	char *dp;
	int ni;
	for (sp=src, dp=dst, ni=0; *sp != '\0' && ni<n; sp++,dp++,ni++)
	{
		*dp = *sp;
	}
	*dp = '\0';
}

size_t lite_strlen(const char *s)
{
	const char *p;
	size_t l = 0;
	for (p = s; *p!='\0'; p++)
	{
		l++;
	}
	return l;
}

size_t lite_strnlen(const char *s, size_t max)
{
	const char *p;
	size_t l = 0;
	for (p = s; *p!='\0' && l<max; p++)
	{
		l++;
	}
	return l;
}

const char *lite_strchrnul(const char *p, char c)
{
	const char *o;
	for (o=p; *o!=c && *o!='\0'; o++)
		{ }
	return o;
}

void lite_strcat(char *dst, const char *src)
{
	lite_strcpy(dst+lite_strlen(dst), src);
}

void lite_strncat(char *dst, const char *src, int max)
{
	int dstlen = lite_strlen(dst);
	lite_strncpy(dst+dstlen, src, max-dstlen);
}

int lite_strcmp(const char *s1, const char *s2)
{
	const char *p1 = s1;
	const char *p2 = s2;
	
	for (; ((*p1)!='\0' && (*p2)!='\0'); p1++, p2++)
	{
		int d = (*p1)-(*p2);
		if (d!=0)
		{
			return d;
		}

	}
	// one has terminated. Have both?
	int d = (*p1)-(*p2);
	return d;
}

char *lite_index(const char *s, int c)
{
	const char *p;
	for (p=s; (*p)!='\0'; p++)
	{
		if ((*p)==c)
		{
			return (char*) p;
		}
	}
	return NULL;
}
 
char *lite_rindex(const char *s, int c)
{
	const char *e = s+lite_strlen(s)-1;
	for (; e>=s; e--)
	{
		if ((*e)==c)
		{
			return (char*) e;
		}
	}
	return NULL;
}

bool lite_starts_with(const char *prefix, const char *str)
{
	size_t prefixlen = lite_strlen(prefix);
	if (lite_strlen(str) < prefixlen)
	{
		return false;
	}
	return lite_memcmp(str, prefix, prefixlen)==0;
}

bool lite_ends_with(const char *suffix, const char *str)
{
	int suffixlen = lite_strlen(suffix);
	int str_len = lite_strlen(str);
	if (str_len < suffixlen)
	{
		return false;
	}
	return lite_memcmp(str+str_len-suffixlen, suffix, suffixlen)==0;
}

int lite_split(const char* buf, char delim, const char** out_fields, int max_fields)
{
	int f = 0;
	while (f<max_fields)
	{
		out_fields[f] = buf;
		buf = lite_index(buf, delim);
		f++;
		if (buf==NULL) { break; }
		buf += 1;	// point after the delimiter.
	}
	return f;
}

void lite_memcpy(void *dst, const void *src, size_t n)
{
	char *dstc = (char *) dst;
	const char *srcc = (const char *) src;
	const char *sp;
	char *dp;
	for (sp=srcc, dp=dstc; sp<srcc+n; sp++,dp++)
	{
		*dp = *sp;
	}
}

void lite_memset(void *start, char val, int size)
{
	char *startc = (char*) start;
	char *p=0;
	for (p=startc; p<startc+size; p++)
	{
		*p = val;
	}
}

void lite_bzero(void *start, int size)
{
	lite_memset(start, 0, size);
}

int lite_memcmp(const void *s1, const void *s2, size_t n)
{
	const char *p1 = (const char *) s1;
	const char *p2 = (const char *) s2;
	size_t count;
	
	for (count=0; count<n; count++, p1++, p2++)
	{
		char v = (*p1)-(*p2);
		if (v!=0)
		{
			return v;
		}
	}
	return 0;
}

void *lite_memchr(const void *start, char val, size_t n)
{
	const char *p;
	for (p=(char*)start; n>0; p++, n--)
	{
		if ((*p) == val)
		{
			return (void *) p;
		}
	}
	return NULL;
}

bool lite_memequal(const char *start, char val, int size)
{
	const char *p, *end=start+size;
	for (p = start; p<end; p++)
	{
		if (*p!=val)
		{
			return false;
		}
	}
	return true;
}

uint32_t lite_atoi(const char *s)
{
	uint32_t v = 0;
	while (s[0]>='0' && s[0]<='9')
	{
		v = v*10 + (s[0]-'0');
		s++;
	}
	return v;
}
