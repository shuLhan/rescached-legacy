/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_QUEUE_HPP
#define	_RESCACHED_QUEUE_HPP	1

#include "Socket.hh"
#include "DNSQuery.hh"

using vos::Socket;
using vos::DNSQuery;

namespace rescached {

class ResQueue {
public:
	ResQueue();
	~ResQueue();

	time_t			_timeout;
	struct sockaddr_in*	_udp_client;
	Socket*			_tcp_client;
	DNSQuery*		_qstn;
	ResQueue*		_next;
	ResQueue*		_prev;
	ResQueue*		_last;

	static ResQueue* NEW();
	static void PUSH(ResQueue** head, ResQueue* node);
	static void REMOVE(ResQueue** head, ResQueue* node);
private:
	ResQueue(const ResQueue&);
	void operator=(const ResQueue&);
};

} /* namespace::rescached */

#endif
