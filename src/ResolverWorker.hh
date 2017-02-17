/**
 * Copyright 2017 M. Shulhan (ms@kilabit.info). All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef	_RESCACHED_RESOLVER_HH
#define	_RESCACHED_RESOLVER_HH 1

#include "lib/Thread.hh"
#include "lib/Resolver.hh"
#include "common.hh"
#include "ClientWorker.hh"

using vos::Resolver;
using vos::connection_type;

namespace rescached {

extern ClientWorker CW;
extern NameCache _nc;

class ResolverWorker : public Thread {
public:
	ResolverWorker(Buffer* dns_parent, uint8_t conn_type);
	~ResolverWorker();

	void* run(void* arg);

	Buffer _dns_parent;
	uint8_t _conn_type;
	fd_set _fd_all;
	fd_set _fd_read;

	static ResolverWorker* INIT(Buffer* dns_parent, uint8_t conn_type);

	static const char* __cname;
private:
	ResolverWorker(const ResolverWorker&);
	void operator=(const ResolverWorker&);

	int do_read();
	int init();
};

}

#endif
