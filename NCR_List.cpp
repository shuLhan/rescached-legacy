/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "NCR_List.hpp"

namespace rescached {

void NCR_List::ADD(NCR_List **top, NCR_List *node)
{
	NCR_List *p = NULL;

	if (! node)
		return;

	if (! (*top)) {
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
			(*top)->_up	= node;
			(*top)		= node;
		}
	}
}

void NCR_List::ADD_RECORD(NCR_List **top, NCR *record)
{
	NCR_List *node = NULL;

	if (! record)
		return;

	node = new NCR_List(record);
	if (! node)
		throw Error(vos::E_MEM);

	ADD(top, node);
}

NCR_List::NCR_List(NCR *record) :
	_rec(NULL),
	_up(NULL),
	_down(NULL),
	_last(NULL)
{
	if (record)
		_rec = record;
}

NCR_List::~NCR_List()
{
	if (_rec)
		delete _rec;

	_up	= NULL;
	_down	= NULL;
	_last	= NULL;
}

void NCR_List::dump()
{
	int		i	= 0;
	NCR_List	*p	= this;

	putchar('\n');
	while (p) {
		if (p->_rec) {
			printf("[%4d] %6d|%s\n", i++, p->_rec->_stat,
					p->_rec->_name->_v);
		}
		p = p->_down;
	}
	putchar('\n');
}

} /* namespace::rescached */
