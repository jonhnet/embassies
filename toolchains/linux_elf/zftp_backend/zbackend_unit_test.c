#include <assert.h>
#include "zftp_protocol.h"
#include "tree_math.h"

uint32_t _hash_index(int level, int col)
{
	assert(col < (1<<level));
	if (level==0)
	{
		return 0;
	}
	return compute_tree_size(level-1)+col;
}

int main()
{
	int nb;
	for (nb=0; nb<20; nb++)
	{
		int td = compute_tree_depth(nb);
		int ts = compute_tree_size(td);
		fprintf(stdout, "nb %2d td %2d ts %2d hi %2d\n", nb, td, ts, _hash_index(td,0));
	}

	int loc;
	for (loc=0; loc<20; loc++)
	{
		int level;
		int col;
		_tree_unindex(loc, &level, &col);
		uint32_t reindex = _tree_index(level, col, 20);
		fprintf(stdout, "loc %2d level %2d col %2d reindex %2d parent %2d sibling %d left_child %2d\n",
			loc,
			level,
			col,
			reindex,
			_parent_location(loc),
			(loc>0 ? _sibling_location(loc) : -1),
			_left_child(loc));
		assert(reindex==loc);
		assert(_parent_location(_left_child(loc))==loc);
	}
	return 0;
}
