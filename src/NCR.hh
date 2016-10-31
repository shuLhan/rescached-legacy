//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _RESCACHED_NCR_HH
#define _RESCACHED_NCR_HH 1

#include "common.hh"
#include "DNSQuery.hh"

using vos::Object;
using vos::DNSQuery;
using vos::Buffer;
using vos::BNode;

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
class NCR : public Object {
public:
	NCR();
	~NCR();

	const char* chars();

	int		_stat;
	uint32_t	_ttl;
	Buffer*		_name;
	DNSQuery*	_answ;
	void*		_p_tree;
	BNode*		_p_list;

	static int INIT(NCR** o, const Buffer* name, const Buffer* answer);
	static int CMP_BY_STAT(Object* x, Object* y);

	static const char* __name;
private:
	NCR(const NCR&);
	void operator=(const NCR&);
};

} /* namespace::rescached */

#endif
// vi: ts=8 sw=8 tw=78:
