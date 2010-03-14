/*
 * Copyright (C) 2009,2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_NCR_TREE_HPP
#define	_RESCACHED_NCR_TREE_HPP	1

#include "NCR.hpp"

using vos::DNSQuery;

namespace rescached {

enum _rbt_color {
	RBT_IS_BLACK	= 0,
	RBT_IS_RED	= 1
};

class NCR_Tree {
public:
	NCR_Tree();
	~NCR_Tree();

	int cmp(NCR *ncr);
	NCR_Tree* search_record_name(Buffer *name);

	void prune();
	void dump();
	void dump_tree(const int t);

	static int RBT_INSERT(NCR_Tree **root, NCR_Tree *node);
	static NCR_Tree * RBT_REMOVE(NCR_Tree **root, NCR_Tree *node);

	int		_color;
	NCR		*_rec;
	NCR_Tree	*_left;
	NCR_Tree	*_right;
	NCR_Tree	*_top;
	void		*_p_list;
private:
	NCR_Tree(const NCR_Tree&);
	void operator=(const NCR_Tree&);
};

} /* namespace::rescached */

#endif