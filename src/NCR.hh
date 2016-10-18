//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _RESCACHED_NCR_HH
#define _RESCACHED_NCR_HH 1

#include "common.hh"
#include "DNSQuery.hh"

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
// vi: ts=8 sw=8 tw=78:
