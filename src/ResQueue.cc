//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "ResQueue.hh"

namespace rescached {

const char* ResQueue::__cname = "ResQueue";

ResQueue::ResQueue() : Object()
,	_timeout(0)
,	_udp_client(NULL)
,	_tcp_client(NULL)
,	_qstn(NULL)
,	_state(IS_NEW)
{}

ResQueue::~ResQueue()
{
	if (_udp_client) {
		free(_udp_client);
		_udp_client = NULL;
	}
	if (_tcp_client) {
		_tcp_client = NULL;
	}
	if (_qstn) {
		delete _qstn;
		_qstn = NULL;
	}
}

void ResQueue::set_state(queue_state state)
{
	_state = state;
}

queue_state ResQueue::get_state()
{
	return _state;
}

} /* namespace::rescached */
// vi: ts=8 sw=8 tw=78:
