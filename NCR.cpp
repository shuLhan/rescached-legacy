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
	printf(" [NCR_Tree: %s]\n", _name ? _name->_v : "\0");
	printf("\t n accessed : %d\n", _stat); 

	printf("\t question: \n");
	if (_qstn)
		_qstn->dump();

	printf("\t answer:\n");
	if (_answ)
		_answ->dump();

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

	s = DNSQuery::INIT(&(*o)->_qstn, question);
	if (s != 0)
		goto err;

	s = DNSQuery::INIT(&(*o)->_answ, answer);
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
