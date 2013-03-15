#include <malloc.h>
#include <string.h>

#include "standard_malloc_factory.h"
#include "CatalogEntry.h"
#include "ZFSReader.h"

CatalogEntry::CatalogEntry(const char *url, bool present, uint32_t len, uint32_t flags, FileSystemView *file_system_view)
{
	this->url = strdup(url);
	phdr = new PHdr(len, flags);
	this->present = present;
	this->file_system_view = file_system_view;
}

CatalogEntry::~CatalogEntry()
{
	free(url);
	delete phdr;
}

PHdr *CatalogEntry::take_phdr()
{
	PHdr *result = phdr;
	phdr = NULL;
	return result;
}

int CatalogEntry::cmp_by_url(const void *v_a, const void *v_b)
{
	CatalogEntry **a = (CatalogEntry **) v_a;
	CatalogEntry **b = (CatalogEntry **) v_b;
	return strcmp((*a)->get_url(), (*b)->get_url());
}
