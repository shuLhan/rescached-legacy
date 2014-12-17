/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_HPP
#define	_RESCACHED_HPP	1

#include "lib/Config.hpp"
#include "lib/Resolver.hpp"
#include "lib/SockServer.hpp"
#include "lib/SSVReader.hpp"
#include "common.hpp"
#include "NameCache.hpp"
#include "NCR.hpp"
#include "NCR_List.hpp"
#include "ResQueue.hpp"

using vos::File;
using vos::Config;
using vos::SockAddr;
using vos::Socket;
using vos::SockServer;
using vos::Resolver;
using vos::SSVReader;

namespace rescached {

#define	RESCACHED_CONF		"rescached.cfg"
#define	RESCACHED_CONF_HEAD	"RESCACHED"
#define	RESCACHED_DATA		"rescached.vos"
#define	RESCACHED_DATA_BAK	"rescached.vos.bak"
#define	RESCACHED_DATA_BAK_EXT	".bak"
#define	RESCACHED_LOG		"rescached.log"
#define	RESCACHED_PID		"rescached.pid"
#define	RESCACHED_CACHE_MAX	100000

#define	RESCACHED_DEF_PARENT	"8.8.8.8, 8.8.4.4"
#define	RESCACHED_DEF_LISTEN	"127.0.0.1"
#define	RESCACHED_DEF_PORT	53
#define	RESCACHED_DEF_THRESHOLD	1
#define	RESCACHED_DEF_DEBUG	0
#define	RESCACHED_DEF_TIMEOUT	300
#define	RESCACHED_DEF_MODE	CACHE_IS_TEMPORARY

class Rescached {
public:
	Rescached();
	~Rescached();

	int init(const char* fconf);
	int load_config(const char* fconf);
	int bind();
	int load_hosts ();
	int load_cache();

	int run();
	void queue_clean();
	int queue_process(DNSQuery* answer);
	int queue_send_answer(struct sockaddr_in* udp_client
				, Socket* tcp_client
				, DNSQuery* question
				, DNSQuery* answer);
	int process_client(struct sockaddr_in* udp_client
				, Socket* tcp_client
				, DNSQuery** question);
	int process_tcp_client();
	int queue_push(struct sockaddr_in* udp_client, Socket* tcp_client
			, DNSQuery** question);

	void exit();
	int create_backup();

	Buffer		_fdata;
	Buffer		_fdatabak;
	Buffer		_flog;
	Buffer		_fpid;
	Buffer		_dns_parent;
	Buffer		_listen_addr;
	int		_listen_port;
	int		_rto;

	Resolver	_resolver;
	SockServer	_srvr_udp;
	SockServer	_srvr_tcp;

	fd_set		_fd_all;
	fd_set		_fd_read;
	int		_maxfds;
	int		_running;

	NameCache	_nc;
	ResQueue*	_queue;
private:
	Rescached(const Rescached&);
	void operator=(const Rescached&);
};

} /* namespace::rescached */

#endif
