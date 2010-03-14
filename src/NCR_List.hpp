/*
 * Copyright (C) 2009,2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_NCR_LIST_HPP
#define	_RESCACHED_NCR_LIST_HPP	1

#include "NCR.hpp"

namespace rescached {

class NCR_List {
public:
	NCR_List();
	~NCR_List();

	void dump();

	static void REBUILD(NCR_List **top, NCR_List *node);
	static void ADD(NCR_List **top, NCR_List *node);

	NCR		*_rec;
	NCR_List	*_up;
	NCR_List	*_down;
	NCR_List	*_last;
	void		*_p_tree;
private:
	NCR_List(const NCR_List&);
	void operator=(const NCR_List&);
};

} /* namespace::rescached */

#endif
