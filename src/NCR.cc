//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "NCR.hh"

namespace rescached {

const char* NCR::__name = "NCR";

NCR::NCR() : Object()
,	_stat(1)
,	_ttl(0)
,	_name(NULL)
,	_answ(NULL)
,	_p_tree(NULL)
,	_p_list(NULL)
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
	_p_tree = NULL;
	_p_list = NULL;
}

const char* NCR::chars()
{
	Buffer b;

	if (_v) {
		free(_v);
		_v = NULL;
	}

	b.aprint("[ \"name\": %s, \"TTL\": %d ]", _name->_v, _ttl);

	_v = b._v;
	b._v = NULL;

	return _v;
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

//
// `CMP_BY_STAT` will compare NCR object `x` and `y`.
// It will return
//
// - 0 if stat of both object equal, or
// - 1 if `x._stat` greater than `y._stat`, or
// - -1 if `x._stat` less than `y._stat`.
//
int NCR::CMP_BY_STAT(Object* x, Object* y)
{
	NCR* ncrx = (NCR*) x;
	NCR* ncry = (NCR*) y;

	if (ncrx->_stat == ncry->_stat) {
		return 0;
	}
	if (ncrx->_stat > ncry->_stat) {
		return 1;
	}
	return -1;
}

} /* namespace::rescached */
// vi: ts=8 sw=8 tw=78:
