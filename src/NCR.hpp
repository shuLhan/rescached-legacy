/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_NCR_HPP
#define	_RESCACHED_NCR_HPP	1

#include "common.hpp"
#include "DNSQuery.hpp"

using vos::DNSQuery;
using vos::Buffer;

namespace rescached {

/**
 * @class	: NCR
 * @attr	:
 *	- _stat	: number of times this record is queried.
 *	- _name	: domain name this record represent.
 *	- _answ	: pointer to Answer packet.
 *	- _ttl	: time to live.
 * @desc	: Name Cache Record. This class represent a domain name and
 * their DNS packet with number of times the same name is queried by clients. 
 */
class NCR {
public:
	NCR();
	~NCR();

	DNSQuery* search_answer_by_type (uint16_t type);
	void answer_push (DNSQuery* answer);
	void answer_pop ();
	void dump();

	int		_stat;
	uint32_t	_ttl;
	Buffer*		_name;
	DNSQuery*	_answ;

	static int INIT(NCR** o, const Buffer* name, const Buffer* answer);
private:
	NCR(const NCR&);
	void operator=(const NCR&);
};

} /* namespace::rescached */

#endif
