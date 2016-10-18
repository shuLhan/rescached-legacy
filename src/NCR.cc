//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "NCR.hh"

namespace rescached {

NCR::NCR() :
	_stat(1)
,	_ttl(0)
,	_name(NULL)
,	_answ(NULL)
{}

NCR::~NCR()
{
	if (_name) {
		delete _name;
		_name = NULL;
	}
	if (_answ) {
		delete _answ;
		_answ = NULL;
	}
}

void NCR::dump()
{
	dlog.writes("[rescached] NCR::dump: %s\n"
		"  accessed : %d times\n", _name ? _name->chars() : "\0", _stat);

	if (_answ) {
		dlog.write_raw("  answer     :\n");
		_answ->dump(vos::DNSQ_DO_ALL);
	}
}

/**
 * @return	:
 *	< 0	: success.
 *	< <0	: fail.
 */
int NCR::INIT(NCR** o, const Buffer* name, const Buffer* answer)
{
	if (!name || !answer) {
		return -1;
	}

	int s;

	(*o) = new NCR();
	if (!(*o)) {
		return -1;
	}

	s = Buffer::INIT(&(*o)->_name, name);
	if (s != 0) {
		goto err;
	}

	s = DNSQuery::INIT(&(*o)->_answ, answer, vos::BUFFER_IS_UDP);
	if (s != 0) {
		goto err;
	}

	return s;
err:
	delete (*o);
	(*o) = NULL;

	return s;
}

} /* namespace::rescached */
// vi: ts=8 sw=8 tw=78:
