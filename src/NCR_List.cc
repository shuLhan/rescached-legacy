//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "NCR_List.hh"

namespace rescached {

NCR_List::NCR_List() :
	_rec(NULL)
,	_up(NULL)
,	_down(NULL)
,	_last(NULL)
,	_p_tree(NULL)
{}

NCR_List::~NCR_List()
{
	_rec	= NULL;
	_up	= NULL;
	_down	= NULL;
	_last	= NULL;
	_p_tree	= NULL;
}

/**
 * @method	: NCR_List::dump
 * @desc	: Dump content of all NCR_list object to standard output.
 */
void NCR_List::dump()
{
	int		i	= 1;
	NCR_List*	p	= this;
	Buffer		o;

	o.append_raw("[rescached::NCR_LIST] dump:\n");
	while (p) {
		if (p->_rec) {
			o.aprint("  [%6d] %6d|%s\n", i++, p->_rec->_stat
					, p->_rec->_name->chars());
		}
		p = p->_down;
	}
	dlog.write(&o);
}

/**
 * @method	: NCR_List::REBUILD
 * @param	:
 *	> top	: pointer to the head of the list.
 *	> node	: a pointer to member in the list.
 * @desc	: Rebuild list based on value of NCR statistic.
 */
void NCR_List::REBUILD(NCR_List** top, NCR_List* node)
{
	if (!(*top) || !node) {
		return;
	}
	if (node == (*top)) {
		return;
	}
	/* quick check on first node above node */
	if (node->_up->_rec->_stat >= node->_rec->_stat) {
		return;
	}

	NCR_List* up = node->_up;

	DETACH(top, node);
	ADD(top, up, node);
}

/**
 * @method		: NCR_List::ADD
 * @param		:
 *	> top		: pointer to the head of the list.
 *	> startp	: pointer to the list member where statistic check
 *			started.
 *	> node		: pointer to the member in the list.
 * @desc		: add 'node' to the list 'top'.
 */
void NCR_List::ADD(NCR_List** top, NCR_List* startp, NCR_List* node)
{
	if (!node) {
		return;
	}
	if (!(*top)) {
		(*top)		= node;
		(*top)->_last	= node;
		return;
	}
	if (!startp) {
		startp = (*top)->_last;
	}

	int node_stat = node->_rec->_stat;

	while (startp && (startp->_rec->_stat < node_stat)) {
		startp = startp->_up;
	}
	if (startp) {
		node->_up	= startp;
		node->_down	= startp->_down;
		startp->_down	= node;

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

/**
 * @method	: NCR_List::DETACH
 * @param	:
 *	> top	: pointer to the head of the list.
 *	> node	: pointer to the member in the list.
 * @desc	: detach 'node' from the list 'top'.
 */
void NCR_List::DETACH(NCR_List** top, NCR_List* node)
{
	if (!(*top) || !node) {
		return;
	}
	if ((*top) == node) {
		(*top) = (*top)->_down;
		if ((*top)) {
			(*top)->_last	= node->_last;
			(*top)->_up	= NULL;
			node->_last	= NULL;
		}
	} else if ((*top)->_last == node) {
		(*top)->_last		= node->_up;
		(*top)->_last->_down	= NULL;
	} else {
		node->_up->_down	= node->_down;
		node->_down->_up	= node->_up;
	}
	node->_up	= NULL;
	node->_down	= NULL;
}

} /* namespace::rescached */
// vi: ts=8 sw=8 tw=78:
