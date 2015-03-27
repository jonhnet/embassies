#pragma once

class CatalogEntryByUrl {
private:
	CatalogEntry *ce;

public:
	CatalogEntryByUrl(CatalogEntry *ce) : ce(ce) {}
	CatalogEntryByUrl getKey() { return *this; }
	CatalogEntry* get_catalog_entry() { return ce; }

	bool operator<(CatalogEntryByUrl b)
		{ return strcmp(ce->get_url(), b.ce->get_url())<0; }
	bool operator>(CatalogEntryByUrl b)
		{ return strcmp(ce->get_url(), b.ce->get_url())>0; }
	bool operator==(CatalogEntryByUrl b)
		{ return strcmp(ce->get_url(), b.ce->get_url())==0; }
	bool operator<=(CatalogEntryByUrl b)
		{ return strcmp(ce->get_url(), b.ce->get_url())<=0; }
	bool operator>=(CatalogEntryByUrl b)
		{ return strcmp(ce->get_url(), b.ce->get_url())>=0; }
};

typedef AVLTree<CatalogEntryByUrl,CatalogEntryByUrl> EntryTree;
