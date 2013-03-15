#include "thread_table.h"
#include "xax_util.h"

void _thread_table_clear(ThreadTable *tt, int start)
{
	int j;
	for (j=0, j=start; j<tt->capacity; j++)
	{
		tt->entries[j].tsb = NULL;
		tt->entries[j].tid = -1;
	}
}

void thread_table_init(ThreadTable *tt, MallocFactory *mf, ZoogDispatchTable_v1 *zdt)
{
	tt->mf = mf;
	tt->capacity = 10;
	tt->entries = mf_malloc(mf, sizeof(ThreadTableEntry)*tt->capacity);
	tt->pseudo_tid = 1;
	tt->zdt = zdt;
	zmutex_init(&tt->table_mutex);

	_thread_table_clear(tt, 0);
}

int _thread_table_find_lockheld(ThreadTable *tt, pid_t tid)
{
	int i;
	for (i=0; i<tt->capacity; i++)
	{
		if (tt->entries[i].tid==tid)
		{
			return i;
		}
	}
	return -1;
}

void _thread_table_expand_lockheld(ThreadTable *tt)
{
	int old_capacity = tt->capacity;
	int new_capacity = (tt->capacity+1) * 2;
	tt->entries = mf_realloc(tt->mf, tt->entries,
		sizeof(ThreadTableEntry)*tt->capacity,
		sizeof(ThreadTableEntry)*new_capacity);
	tt->capacity = new_capacity;
	_thread_table_clear(tt, old_capacity);
}

void thread_table_insert(ThreadTable *tt, pid_t tid, ThreadStartBlock *tsb)
{
	zmutex_lock(tt->zdt, &tt->table_mutex);

	xax_assert(_thread_table_find_lockheld(tt, tid)==-1);

	int i;
	for (i=0; i<tt->capacity; i++)
	{
		if (tt->entries[i].tsb==NULL)
		{
			break;
		}
	}
	if (i==tt->capacity)
	{
		_thread_table_expand_lockheld(tt);
	}
	xax_assert(tt->entries[i].tsb==NULL);
	tt->entries[i].tid = tid;
	tt->entries[i].tsb = tsb;

	zmutex_unlock(tt->zdt, &tt->table_mutex);
}

bool thread_table_remove(ThreadTable *tt, pid_t tid, ThreadStartBlock **out_tsb)
{
	zmutex_lock(tt->zdt, &tt->table_mutex);

	bool result;
	int i = _thread_table_find_lockheld(tt, tid);
	if (i==-1)
	{
		result = false;
	}
	else
	{
		(*out_tsb) = tt->entries[i].tsb;
		tt->entries[i].tid = -1;
		tt->entries[i].tsb = NULL;
		result = true;
	}

	zmutex_unlock(tt->zdt, &tt->table_mutex);
	return result;
}

int thread_table_allocate_pseudotid(ThreadTable *tt)
{
	int ptid = tt->pseudo_tid;
	tt->pseudo_tid += 1;
	return ptid;
}
