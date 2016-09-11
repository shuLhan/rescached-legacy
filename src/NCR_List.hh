/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_NCR_LIST_HPP
#define	_RESCACHED_NCR_LIST_HPP	1

#include "NCR.hh"

namespace rescached {

/**
 * @class		: NCR_List
 * @attr		:
 *	- _rec		: pointer to NCR object.
 *	- _up		: pointer to up object, record with higher statistic
 *			value.
 *	- _down		: pointer to down object, record with lower statistic
 *			value.
 *	- _last		: pointer to the last object, or the bottom of the
 *			list.
 *	- _p_tree	: pointer to same NCR object but in the tree.
 * @desc		:
 * A representation of cache in stack or top-down list mode.
 */
class NCR_List {
public:
	NCR_List();
	~NCR_List();

	void dump();

	NCR*		_rec;
	NCR_List*	_up;
	NCR_List*	_down;
	NCR_List*	_last;
	void*		_p_tree;

	static void REBUILD(NCR_List** top, NCR_List* node);
	static void ADD(NCR_List** top, NCR_List* startp, NCR_List* node);
	static void DETACH(NCR_List** top, NCR_List* node);
private:
	NCR_List(const NCR_List&);
	void operator=(const NCR_List&);
};

} /* namespace::rescached */

#endif
