#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#include <Windows.h>
#endif // _WIN32

#include <stdlib.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ZDB.h"
#include "ZDBCursor.h"

#define LOCKSUFFIX	".lock"
#define PIDSUFFIX	".pid"

ZDB::ZDB(ZFTPIndexConfig *zic, const char *index_dir, const char *name)
{
	int rc;

	assert(index_dir!=NULL);
#ifdef _WIN32
	rc = CreateDirectory(index_dir, NULL);
	assert(rc || GetLastError() == ERROR_ALREADY_EXISTS);
#else
	rc = mkdir(index_dir, 0755);
	assert(rc==0 || errno==EEXIST);
#endif // _WIN32
	
	filename = (char*) malloc(strlen(index_dir)+1+strlen(name)+1);
	sprintf(filename, "%s/%s", index_dir, name);
	lockfilename = (char*) malloc(strlen(filename)+strlen(LOCKSUFFIX)+1);
	sprintf(lockfilename, "%s%s", filename, LOCKSUFFIX);
	pidfilename = (char*) malloc(strlen(filename)+strlen(PIDSUFFIX)+1);
	sprintf(pidfilename, "%s%s", filename, PIDSUFFIX);

	lock();

	bool on_second_try = false;
	while (true)
	{
		rc = db_create(&db, NULL, 0);
		assert(rc==0);
		rc = db->open(db, NULL, filename, NULL, DB_BTREE, DB_CREATE, 0600);
		assert(rc==0);

		bool config_fresh = check_config(zic);
		if (config_fresh)
		{
			break;
		}

		assert(!on_second_try || config_fresh);

		fprintf(stderr, "Cache index config stale; wiping.\n");
		db->close(db, 0);
				
#ifdef _WIN32		
		rc = _unlink(filename);
#else
		rc = unlink(filename);
#endif //_WIN32
		assert(rc==0 /* can't delete index_dir */ );
		on_second_try = true;
	}
}

ZDB::~ZDB()
{
	db->close(db, 0);
	unlock();
	free(filename);
	free(lockfilename);
}

bool ZDB::check_config(ZFTPIndexConfig *zic)
{
	ZDBDatum *config_value = lookup(zic->get_config_key());
	bool result;
	if (config_value==NULL)
	{
		insert(ZFTPIndexConfig::get_config_key(), zic->get_config_value());
		result = true;
	}
	else
	{
		ZDBDatum *ref_value = zic->get_config_value();
		result = (config_value->cmp(ref_value)==0);
		delete ref_value;
		delete config_value;
	}
	return result;
}

bool ZDB::lock_inner()
{
#ifdef _WIN32
	int rc = CreateDirectory(lockfilename, NULL) ? 0 : 1;
#else
	int rc = mkdir(lockfilename, 0700);
#endif // _WIN32
	
	if (rc!=0)
	{
		return false;
	}

#ifdef _WIN32		
		rc = _unlink(pidfilename);
#else
		rc = unlink(pidfilename);
#endif //_WIN32
	
	FILE *fp = fopen(pidfilename, "w");
	if (fp==NULL)
	{
		lite_assert(false); // I didn't expect THAT to fail!
		return false;
	}
#ifdef _WIN32
	fprintf(fp, "%d\n", GetCurrentProcessId());
#else
	fprintf(fp, "%d\n", getpid());
#endif // _WIN32
	fclose(fp);
	return true;
}

void ZDB::lock()
{
	if (lock_inner())
	{
		return;
	}

	const char *reason;
	FILE *fp = fopen(pidfilename, "r");
	if (fp==NULL)
	{
		reason = "no pidfile";
		goto fail;
	}

	char buf[1000];
	char *res;
	res = fgets(buf, sizeof(buf), fp);
	if (res==NULL)
	{
		reason = "cannot read pidfile";
		goto fail;
	}
	fclose(fp);
	int pid;
	pid = atoi(res);

	int tries;
	for (tries=0; tries<3; tries++)
	{
		//fprintf(stderr, "Killin' pid %d\n", pid);
		int rc;
#ifdef _WIN32
		HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (proc != NULL) {
			rc = TerminateProcess(proc, SIGINT);
		} else {
			rc = -1;
			errno = ESRCH;
		}
#else
		rc = kill(pid, SIGINT);
#endif // _WIN32
		
		if (rc==-1 && errno==ESRCH)
		{
			// he's not even there. We can just clean up his lock.
//			fprintf(stderr, "removing stale lock %s\n", lockfilename);
#ifdef _WIN32
			rc = RemoveDirectory(lockfilename) ? 0 : 1;
#else
			rc = rmdir(lockfilename);
#endif // _WIN32
			break;
		}
		else
		{
			// wait for him to clean up his own mess.
			fprintf(stderr, "killed other zftp %d\n", pid);
			int wait_msec = 100;
#ifndef _WIN32
			struct timespec ts;
			ts.tv_sec = wait_msec/1000;
			int remainder = wait_msec - (ts.tv_sec * 1000);
			ts.tv_nsec = remainder*1000*1000;
			nanosleep(&ts, NULL);
#else
			Sleep(wait_msec);
#endif // _WIN32
			continue;
			// TODO add signal handler to clean up my own mess gracefully.
		}
	}

	if (lock_inner())
	{
		return;
	}

	reason = "second lock attempt failed, too";

fail:
	fprintf(stderr, "Cannot take lock %s (%s)\n", lockfilename, reason);
	assert(false);
}

void ZDB::unlock()
{
#ifdef _WIN32
	int	rc = RemoveDirectory(lockfilename) ? 0 : 1;
#else
	int rc = rmdir(lockfilename);
#endif // _WIN32
	
	assert(rc==0);
}

void ZDB::insert(ZDBDatum *key, ZDBDatum *value)
{
	int rc;
	rc = db->put(db, NULL, key->get_dbt(), value->get_dbt(), 0);
	assert(rc==0);
	db->sync(db, 0);	// TODO slow, surely
}

ZDBDatum *ZDB::lookup(ZDBDatum *key)
{
	DBT value_dbt;
	memset(&value_dbt, 0, sizeof(value_dbt));
	value_dbt.flags = DB_DBT_MALLOC;
	int rc;
	rc = db->get(db, NULL, key->get_dbt(), &value_dbt, 0);
	if (rc==DB_NOTFOUND)
	{
		assert(value_dbt.data == NULL);
		return NULL;
	}
	else
	{
		assert(value_dbt.data != NULL);
		ZDBDatum *result = new ZDBDatum((uint8_t*) value_dbt.data, value_dbt.size, true);
		return result;
	}
}

ZDBCursor *ZDB::create_cursor()
{
	return new ZDBCursor(this);
}

int ZDB::get_nkeys()
{
	int rc;

	DB_BTREE_STAT *stato = NULL;
	rc = db->stat(db, NULL, &stato, 0);
	if (rc!=0)
	{
		return -1;
	}
	return stato->bt_nkeys;
}
