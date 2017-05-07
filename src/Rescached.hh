//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _RESCACHED_HH
#define _RESCACHED_HH 1

#include "lib/Dir.hh"
#include "lib/List.hh"
#include "lib/ListBuffer.hh"
#include "lib/Config.hh"
#include "lib/SSVReader.hh"
#include "ResolverWorker.hh"

using vos::BNode;
using vos::List;
using vos::File;
using vos::Config;
using vos::SockAddr;
using vos::Socket;
using vos::SSVReader;
using vos::Dir;
using vos::DirNode;

namespace rescached {

#define	RESCACHED_CONF		"rescached.cfg"
#define	RESCACHED_CONF_HEAD	"RESCACHED"
#define	RESCACHED_CONF_LOG	"LOG"
#define	RESCACHED_DATA		"rescached.vos"
#define	RESCACHED_LOG		"rescached.log"
#define	RESCACHED_PID		"rescached.pid"
#define	RESCACHED_CACHE_MAX	100000
#define	RESCACHED_SYS_HOSTS	"/etc/hosts"

#define	HOSTS_D		"/etc/rescached/hosts.d"
#define	HOSTS_BLOCK	"hosts.block"

#define	RESCACHED_DEF_PARENT	"8.8.8.8, 8.8.4.4"
#define	RESCACHED_DEF_PARENT_CONN	"udp"
#define RESCACHED_DEF_LISTEN		"127.0.0.1:53"
#define	RESCACHED_DEF_PORT	53
#define	RESCACHED_DEF_THRESHOLD	1
#define	RESCACHED_DEF_DEBUG	0
#define	RESCACHED_DEF_MINTTL		60
#define RESCACHED_DEF_MAXTTL            86400

#define	RESCACHED_DEF_LOG_SHOW_TS	0
#define	RESCACHED_DEF_LOG_SHOW_STAMP	0
#define	RESCACHED_DEF_STAMP		"[rescached] "

extern ClientWorker CW;

class Rescached : public Object {
public:
	Rescached();
	~Rescached();

	int init(const char* fconf);
	int config_parse_server_listen(Config* cfg);
	int load_config(const char* fconf);
	int bind();
	int load_hosts(const char* hosts_file, const uint32_t attrs);
	void load_cache();

	int run();
	int process_tcp_client();
	int queue_push(struct sockaddr_in* udp_client, Socket* tcp_client
			, DNSQuery** question);

	void exit();
	int create_backup();

	Buffer		_fdata;
	Buffer		_flog;
	Buffer		_fpid;
	Buffer		_dns_parent;
	Buffer		_dns_conn;
	Buffer		_listen_addr;
	uint16_t	_listen_port;

	SockServer	_srvr_tcp;

	fd_set		_fd_all;
	fd_set		_fd_read;

	int		_show_timestamp;
	int		_show_appstamp;

private:
	Rescached(const Rescached&);
	void operator=(const Rescached&);

	void load_host_files(const char* dir, DirNode* host_file);
	void load_hosts_d();

	ResolverWorker*	_RW;

	static const char* __cname;
};

} /* namespace::rescached */

#endif
// vi: ts=8 sw=8 tw=78:
