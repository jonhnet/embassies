#include <errno.h>	// EACCES
#include "DecomposePath.h"
#include "LiteLib.h"

DecomposePath::DecomposePath(MallocFactory *mf, const char *in_path)
{
	this->mf = mf;
	err = XFS_NO_ERROR;
	prefix = mf_strdup(mf, in_path);
	tail = lite_rindex(prefix, '/');
	if (tail==NULL)
	{
		// sorry, mkdir doesn't work in root, or cwd :v)
		err = (XfsErr) EACCES;
	}
	else
	{
		tail[0] = '\0';
		tail += 1;
	}
}

DecomposePath::~DecomposePath()
{
	mf_free(mf, prefix);
}
