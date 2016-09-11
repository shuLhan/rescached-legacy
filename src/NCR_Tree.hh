//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _RESCACHED_NCR_TREE_HH
#define _RESCACHED_NCR_TREE_HH 1

#include "NCR.hh"

using vos::DNSQuery;

namespace rescached {

enum _rbt_color {
	RBT_IS_BLACK	= 0
,	RBT_IS_RED	= 1
};

/**
 * @class		: NCR_Tree
 * @attr		:
 *	- _color	: a color of node.
 *	- _rec		: pointer to NCR object.
 *	- _left		: pointer to the left node.
 *	- _right	: pointer to the right node.
 *	- _top		: pointer to the parent node.
 *	- _p_list	: pointer to the same NCR object but in the list.
 * @desc		: A representation of cache in tree mode (a Red-Black Tree)
 */
class NCR_Tree {
public:
	NCR_Tree();
	~NCR_Tree();

	int cmp(NCR* ncr);
	NCR_Tree* search_record_name(Buffer* name);

	void prune();
	void dump();
	void dump_tree(const int t);

	int		_color;
	NCR*		_rec;
	NCR_Tree*	_left;
	NCR_Tree*	_right;
	NCR_Tree*	_top;
	void*		_p_list;

	static int RBT_INSERT(NCR_Tree** root, NCR_Tree* node);
	static NCR_Tree* RBT_REMOVE(NCR_Tree** root, NCR_Tree* node);
private:
	NCR_Tree(const NCR_Tree&);
	void operator=(const NCR_Tree&);
};

} /* namespace::rescached */

#endif
// vi: ts=8 sw=8 tw=78:
