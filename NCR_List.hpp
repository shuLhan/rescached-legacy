/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_NCR_LIST_HPP
#define	_RESCACHED_NCR_LIST_HPP	1

#include "main.hpp"
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
	DISALLOW_COPY_AND_ASSIGN(NCR_List);
};

} /* namespace::rescached */

#endif
