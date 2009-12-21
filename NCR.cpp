/**
 * Copyright (C) 2009 kilabit.org
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
	dlog.er(" [NCR_Tree: %s]\n", _name ? _name->_v : "\0");
	dlog.er("\t n accessed	: %d\n", _stat); 
	dlog.er("\t type	: %d\n", _type);
	if (_qstn) {
		dlog.er("\t question   : \n");
		_qstn->dump(vos::DNSQ_DO_ALL);
	}
	if (_answ) {
		dlog.er("\t answer     :\n");
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
		return -vos::E_MEM;

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
