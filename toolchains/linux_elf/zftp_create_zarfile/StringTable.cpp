#include <malloc.h>
#include <string.h>

#include "StringTable.h"

class StringRecord {
private:
	int len;
	char *buf;
public:
	StringRecord(const char *url);
	~StringRecord();
	int get_size() { return len; }
	const char *get_data() { return buf; }
};

StringRecord::StringRecord(const char *url)
{
	len = strlen(url)+1;
	buf = (char*) malloc(len);
	memcpy(buf, url, len);
}

StringRecord::~StringRecord()
{
	free(buf);
	buf = NULL;
}

//////////////////////////////////////////////////////////////////////////////

StringTable::StringTable(MallocFactory *mf)
{
	total_size = 0;
	linked_list_init(&strings, mf);
}

StringTable::~StringTable()
{
	while (strings.count>0)
	{
		StringRecord *sr = (StringRecord*) linked_list_remove_head(&strings);
		delete sr;
	}
}

uint32_t StringTable::get_size()
{
	return total_size;
}

uint32_t StringTable::allocate(const char *url)
{
	uint32_t string_index = total_size;
	StringRecord *sr = new StringRecord(url);
	linked_list_insert_tail(&strings, sr);
	total_size += sr->get_size();
	return string_index;
}

const char* StringTable::get_type()
{
	return "StringTable";
}

void StringTable::emit(FILE *fp)
{
	LinkedListIterator lli;
	for (ll_start(&strings, &lli); ll_has_more(&lli); ll_advance(&lli))
	{
		StringRecord *sr = (StringRecord *) ll_read(&lli);
		int rc = fwrite(sr->get_data(), sr->get_size(), 1, fp);
		lite_assert(rc==1);
	}
}
