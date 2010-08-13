/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "ResThread.hpp"

namespace rescached {

ResThread::ResThread() :
	_id()
,	_running(1)
,	_n_query(0)
,	_lock()
,	_cond()
,	_q_query(NULL)
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
	while (pthread_mutex_trylock(&_lock) != 0) {
		;
	}
}

void ResThread::unlock()
{
	while (pthread_mutex_unlock(&_lock) != 0) {
		;
	}
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
int ResThread::push_query(struct sockaddr_in* udp_client
				, Socket* tcp_client, DNSQuery* question)
{
	ResQueue* obj = NULL;

	obj = new ResQueue();
	if (!obj) {
		return -1;
	}

	obj->_udp_client	= udp_client;
	obj->_tcp_client	= tcp_client;
	obj->_qstn		= question;

	lock();

	ResQueue::PUSH(&_q_query, obj);
	_n_query++;
	pthread_cond_signal(&_cond);

	unlock();

	return 0;
}


ResQueue* ResThread::pop_query()
{
	ResQueue* queue = NULL;

	lock();
	if (_n_query > 0) {
		ResQueue::POP(&_q_query, &queue);
		if (queue) {
			_n_query--;
		}
	}
	unlock();

	return queue;
}

ResQueue* ResThread::detach_query()
{
	if (!_q_query) {
		return NULL;
	}

	ResQueue* queries = NULL;

	lock();
	queries 	= _q_query;
	_q_query	= NULL;
	_n_query	= 0;
	unlock();

	return queries;
}

void ResThread::set_running(const int run)
{
	lock();
	_running = run;
	unlock();
}

int ResThread::is_still_running()
{
	int r;

	lock();
	r = _running;
	unlock();

	return r;
}

} /* namespace::rescached */
