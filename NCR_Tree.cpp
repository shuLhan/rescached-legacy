/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "NCR_Tree.hpp"

namespace rescached {

NCR_Tree::NCR_Tree(NCR *record) :
	_color(0),
	_rec(NULL),
	_left(NULL),
	_right(NULL),
	_top(NULL)
{
	if (record)
		_rec = record;
}

NCR_Tree::~NCR_Tree()
{
	if (_left) {
		delete _left;
		_left = NULL;
	}

	if (_rec)
		delete _rec;

	if (_right) {
		delete _right;
		_right = NULL;
	}
}

/**
 * @desc: compare record on node with buffer 'bfr'.
 * @param:
 *	> bfr	: buffer to compare to.
 * @return:
 *	< 1	: this > bfr.
 *	< 0	: this == bfr.
 *	< -1	: this < bfr.
 */
int NCR_Tree::cmp(Buffer *bfr)
{
	if (!_rec) {
		return -1;
	}
	if (!_rec->_name) {
		return -1;
	}
	return _rec->_name->cmp(bfr);
}

/**
 * @desc: insert node into tree.
 *
 * @param:
 *	> node : NCR_Tree object.
 *
 * @return:
 *	< 0	: success.
 *	< 1	: success, root is nil and we only keep the record data.
 */
int NCR_Tree::insert(NCR_Tree *node)
{
	int		s;
	NCR_Tree	*p = this;

	if (! node)
		return 0;

	if (! _rec) {
		_rec		= node->_rec;
		node->_rec	= NULL;

		return 1;
	}

	while (p) {
		s = p->cmp(node->_rec->_name);
		if (s < 0) {
			if (! p->_left) {
				p->_left	= node;
				node->_top	= p;
				break;
			} else {
				p = p->_left;
			}
		} else {
			if (! p->_right) {
				p->_right	= node;
				node->_top	= p;
				break;
			} else {
				p = p->_right;
			}
		}
	}

	return 0;
}

void NCR_Tree::insert_record(NCR *record)
{
	int		s;
	NCR_Tree	*node = NULL;

	if (! record)
		return;

	node = new NCR_Tree(record);
	if (! node)
		throw Error(vos::E_MEM);

	s = insert(node);
	if (s)
		delete node;
}

void NCR_Tree::prune()
{
	if (_left)
		_left->prune();
	_rec = NULL;
	if (_right)
		_right->prune();
}

void NCR_Tree::dump()
{
	if (_left) {
		_left->dump();
	}
	if (_rec) {
		printf("%6d|%s\n", _rec->_stat, _rec->_name->_v);
	}
	if (_right) {
		_right->dump();
	}
}

} /* namespace::rescached */
