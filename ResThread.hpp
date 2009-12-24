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

	int push_query_r(struct sockaddr *udp_client,
				Socket *tcp_client, DNSQuery *question);
	ResQueue * pop_query_r();

	void set_running_r(const int run);
	int is_still_running_r();

	pthread_t	_id;
	int		_running;
	int		_n_query;
	pthread_mutex_t	_lock;
	pthread_cond_t	_cond;
	ResQueue	*_q_query;
private:
	DISALLOW_COPY_AND_ASSIGN(ResThread);
};

} /* namespace::rescached */

#endif
