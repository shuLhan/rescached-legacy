/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "NCR.hpp"

namespace rescached {

NCR::NCR(Buffer *name, Buffer *question, Buffer *answer) :
	_stat(1),
	_name(NULL),
	_qstn(NULL),
	_answ(NULL)
{
	if (name)
		_name = new Buffer(name);
	if (question)
		_qstn = new DNSQuery(question);
	if (answer)
		_answ = new DNSQuery(answer);
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

} /* namespace::rescached */
