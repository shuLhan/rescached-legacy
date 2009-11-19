/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_NCR_LIST_HPP
#define	_RESCACHED_NCR_LIST_HPP	1

#include "NCR.hpp"

using vos::Error;

namespace rescached {

class NCR_List {
public:
	static void ADD(NCR_List **top, NCR_List *node);
	static void ADD_RECORD(NCR_List **top, NCR *record);

	NCR_List(NCR *record = NULL);
	~NCR_List();

	void dump();

	NCR		*_rec;
	NCR_List	*_up;
	NCR_List	*_down;
	NCR_List	*_last;
private:
	DISALLOW_COPY_AND_ASSIGN(NCR_List);
};

} /* namespace::rescached */

#endif
