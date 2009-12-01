/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_NCR_HPP
#define	_RESCACHED_NCR_HPP	1

#include "DNSQuery.hpp"

using vos::DNSQuery;

namespace rescached {

class NCR {
public:
	NCR();
	~NCR();

	void dump();

	static int INIT(NCR **o, const int type, const Buffer *name,
			const Buffer *question, const Buffer *answer);

	int		_stat;
	int		_type;
	Buffer		*_name;
	DNSQuery	*_qstn;
	DNSQuery	*_answ;
private:
	DISALLOW_COPY_AND_ASSIGN(NCR);
};

} /* namespace::rescached */

#endif
