/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_NCR_HPP
#define	_RESCACHED_NCR_HPP	1

#include "Rescached.hpp"
#include "DNSQuery.hpp"

using vos::DNSQuery;
using vos::Buffer;

namespace rescached {

/**
 * @class	: NCR
 * @attr	:
 *	- _stat	: number of times this record is queried.
 *	- _type	: type of record, TCP or UDP packet.
 *	- _name	: domain name this record represent.
 *	- _qstn	: pointer to Question packet.
 *	- _answ	: pointer to Answer packet.
 * @desc	: Name Cache Record. This class represent a domain name and
 * their DNS packet with number of times the same name is queried by clients. 
 */
class NCR {
public:
	NCR();
	~NCR();

	void dump();

	int		_stat;
	int		_type;
	Buffer*		_name;
	DNSQuery*	_qstn;
	DNSQuery*	_answ;

	static int INIT(NCR** o, const int type, const Buffer* name
			, const Buffer* question, const Buffer* answer);
private:
	NCR(const NCR&);
	void operator=(const NCR&);
};

} /* namespace::rescached */

#endif
