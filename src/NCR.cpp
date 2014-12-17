/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "NCR.hpp"

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

/**
 * @method	: NCR::search_answer_by_type
 * @desc	: search list answer by query type 'qtype'.
 * @param qtype	: query type.
 * @return q	: found.
 * @return NULL	: not found
 */
DNSQuery* NCR::search_answer_by_type (uint16_t qtype)
{
	DNSQuery* p = _answ;

	while (p) {
		if (p->_q_type == qtype) {
			return p;
		}
		p = _answ->_next;
	}
	return NULL;
}

void NCR::dump()
{
	dlog.writes("[rescached] NCR::dump: %s\n"
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

	(*o)->_answ->extract_header ();
	(*o)->_answ->extract_question ();

	return 0;
err:
	delete (*o);
	(*o) = NULL;

	return s;
}

} /* namespace::rescached */
