#include "malloc_factory.h"
#include "LiteLib.h"
#include "math_util.h"

char *mf_strdup(MallocFactory *mf, const char *str)
{
	int size = lite_strlen(str)+1;
	char *result = (char*)(mf->c_malloc)(mf, size);
	lite_memcpy(result, str, size);
	return result;
}

char *mf_strndup(MallocFactory *mf, const char *str, int max_len)
{
	int size = min(max_len, lite_strlen(str))+1;
	char *result = (char*)(mf->c_malloc)(mf, size);
	lite_memcpy(result, str, size);
	result[size-1] = '\0';
	return result;
}

void *mf_malloc_zeroed(MallocFactory *mf, size_t size)
{
	void *buffer = (mf->c_malloc)(mf, size);
	lite_memset(buffer, (unsigned char) 0, size);
	return buffer;
}

void *mf_realloc(MallocFactory *mf, void *old_ptr, size_t old_size, size_t new_size)
{
	if (old_size==0)
	{
		if (new_size==0)
		{
			lite_assert(0); // you're kidding, right?
		}
		lite_assert(old_ptr==NULL);
		return mf_malloc(mf, new_size);
	}

	if (new_size==0)
	{
		mf_free(mf, old_ptr);
		return NULL;
	}

	void *new_ptr = mf_malloc(mf, new_size);
	size_t copy_size = new_size;
	if (copy_size > old_size)
	{
		copy_size = old_size;
	}
	lite_memcpy(new_ptr, old_ptr, copy_size);
	mf_free(mf, old_ptr);
	return new_ptr;
}
