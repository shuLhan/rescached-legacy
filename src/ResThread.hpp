/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_THREAD_HPP
#define	_RESCACHED_THREAD_HPP

#include <pthread.h>
#include <signal.h>
#include "ResQueue.hpp"

using rescached::ResQueue;

namespace rescached {

class ResThread {
public:
	ResThread();
	~ResThread();

	void lock();
	void unlock();

	void wait();
	void wakeup();

	int push_query(struct sockaddr_in* udp_client
			, Socket* tcp_client, DNSQuery* question);
	ResQueue* pop_query();
	ResQueue* detach_query();

	void set_running(const int run);
	int is_still_running();

	pthread_t	_id;
	int		_running;
	int		_n_query;
	pthread_mutex_t	_lock;
	pthread_cond_t	_cond;
	ResQueue*	_q_query;
private:
	ResThread(const ResThread&);
	void operator=(const ResThread&);
};

} /* namespace::rescached */

#endif
