/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "NCR_List.hpp"

namespace rescached {

NCR_List::NCR_List() :
	_rec(NULL),
	_up(NULL),
	_down(NULL),
	_last(NULL),
	_p_tree(NULL)
{}

NCR_List::~NCR_List()
{
	if (_rec) {
		delete _rec;
	}
	_up	= NULL;
	_down	= NULL;
	_last	= NULL;
	_p_tree	= NULL;
}

void NCR_List::dump()
{
	int		i	= 1;
	NCR_List	*p	= this;

	while (p) {
		if (p->_rec) {
			dlog.writes("[%d] %d|%s\n", i++, p->_rec->_stat,
					p->_rec->_name->_v);
		}
		p = p->_down;
	}
}

void NCR_List::REBUILD(NCR_List **top, NCR_List *node)
{
	int		stat	= 0;
	NCR_List	*up	= NULL;

	if (!node)
		return;

	stat = node->_rec->_stat;
	while (node->_up) {
		up = node->_up;

		if (up->_rec->_stat >= stat)
			break;

		if (node->_down) {
			node->_down->_up = up;
		}
		up->_down	= node->_down;
		node->_up	= up->_up;
		node->_down	= up;
		if (up->_up) {
			up->_up->_down = node;
		}
		up->_up = node;
	} 
	if ((*top)->_up) {
		node->_last	= (*top)->_last;
		(*top)->_last	= NULL;
		(*top)		= node;
	}
	while ((*top)->_last->_down) {
		(*top)->_last = (*top)->_last->_down;
	}
}

void NCR_List::ADD(NCR_List **top, NCR_List *node)
{
	NCR_List *p = NULL;

	if (!node)
		return;

	if (!(*top)) {
		(*top)		= node;
		(*top)->_last	= node;
	} else {
		p = (*top)->_last;
		while (p) {
			if (p->_rec) {
				if (p->_rec->_stat >= node->_rec->_stat) {
					break;
				}
			}

			p = p->_up;
		}

		if (p) {
			node->_up	= p;
			node->_down	= p->_down;
			p->_down	= node;

			if (node->_down) {
				node->_down->_up = node;
			} else {
				(*top)->_last = node;
			}
		} else {
			node->_down	= (*top);
			node->_last	= (*top)->_last;
			(*top)->_last	= NULL;
			(*top)->_up	= node;
			(*top)		= node;
		}
	}
}

} /* namespace::rescached */
