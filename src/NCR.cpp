/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "NCR.hpp"

namespace rescached {

NCR::NCR() :
	_stat(1)
,	_name(NULL)
,	_answ(NULL)
{}

NCR::~NCR()
{
	if (_name) {
		delete _name;
	}
	if (_answ) {
		delete _answ;
	}
}

void NCR::dump()
{
	dlog.writes("[rescached::NCR_Tree] %s\n"
		"  accessed : %d times\n", _name ? _name->v() : "\0", _stat);

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

	return 0;
err:
	delete (*o);
	(*o) = NULL;

	return s;
}

} /* namespace::rescached */
