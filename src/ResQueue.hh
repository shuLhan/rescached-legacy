//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _RESCACHED_QUEUE_HH
#define _RESCACHED_QUEUE_HH 1

#include "Socket.hh"
#include "DNSQuery.hh"

using vos::Object;
using vos::Socket;
using vos::DNSQuery;

namespace rescached {

enum queue_state {
	IS_NEW		= 0
,	IS_RESOLVING	= 1
,	IS_RESOLVED	= 2
,	IS_TIMEOUT	= 4
};

class ResQueue : public Object {
public:
	ResQueue();
	~ResQueue();

	time_t			_timeout;
	struct sockaddr_in*	_udp_client;
	Socket*			_tcp_client;
	DNSQuery*		_qstn;

	void set_state(queue_state);
	queue_state get_state();

	static const char* __cname;
private:
	ResQueue(const ResQueue&);
	void operator=(const ResQueue&);

	queue_state _state;
};

} /* namespace::rescached */

#endif
// vi: ts=8 sw=8 tw=78:
