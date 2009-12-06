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
	_last(NULL)
{}

NCR_List::~NCR_List()
{
	if (_rec) {
		delete _rec;
	}
	_up	= NULL;
	_down	= NULL;
	_last	= NULL;
}

void NCR_List::dump()
{
	int		i	= 0;
	NCR_List	*p	= this;

	while (p) {
		if (p->_rec) {
			dlog.it("[%d] %d|%s\n", i++, p->_rec->_stat,
					p->_rec->_name->_v);
		}
		p = p->_down;
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

/**
 * @desc	: add a 'record' to the list 'top'.
 *
 * @param	:
 *	> top	: the head of linked list.
 *	> record: record to be inserted to list.
 *
 * @return	:
 *	< 0	: success.
 *	< <0	: fail, out of memory.
 */
int NCR_List::ADD_RECORD(NCR_List **top, NCR *record)
{
	NCR_List *node = NULL;

	if (!record)
		return 0;

	node = new NCR_List();
	if (!node)
		return -vos::E_MEM;

	node->_rec = record;

	ADD(top, node);

	return 0;
}

} /* namespace::rescached */
