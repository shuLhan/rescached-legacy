/*
 * Copyright (C) 2009,2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "ResThread.hpp"

namespace rescached {

ResThread::ResThread() :
	_id(),
	_running(1),
	_n_query(0),
	_lock(),
	_cond(),
	_q_query(NULL)
{
	pthread_mutex_init(&_lock, NULL);
	pthread_cond_init(&_cond, NULL);
}

ResThread::~ResThread()
{
	pthread_cond_broadcast(&_cond);

	pthread_cond_destroy(&_cond);
	pthread_mutex_destroy(&_lock);
	if (_q_query) {
		delete _q_query;
	}
}

void ResThread::lock()
{
	while (pthread_mutex_trylock(&_lock) != 0)
		;
}

void ResThread::unlock()
{
	while (pthread_mutex_unlock(&_lock) != 0)
		;
}

void ResThread::wait()
{
	lock();
	pthread_cond_wait(&_cond, &_lock);
	unlock();
}

void ResThread::wakeup()
{
	lock();
	pthread_cond_broadcast(&_cond);
	unlock();
}

/**
 * @return	:
 *	< 0	: success.
 *	< <0	: fail.
 */
int ResThread::push_query_r(struct sockaddr_in *udp_client,
				Socket *tcp_client, DNSQuery *question)
{
	ResQueue *obj = NULL;

	obj = new ResQueue();
	if (!obj)
		return -1;

	obj->_udp_client	= udp_client;
	obj->_tcp_client	= tcp_client;
	obj->_qstn		= question;

	lock();

	_q_query = ResQueue::PUSH(_q_query, obj);
	_n_query++;
	pthread_cond_signal(&_cond);

	unlock();

	return 0;
}


ResQueue * ResThread::pop_query_r()
{
	ResQueue *queue	= NULL;

	lock();
	if (_n_query > 0) {
		_q_query = ResQueue::POP(_q_query, &queue);
		if (queue) {
			_n_query--;
		}
	}
	unlock();

	return queue;
}

void ResThread::set_running_r(const int run)
{
	lock();
	_running = run;
	unlock();
}

int ResThread::is_still_running_r()
{
	int r;

	lock();
	r = _running;
	unlock();

	return r;
}

} /* namespace::rescached */
