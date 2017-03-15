/**
 * Copyright 2017 M. Shulhan (ms@kilabit.info). All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ResolverWorker.hh"

namespace rescached {

//{{{ STATIC VARIABLES

const char* ResolverWorker::__cname = "ResolverWorker";

//}}}
//{{{ PUBLIC METHODS

ResolverWorker::ResolverWorker(Buffer* dns_parent, SocketConnType conn_type)
: Thread(NULL)
, _dns_parent(dns_parent)
, _conn_type(conn_type)
, _fd_all()
, _fd_read()
{
	FD_ZERO(&_fd_all);
	FD_ZERO(&_fd_read);
}

ResolverWorker::~ResolverWorker()
{}

void* ResolverWorker::run(void* arg)
{
	int s = 0;
	int running = 0;
	ResolverWorker* RW = (ResolverWorker*) arg;
	struct timeval timeout;

	lock();
	running = RW->is_running();
	unlock();

	while (running) {
		// TCP: if resolver connection is open, add it to selector.
		if (RW->_conn_type == vos::IS_TCP
		&&  _resolver.is_open()) {
			FD_SET(_resolver._d, &RW->_fd_all);

			if (DBG_LVL_IS_2) {
				dlog.out ("%s: adding open resolver.\n"
					, __cname);
			}
		}

		RW->_fd_read = RW->_fd_all;
		timeout.tv_sec	= _rto;
		timeout.tv_usec	= 0;

		s = select(FD_SETSIZE, &RW->_fd_read, NULL, NULL, &timeout);
		if (s <= 0) {
			if (EINTR == errno) {
				return 0;
			}
			goto cont;
		}

		if (FD_ISSET(_resolver._d, &RW->_fd_read)) {
			RW->do_read();
		}

cont:
		lock();
		running = RW->is_running();
		unlock();
	}

	if (DBG_LVL_IS_2) {
		dlog.out("%s: exit\n", __cname);
	}

	return 0;
}

int ResolverWorker::init()
{
	int s = 0;

	if (DBG_LVL_IS_2) {
		dlog.out("%s: set parent server to %s\n", __cname
			, _dns_parent.chars());
	}

	s = _resolver.set_server(_dns_parent.chars());
	if (s) {
		dlog.er("%s: fail to set parent server to %s\n", __cname
			, _dns_parent.chars());
		return -1;
	}

	if (DBG_LVL_IS_2) {
		dlog.out("%s: create socket %d\n", __cname, _conn_type);
	}

	if (_conn_type == vos::IS_UDP) {
		s = _resolver.init(SOCK_DGRAM);
	} else {
		s = _resolver.init(SOCK_STREAM);
	}
	if (s) {
		dlog.er("%s: fail to create socket %d\n", __cname
			, _conn_type);
		return -2;
	}

	if (_conn_type == vos::IS_UDP) {
		FD_SET(_resolver._d, &_fd_all);
	}

	s = start();
	if (s) {
		dlog.er("%s: fail to create thread %d\n", __cname
			, s);
		return -3;
	}

	return 0;
}

//}}}
//{{{ STATIC METHODS

/**
 * `start()` will initialize the parent server, set connection type, and run
 * the thread. It will return,
 *
 * `-1` if resolver fail to set parent dns server to `dns_parent`.
 * `-2` if resolver fail to create socket.
 * `-3` if thread can not be created.
 * `0` if success.
 */
ResolverWorker* ResolverWorker::INIT(Buffer* dns_parent
	, SocketConnType conn_type)
{
	int s = 0;
	ResolverWorker* rw = new ResolverWorker(dns_parent, conn_type);

	if (!rw) {
		return NULL;
	}

	s = rw->init();
	if (s) {
		delete rw;
		rw = NULL;
	}

	return rw;
}

//}}}
//{{{ PRIVATE METHODS

/**
 * `do_read()` will read answer from parent answer.
 *
 * There are two type of parent server connection: UDP or TCP.
 * If the connection is UDP we read it as normal.
 * If the connection is TCP, we read them and convert it to UDP because our
 * client maybe not using TCP.
 *
 * After that we pass the answer to our clients.
 */
int ResolverWorker::do_read()
{
	int s = 0;
	DNSQuery* answer = new DNSQuery();

	if (_conn_type == vos::IS_UDP) {
		if (DBG_LVL_IS_2) {
			dlog.out("%s: read udp.\n", __cname);
		}

		s = _resolver.recv_udp(answer);
	} else {
		if (DBG_LVL_IS_2) {
			dlog.out("%s: read tcp.\n", __cname);
		}

		s = _resolver.recv_tcp(answer);

		if (s <= 0) {
			FD_CLR(_resolver._d, &_fd_all);
			_resolver.close();
		} else {
			// convert answer to UDP.
			answer->to_udp();
		}
	}

	if (DBG_LVL_IS_2) {
		dlog.out("%s: read status %d.\n", __cname, s);

		dlog.out("%s: %s\n", __cname
			, answer->get_rcode_name());
	}

	if (s != 0) {
		delete answer;
		return -1;
	}

	s = _nc.insert_copy(answer, 1, 0);

	if (DBG_LVL_IS_2) {
		dlog.out("%s: push answer %s\n", __cname
			, answer->_name.chars());
	}

	if (answer) {
		CW.push_answer(answer);
	}

	return s;
}

//}}}

}
