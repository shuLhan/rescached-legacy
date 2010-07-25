/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_QUEUE_HPP
#define	_RESCACHED_QUEUE_HPP	1

#include "Socket.hpp"
#include "DNSQuery.hpp"

using vos::Socket;
using vos::DNSQuery;

namespace rescached {

class ResQueue {
public:
	ResQueue();
	~ResQueue();

	static ResQueue * PUSH(ResQueue *head, ResQueue *node);
	static ResQueue * POP(ResQueue *head, ResQueue **node);

	struct sockaddr_in	*_udp_client;
	Socket			*_tcp_client;
	DNSQuery		*_qstn;
	ResQueue		*_next;
private:
	ResQueue(const ResQueue&);
	void operator=(const ResQueue&);
};

} /* namespace::rescached */

#endif
