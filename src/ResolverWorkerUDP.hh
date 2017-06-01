/**
 * Copyright 2017 M. Shulhan (ms@kilabit.info). All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef	_RESCACHED_RESOLVER_WORKER_UDP_HH
#define	_RESCACHED_RESOLVER_WORKER_UDP_HH 1

#include "lib/Thread.hh"
#include "lib/Resolver.hh"
#include "common.hh"
#include "ClientWorker.hh"

using vos::Resolver;
using vos::SocketConnType;

namespace rescached {

extern ClientWorker CW;
extern NameCache _nc;

class ResolverWorkerUDP : public Thread {
public:
	ResolverWorkerUDP(Buffer* dns_parent);
	~ResolverWorkerUDP();

	void* run(void* arg);

	Buffer _dns_parent;
	fd_set _fd_all;
	fd_set _fd_read;

	static ResolverWorkerUDP* INIT(Buffer* dns_parent);

	static const char* __cname;
private:
	ResolverWorkerUDP(const ResolverWorkerUDP&);
	void operator=(const ResolverWorkerUDP&);

	int do_read();
	int init();
};

}

#endif
