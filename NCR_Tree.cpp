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
	_top(NULL),
	_p_list(NULL)
{}

NCR_Tree::~NCR_Tree()
{
	if (_left) {
		delete _left;
		_left = NULL;
	}

	if (_rec) {
		delete _rec;
		_p_list = NULL;
	}

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

NCR_Tree* NCR_Tree::search_record_name(Buffer *name)
{
	int		s;
	NCR_Tree	*p = this;

	while (p) {
		s = name->like(p->_rec->_name);
		if (s == 0) {
			return p;
		} else if (s < 0) {
			p = p->_left;
		} else {
			p = p->_right;
		}
	}

	return NULL;
}

void NCR_Tree::prune()
{
	if (_left) {
		_left->prune();
	}
	_rec	= NULL;
	_p_list	= NULL;
	if (_right) {
		_right->prune();
	}
}

void NCR_Tree::dump()
{
	if (_left) {
		_left->dump();
	}
	if (_rec) {
		dlog.it("\t%d|%s\n", _rec->_stat, _rec->_name->_v);
	}
	if (_right) {
		_right->dump();
	}
}

void NCR_Tree::dump_tree(const int t)
{
	if (_right) {
		_right->dump_tree(t + 1);
	}

	if (t) {
		for (int i = 0; i < t - 1; ++i) {
			putchar('\t');
		}
		printf("  |-----");
	}
	printf("(%3d)%s\n", _rec->_stat, _rec->_name->_v);

	if (_left) {
		_left->dump_tree(t + 1);
	}
}

static NCR_Tree* tree_rotate_right(NCR_Tree *root, NCR_Tree *y)
{
	NCR_Tree *x = NULL;

	x		= y->_left;
	y->_left	= x->_right;

	if (x->_right)
		x->_right->_top = y;

	x->_right	= y;
	x->_top		= y->_top;

	if (y == root) {
		root = x;
	} else {
		if (y->_top->_left == y)
			y->_top->_left = x;
		else
			y->_top->_right = x;
	}

	y->_top = x;

	return root;
}

static NCR_Tree* tree_rotate_left(NCR_Tree *root, NCR_Tree *y)
{
	NCR_Tree *x = NULL;

	x		= y->_right;
	y->_right	= x->_left;

	if (x->_left)
		x->_left->_top = y;

	x->_left	= y;
	x->_top		= y->_top;

	if (y == root) {
		root = x;
	} else {
		if (y->_top->_left == y)
			y->_top->_left = x;
		else
			y->_top->_right = x;
	}

	y->_top = x;

	return root;

}

NCR_Tree* NCR_Tree::REBUILD(NCR_Tree *root, NCR_Tree *node)
{
	int		stat	= 0;
	NCR_Tree	*p	= node;
	NCR_Tree	*top	= NULL;

	stat = node->_rec->_stat;
	while (p->_top) {
		top = p->_top;
		if (stat > top->_rec->_stat) {
			if (top->_left == p) {
				root = tree_rotate_right(root, top);
			} else {
				root = tree_rotate_left(root, top);
			}
		} else {
			break;
		}
	}

	return root;
}

void NCR_Tree::INSERT(NCR_Tree **root, NCR_Tree *node)
{
	int		s	= 0;
	Buffer		*name	= NULL;
	NCR_Tree	*p	= NULL;
	NCR_Tree	*top	= NULL;

	if (!node || (node && !node->_rec))
		return;

	if (!(*root)) {
		(*root) = node;
		return;
	}

	name	= node->_rec->_name;
	p	= (*root);
	while (p) {
		top	= p;
		s	= name->cmp(p->_rec->_name);
		if (s < 0)
			p = p->_left;
		else
			p = p->_right;
	}

	if (s < 0)
		top->_left = node;
	else
		top->_right = node;

	node->_top = top;

	(*root) = REBUILD((*root), node);
}

NCR_Tree* NCR_Tree::REMOVE(NCR_Tree **root, NCR_Tree *node)
{
	if (node != (*root)) {
		if (node->_top->_left == node)
			node->_top->_left = NULL;
		else
			node->_top->_right = NULL;

		node->_top = NULL;

		if (node->_right) {
			INSERT(root, node->_right);
		}
		if (node->_left) {
			INSERT(root, node->_left);
		}
	} else {
		if (node->_right) {
			(*root) = node->_right;
			INSERT(root, node->_left);
		} else {
			(*root) = node->_left;
		}
	}
	node->_right	= NULL;
	node->_left	= NULL;
	node->_top	= NULL;

	return node;
}

/**
 * @return		:
 *	< NULL		: record not found.
 *	< NCRTree*	: success.
 */
NCR_Tree* NCR_Tree::REMOVE_RECORD(NCR_Tree **root, NCR *record)
{
	int		s	= 0;
	Buffer		*name	= NULL;
	NCR_Tree	*p	= (*root);

	if (!record)
		return NULL;

	name = record->_name;
	while (p) {
		s = name->like(p->_rec->_name);
		if (s == 0)
			break;
		if (s < 0)
			p = p->_left;
		else
			p = p->_right;
	}
	if (!p)
		return NULL;

	return REMOVE(root, p);
}

} /* namespace::rescached */
