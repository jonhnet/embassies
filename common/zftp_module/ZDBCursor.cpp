#include <string.h>
#include <assert.h>

#include "ZDBCursor.h"

ZDBCursor::ZDBCursor(ZDB *zdb)
{
	this->dbc = NULL;
	int rc = zdb->db->cursor(zdb->db, NULL, &this->dbc, 0);
	assert(rc==0);
}

ZDBCursor::~ZDBCursor()
{
	if (dbc!=NULL)
	{
		int rc = dbc->c_close(dbc);
		assert(rc==0);
	}
}

void ZDBCursor::_blank_dbt(DBT *dbt)
{
	memset(dbt, 0, sizeof(*dbt));
	dbt->flags = DB_DBT_MALLOC;
}

bool ZDBCursor::get(ZDBDatum **k, ZDBDatum **v)
{
	while (1)
	{
		DBT key_dbt;
		_blank_dbt(&key_dbt);
		DBT value_dbt;
		_blank_dbt(&value_dbt);
		int rc = dbc->c_get(dbc, &key_dbt, &value_dbt, DB_NEXT);
		if (rc==DB_NOTFOUND)
		{
			assert(key_dbt.data == NULL);
			assert(value_dbt.data == NULL);
			return false;
		}
		assert(rc==0);
		ZDBDatum *myk = new ZDBDatum((uint8_t*) key_dbt.data, key_dbt.size, true);
		ZDBDatum *myv = new ZDBDatum((uint8_t*) value_dbt.data, value_dbt.size, true);
		if (myk->cmp(ZFTPIndexConfig::get_config_key())==0)
		{
			// hide the config key; loop around.
			delete myk;
			delete myv;
			continue;
		}
		*k = myk;
		*v = myv;
		return true;
	}
}
