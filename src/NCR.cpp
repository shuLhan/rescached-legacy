/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "NCR.hpp"

namespace rescached {

NCR::NCR() :
	_stat(1),
	_type(vos::BUFFER_IS_UDP),
	_name(NULL),
	_qstn(NULL),
	_answ(NULL)
{
}

NCR::~NCR()
{
	if (_name) {
		delete _name;
	}
	if (_qstn) {
		delete _qstn;
	}
	if (_answ) {
		delete _answ;
	}
}

void NCR::dump()
{
	dlog.writes(
" [NCR_Tree: %s]\n\
\t accessed : %d times\n\
\t type	    : %d\n", _name ? _name->_v : "\0", _stat, _type
	);
	if (_qstn) {
		dlog.write_raw("\t question   : \n");
		_qstn->dump(vos::DNSQ_DO_ALL);
	}
	if (_answ) {
		dlog.write_raw("\t answer     :\n");
		_answ->dump(vos::DNSQ_DO_ALL);
	}
}

/**
 * @return	:
 *	< 0	: success.
 *	< <0	: fail.
 */
int NCR::INIT(NCR **o, const int type, const Buffer *name,
		const Buffer *question, const Buffer *answer)
{
	int s;

	(*o) = new NCR();
	if (!(*o))
		return -1;

	s = Buffer::INIT(&(*o)->_name, name);
	if (s != 0)
		goto err;

	s = DNSQuery::INIT(&(*o)->_qstn, question, type);
	if (s != 0)
		goto err;

	s = DNSQuery::INIT(&(*o)->_answ, answer, type);
	if (s != 0)
		goto err;

	(*o)->_type = type;

	return 0;
err:
	delete (*o);
	(*o) = NULL;

	return s;
}

} /* namespace::rescached */
