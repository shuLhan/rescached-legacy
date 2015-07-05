/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "NCR_Tree.hpp"
#include "NCR_List.hpp"

namespace rescached {

NCR_Tree::NCR_Tree() :
	_color(RBT_IS_BLACK)
,	_rec(NULL)
,	_left(NULL)
,	_right(NULL)
,	_top(NULL)
,	_p_list(NULL)
{}

NCR_Tree::~NCR_Tree()
{
	if (_left) {
		delete _left;
		_left = NULL;
	}
	if (_rec) {
		delete _rec;
		_rec = NULL;
	}
	if (_right) {
		delete _right;
		_right = NULL;
	}
	_p_list = NULL;
}

/**
 * @method	: NCR_Tree::cmp
 * @param	:
 *	> r	: record to compare to.
 * @return	:
 *	< 1	: this > bfr.
 *	< 0	: this == bfr.
 *	< -1	: this < bfr.
 * @desc	: compare name in this object with name in record 'r'.
 */
int NCR_Tree::cmp(NCR* ncr)
{
	if (!_rec) {
		return -1;
	}
	if (!_rec->_name) {
		return -1;
	}
	return _rec->_name->like(ncr->_name);
}

/**
 * @method		: NCR_Tree::search_record_name
 * @param		:
 *	> name		: record name.
 * @return		:
 *	< NCR_Tree*	: success.
 *	< NULL		: fail, record not found.
 * @desc		: search node with name is 'name' in the tree.
 */
NCR_Tree* NCR_Tree::search_record_name(Buffer* name)
{
	int		s;
	NCR_Tree*	p = this;

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

/**
 * @method	: NCR_Tree::prune
 * @desc	: clear all data in this tree.
 */
void NCR_Tree::prune()
{
	if (_left) {
		_left->prune();
	}
	if (_rec) {
		delete _rec;
		_rec = NULL;
	}
	if (_right) {
		_right->prune();
	}
	_top = NULL;
	_p_list	= NULL;
}

/**
 * @method	: NCR_Tree::dump
 * @desc	: Print content of the tree to the standard output, in
 * sequential mode.
 */
void NCR_Tree::dump()
{
	if (_left) {
		_left->dump();
	}
	if (_rec && DBG_LVL_IS_3) {
		dlog.writes("\t%d|%s\n", _rec->_stat, _rec->_name->_v);
	}
	if (_right) {
		_right->dump();
	}
}

/**
 * @method	: NCR_Tree::dump_tree
 * @param	:
 *	> t	: space for formatting.
 * @desc	: Print content of tree to standard mode, in tree mode - left
 * to right.
 */
void NCR_Tree::dump_tree(const int t)
{
	if (_right) {
		_right->dump_tree(t + 1);
	}

	if (t) {
		for (int i = 0; i < t - 1; ++i) {
			dlog.write_raw("\t");
		}
		dlog.write_raw("  |-----");
	}

	dlog.writes("(%d)%s\n", _color, _rec->_name->_v);

	if (_left) {
		_left->dump_tree(t + 1);
	}
}

static NCR_Tree* tree_rotate_right(NCR_Tree* root, NCR_Tree* x)
{
	NCR_Tree* y = NULL;

	y		= x->_left;
	x->_left	= y->_right;

	if (y->_right) {
		y->_right->_top = x;
	}

	y->_top = x->_top;

	if (x->_top == NULL) {
		root = y;
	} else {
		if (x->_top->_right == x) {
			x->_top->_right = y;
		} else {
			x->_top->_left = y;
		}
	}

	y->_right	= x;
	x->_top		= y;

	return root;
}

static NCR_Tree* tree_rotate_left(NCR_Tree* root, NCR_Tree* x)
{
	NCR_Tree* y = NULL;

	y		= x->_right;
	x->_right	= y->_left;

	if (y->_left) {
		y->_left->_top = x;
	}

	y->_top = x->_top;

	if (x->_top == NULL) {
		root = y;
	} else {
		if (x->_top->_left == x) {
			x->_top->_left = y;
		} else {
			x->_top->_right = y;
		}
	}

	y->_left	= x;
	x->_top		= y;

	return root;

}

static NCR_Tree* RBT_INSERT_FIXUP(NCR_Tree* root, NCR_Tree* node)
{
	NCR_Tree* gp	= NULL;
	NCR_Tree* y	= NULL;

	while (node->_top && node->_top->_color == RBT_IS_RED) {
		gp = node->_top->_top;
		if (!gp) {
			break;
		}
		if (node->_top == gp->_left) {
			y = gp->_right;
			if (y && y->_color == RBT_IS_RED) {
				node->_top->_color	= RBT_IS_BLACK;
				y->_color		= RBT_IS_BLACK;
				gp->_color		= RBT_IS_RED;
				node			= gp;
			} else {
				if (node == node->_top->_right) {
					node = node->_top;
					root = tree_rotate_left(root, node);
				}
				node->_top->_color = RBT_IS_BLACK;
				node->_top->_top->_color = RBT_IS_RED;
				root = tree_rotate_right(root, node->_top->_top);
			}
		} else {
			y = gp->_left;
			if (y && y->_color == RBT_IS_RED) {
				node->_top->_color	= RBT_IS_BLACK;
				y->_color		= RBT_IS_BLACK;
				gp->_color		= RBT_IS_RED;
				node			= gp;
			} else {
				if (node == node->_top->_left) {
					node = node->_top;
					root = tree_rotate_right(root, node);
				}
				node->_top->_color = RBT_IS_BLACK;
				node->_top->_top->_color = RBT_IS_RED;
				root = tree_rotate_left(root, node->_top->_top);
			}
		}
	}

	root->_color = RBT_IS_BLACK;
	return root;
}

/**
 * @method	: NCR_Tree::RBT_INSERT
 * @param	:
 *	> root	: pointer to the root of the tree.
 *	> node	: pointer to the new node to be inserted.
 * @return	:
 *	< 0	: success, or node is nil.
 *	< 1	: node already exist.
 * @desc	: Insert a new node to the tree.
 * If record with the same name already exist in tree, replace node answer
 * with a new one.
 */
int NCR_Tree::RBT_INSERT(NCR_Tree** root, NCR_Tree* node)
{
	if (!node || (node && !node->_rec)) {
		return 0;
	}
	if (!(*root)) {
		(*root) = node;
		return 0;
	}

	int		s	= 0;
	Buffer*		name	= NULL;
	NCR_Tree*	p	= NULL;
	NCR_Tree*	top	= NULL;

	name	= node->_rec->_name;
	p	= (*root);
	while (p) {
		top	= p;
		s	= name->like(p->_rec->_name);
		if (s == 0) {
			delete p->_rec->_answ;
			p->_rec->_answ		= NULL;

			if (DBG_LVL_IS_1) {
				dlog.out(
"[rescached] NCR_Tree::RBT_INSERT: '%s' replace old cache with new one\n"
, name->_v);
			}

			p->_rec->_answ		= node->_rec->_answ;
			p->_rec->_ttl		= node->_rec->_ttl;

			node->_rec->_answ	= NULL;
			return 1;
		}
		if (s < 0) {
			p = p->_left;
		} else {
			p = p->_right;
		}
	}

	if (s < 0) {
		top->_left = node;
	} else {
		top->_right = node;
	}

	node->_top	= top;
	node->_color	= RBT_IS_RED;

	(*root) = RBT_INSERT_FIXUP((*root), node);

	return 0;
}

static NCR_Tree* tree_minimum(NCR_Tree* x)
{
	while (x->_left != NULL) {
		x = x->_left;
	}
	return x;
}

static NCR_Tree* tree_successor(NCR_Tree* x)
{
	NCR_Tree* y = NULL;

	if (x->_right != NULL) {
		return tree_minimum(x->_right);
	}
	y = x->_top;
	while (y != NULL && x == y->_right) {
		x = y;
		y = y->_top;
	}

	return y;
}

NCR_Tree* RBT_REMOVE_FIXUP(NCR_Tree* root, NCR_Tree* x)
{
	NCR_Tree* w = NULL;

	while (x != root && x->_color == RBT_IS_BLACK) {
		if (x == x->_top->_left) {
			w = x->_top->_right;

			if (! w) {
				break;
			}

			if (w->_color == RBT_IS_RED) {
				w->_color	= RBT_IS_BLACK;
				x->_top->_color	= RBT_IS_RED;
				root	= tree_rotate_left(root, x->_top);
				w	= x->_top->_right;
			}

			if (w->_left && w->_right) {
				if (w->_left->_color == RBT_IS_BLACK
				&&  w->_right->_color == RBT_IS_BLACK) {
					w->_color	= RBT_IS_RED;
					x		= x->_top;
				} else {
					if (w->_right->_color == RBT_IS_BLACK) {
						w->_left->_color	= RBT_IS_BLACK;
						w->_color		= RBT_IS_RED;
						root	= tree_rotate_right(root, w);
						w	= x->_top->_right;
					}

					w->_color		= x->_top->_color;
					x->_top->_color		= RBT_IS_BLACK;
					w->_right->_color	= RBT_IS_BLACK;
					root	= tree_rotate_left(root, x->_top);
					x	= root;
				}
			} else {
				x = root;
			}
		} else {
			w = x->_top->_left;

			if (! w) {
				break;
			}

			if (w->_color == RBT_IS_RED) {
				w->_color	= RBT_IS_BLACK;
				x->_top->_color	= RBT_IS_RED;
				root	= tree_rotate_right(root, x->_top);
				w	= x->_top->_left;
			}

			if (w->_left && w->_right) {
				if (w->_right->_color == RBT_IS_BLACK
				&&  w->_left->_color == RBT_IS_BLACK) {
					w->_color	= RBT_IS_RED;
					x		= x->_top;
				} else {
					if (w->_left->_color == RBT_IS_BLACK) {
						w->_right->_color	= RBT_IS_BLACK;
						w->_color		= RBT_IS_RED;
						root	= tree_rotate_left(root, w);
						w	= x->_top->_left;
					}

					w->_color		= x->_top->_color;
					x->_top->_color		= RBT_IS_BLACK;
					w->_left->_color	= RBT_IS_BLACK;
					root	= tree_rotate_right(root, x->_top);
					x	= root;
				}
			} else {
				x = root;
			}
		}
	}

	x->_color = RBT_IS_BLACK;

	return root;
}

/**
 * @method	: NCR_Tree::RBT_REMOVE
 * @param	:
 *	> root	: pointer to the root of the tree.
 *	> node	: pointer to the node in the tree.
 * @return	:
 *	< root	: pointer to a new root.
 * @desc	: remove 'node' from the tree.
 */
NCR_Tree* NCR_Tree::RBT_REMOVE(NCR_Tree** root, NCR_Tree* node)
{
	if (!node) {
		return NULL;
	}

	NCR_Tree* x = NULL;
	NCR_Tree* y = NULL;

	if (node->_left == NULL || node->_right == NULL) {
		y = node;
	} else {
		y = tree_successor(node);
	}

	if (y->_left != NULL) {
		x = y->_left;
	} else {
		x = y->_right;
	}

	if (x != NULL) {
		x->_top = y->_top;
	}

	if (y->_top == NULL) {
		(*root) = x;
	} else {
		if (y == y->_top->_left) {
			y->_top->_left = x;
		} else {
			y->_top->_right = x;
		}
	}

	if (y != node) {
		NCR* node_rec		= node->_rec;
		void* node_p_list	= node->_p_list;
		NCR_List* r_list	= (NCR_List*) y->_p_list;

		node->_rec	= y->_rec;
		node->_p_list	= y->_p_list;

		y->_rec 	= node_rec;
		y->_p_list	= node_p_list;

		if (r_list) {
			r_list->_p_tree	= node;
		}
	}
	if (x && y->_color == RBT_IS_BLACK) {
		(*root) = RBT_REMOVE_FIXUP((*root), x);
	}

	y->_left	= NULL;
	y->_right	= NULL;
	y->_top		= NULL;

	return y;
}

} /* namespace::rescached */
