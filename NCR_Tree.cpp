/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "NCR_Tree.hpp"

namespace rescached {

NCR_Tree::NCR_Tree() :
	_color(0),
	_rec(NULL),
	_left(NULL),
	_right(NULL),
	_top(NULL)
{}

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
 * @desc: compare record on node with record 'r'.
 * @param:
 *	> r	: record to compare to.
 * @return:
 *	< 1	: this > bfr.
 *	< 0	: this == bfr.
 *	< -1	: this < bfr.
 */
int NCR_Tree::cmp(NCR *ncr)
{
	if (!_rec) {
		return -1;
	}
	if (!_rec->_name) {
		return -1;
	}
	return _rec->_name->cmp(ncr->_name);
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

	if (!node)
		return 0;

	if (!_rec) {
		_rec		= node->_rec;
		node->_rec	= NULL;
		return 1;
	}

	while (p) {
		s = p->cmp(node->_rec);
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

/**
 * @desc	: insert name-cache record to the tree.
 *
 * @param	:
 *	> record: name-cache record object.
 *
 * @return	:
 *	< 0	: success.
 *	< <0	: fail.
 */
int NCR_Tree::insert_record(NCR *record)
{
	int		s;
	NCR_Tree	*node = NULL;

	if (!record)
		return 0;

	node = new NCR_Tree();
	if (!node) {
		return -vos::E_MEM;
	}

	node->_rec = record;

	s = insert(node);
	if (s != 0) {
		delete node;
	}

	return 0;
}

void NCR_Tree::remove_record(NCR *record)
{
	int		s	= 0;
	NCR_Tree	*p	= this;
	NCR_Tree	*x	= NULL;
	NCR_Tree	*l	= NULL;
	NCR_Tree	*r	= NULL;

	if (!record)
		return;

	while (p) {
		s = p->cmp(record);
		if (s < 0) {
			p = p->_left;
		} else if (s > 0) {
			p = p->_right;
		} else {
			if (p == this) {
				if (p->_left) {
					x		= p->_left;
					p->_rec		= x->_rec;
					l		= x->_left;
					r		= x->_right;
					p->_left	= NULL;
				} else if (p->_right) {
					x		= p->_right;
					p->_rec		= x->_rec;
					l		= x->_left;
					r		= x->_right;
					p->_right	= NULL;
				} else {
					p->_rec = NULL;
					break;
				}
			} else {
				x	= p;
				p	= p->_top;
				l	= x->_left;
				r	= x->_right;

				if (p->_left == x)
					p->_left = NULL;
				else
					p->_right = NULL;
			}

			x->_rec		= NULL;
			x->_top		= NULL;
			x->_left	= NULL;
			x->_right	= NULL;

			if (l) {
				l->_top	= NULL;
				p->insert(l);
			}
			if (r) {
				r->_top	= NULL;
				p->insert(r);
			}

			delete x;
			break;
		}
	}
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
