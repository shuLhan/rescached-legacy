/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_NCR_TREE_HPP
#define	_RESCACHED_NCR_TREE_HPP	1

#include "main.hpp"
#include "NCR.hpp"

using vos::DNSQuery;

namespace rescached {

class NCR_Tree {
public:
	NCR_Tree();
	~NCR_Tree();

	int cmp(NCR *ncr);
	NCR_Tree* search_record_name(Buffer *name);

	void prune();
	void dump();
	void dump_tree(const int t);

	static NCR_Tree* REBUILD(NCR_Tree *root, NCR_Tree *node);

	static void INSERT(NCR_Tree **root, NCR_Tree *node);

	static NCR_Tree* REMOVE(NCR_Tree **root, NCR_Tree *node);
	static NCR_Tree* REMOVE_RECORD(NCR_Tree **root, NCR *record);

	int		_color;
	NCR		*_rec;
	NCR_Tree	*_left;
	NCR_Tree	*_right;
	NCR_Tree	*_top;
	void		*_p_list;
private:
	DISALLOW_COPY_AND_ASSIGN(NCR_Tree);
};

} /* namespace::rescached */

#endif
