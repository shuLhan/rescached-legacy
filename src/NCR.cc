//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "NCR.hh"

namespace rescached {

const char* NCR::__name = "NCR";

NCR::NCR(const Buffer* qname, uint16_t qtype) : Object()
,	_stat(1)
,	_ttl(0)
,	_q_type(qtype)
,	_name(NULL)
,	_answ(NULL)
,	_p_tree(NULL)
,	_p_list(NULL)
{
	_name = new Buffer(qname);
}

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

	if (__str) {
		free(__str);
		__str = NULL;
	}

	b.append_fmt("[ 'name': %s, 'QTYPE': %d, 'TTL': %d ]", _name->v(), _q_type
		, _ttl);

	__str = b.detach();

	return __str;
}

/**
 * @return	:
 *	< 0	: success.
 *	< <0	: fail.
 */
NCR* NCR::INIT(const Buffer* name, const Buffer* answer)
{
	if (!name || !answer) {
		return NULL;
	}

	int s;
	time_t now = time(NULL);

	NCR* o = new NCR(name);
	if (o) {
		s = DNSQuery::INIT(&o->_answ, answer, vos::BUFFER_IS_UDP);
		if (s != 0) {
			goto err;
		}

		o->_q_type = o->_answ->_q_type;

		if (o->_answ->get_num_answer() == 0) {
			o->_ttl = 0;
		} else {
			if (o->_answ->_ans_ttl_max < _cache_minttl) {
				o->_ttl = (uint32_t) (now + _cache_minttl);
			} else {
				o->_ttl = (uint32_t) (now + o->_answ->_ans_ttl_max);
			}
		}
	}

	return o;
err:
	delete o;
	o = NULL;

	return o;
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

/**
 * `CMP()` will compare two NCR objects `x` and `y` by record name and
 * question-type.
 * It will return 0 if both x and y are NULL or equal, or 1 if `x` is
 * greater than `y` or -1 if `x` is less than `y`.
 */
int NCR::CMP(Object* x, Object* y)
{
	if (!x && !y) {
		return 0;
	}
	if (x && !y) {
		return 1;
	}
	if (y && !x) {
		return -1;
	}

	NCR* nx = (NCR*) x;
	NCR* ny = (NCR*) y;
	int qx = nx->_q_type;
	int qy = ny->_q_type;
	int s = nx->_name->like(ny->_name);

	if (s == 0) {
		if (qx > qy) {
			s = 1;
		} else if (qx < qy) {
			s = -1;
		}
	}
	return s;
}

/**
 * `SWAP_PTREE()` will swap pointer to the tree of NCR `x` with `y`.
 */
void NCR::SWAP_PTREE(Object* x, Object* y)
{
	if (!x || !y) {
		return;
	}

	NCR* nx = (NCR*) x;
	NCR* ny = (NCR*) y;

	TreeNode* tmp = nx->_p_tree;
	nx->_p_tree = ny->_p_tree;
	ny->_p_tree = tmp;
}

} /* namespace::rescached */
// vi: ts=8 sw=8 tw=78:
