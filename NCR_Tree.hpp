/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_NCR_TREE_HPP
#define	_RESCACHED_NCR_TREE_HPP	1

#include "NCR.hpp"

using vos::Error;
using vos::DNSQuery;

namespace rescached {

class NCR_Tree {
public:
	NCR_Tree(NCR *record = NULL);
	~NCR_Tree();

	int cmp(Buffer *bfr);
	int insert(NCR_Tree *node);
	void insert_record(NCR *record);
	void prune();
	void dump();

	int		_color;
	NCR		*_rec;
	NCR_Tree	*_left;
	NCR_Tree	*_right;
	NCR_Tree	*_top;
private:
	DISALLOW_COPY_AND_ASSIGN(NCR_Tree);
};

} /* namespace::rescached */

#endif
