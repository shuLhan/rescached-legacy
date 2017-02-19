/*
 * Copyright 2010-2016 Mhd Sulhan (ms@kilabit.info)
 */

#include "Rescached.hh"

namespace rescached {

ClientWorker CW;

Rescached::Rescached() :
	_fdata()
,	_flog()
,	_fpid()
,	_fhostsblock()
,	_dns_parent()
,	_dns_conn()
,	_listen_addr()
,	_listen_port(RESCACHED_DEF_PORT)
,	_srvr_tcp()
,	_fd_all()
,	_fd_read()
,	_show_timestamp(RESCACHED_DEF_LOG_SHOW_TS)
,	_show_appstamp(RESCACHED_DEF_LOG_SHOW_STAMP)
,	_RW(NULL)
{}

Rescached::~Rescached()
{
	if (_RW) {
		delete _RW;
		_RW = NULL;
	}
}

/**
 * @method	: Rescached::init()
 * @param	:
 *	> fconf : config file to read.
 * @return	:
 *	< 0	: success.
 *	< -1	: fail.
 * @desc	: initialize Rescached object.
 */
int Rescached::init(const char* fconf)
{
	register int s;

	s = load_config(fconf);
	if (s != 0) {
		return -1;
	}

	// Open log file with maximum size to 2MB
	s = dlog.open(_flog._v, 2048000
		, _show_appstamp ? RESCACHED_DEF_STAMP : ""
		, _show_timestamp);
	if (s != 0) {
		return -1;
	}

	s = File::WRITE_PID (_fpid.chars());
	if (s != 0) {
		dlog.er("PID file exist"
			", rescached process may already running.\n");
		return -1;
	}

	s = _nc.bucket_init ();
	if (s != 0) {
		return -1;
	}

	s = load_hosts(RESCACHED_SYS_HOSTS, vos::DNS_IS_LOCAL);
	if (s < 0) {
		return -1;
	}

	s = load_hosts_block ();
	if (s < 0) {
		return -1;
	}

	load_cache();

	s = bind();

	return s;
}

//
// `config_parse_server_listen()` will get "server.listen" from user
// configuration and parse their value to get listen address and port.
//
// (1) If no port is set, then default port will be used.
//
int Rescached::config_parse_server_listen(Config* cfg)
{
	int s;
	List* addr_port = NULL;
	Buffer* addr = NULL;
	Buffer* port = NULL;

	const char* v = cfg->get(RESCACHED_CONF_HEAD, "server.listen"
				, RESCACHED_DEF_LISTEN);
	s = _listen_addr.copy_raw(v);
	if (s != 0) {
		return -1;
	}

	addr_port = _listen_addr.split_by_char(':', 1);

	// (1)
	if (addr_port->size() == 1) {
		_listen_port = RESCACHED_DEF_PORT;
		goto out;
	}

	addr = (Buffer*) addr_port->at(0);
	port = (Buffer*) addr_port->at(1);

	_listen_addr.copy(addr);
	_listen_port = (uint16_t) port->to_lint();

out:
	if (addr_port) {
		delete addr_port;
	}

	return 0;
}

/**
 * @method	: Rescached::load_config
 * @param	:
 *	> fconf : config file to read.
 * @return	:
 *	< 0	: success.
 *	< -1	: fail.
 * @desc	: get user configuration from file.
 */
int Rescached::load_config(const char* fconf)
{
	int		s	= 0;
	Config		cfg;
	const char*	v	= NULL;

	_running = 1;

	if (!fconf) {
		fconf = RESCACHED_CONF;
	}

	dlog.out("loading config '%s'\n", fconf);

	s = cfg.load(fconf);
	if (s < 0) {
		dlog.er("cannot open config file '%s'!", fconf);
		return -1;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "file.data", RESCACHED_DATA);
	s = _fdata.copy_raw(v);
	if (s != 0) {
		return -1;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "file.log", RESCACHED_LOG);
	s = _flog.copy_raw(v);
	if (s != 0) {
		return -1;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "file.pid", RESCACHED_PID);
	s = _fpid.copy_raw(v);
	if (s != 0) {
		return -1;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "file.hosts.block"
		, RESCACHED_HOSTS_BLOCK);
	s = _fhostsblock.copy_raw(v);
	if (s != 0) {
		return -1;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "server.parent"
			, RESCACHED_DEF_PARENT);
	s = _dns_parent.copy_raw(v);
	if (s != 0) {
		return -1;
	}

	v = cfg.get (RESCACHED_CONF_HEAD, "server.parent.connection"
			, RESCACHED_DEF_PARENT_CONN);
	s = _dns_conn.copy_raw (v);
	if (s != 0) {
		return -1;
	}

	if (_dns_conn.like_raw ("tcp") == 0) {
		_dns_conn_t = connection_type::IS_TCP;
	}

	s = config_parse_server_listen(&cfg);
	if (s != 0) {
		return -1;
	}

	_rto = (uint8_t) cfg.get_number(RESCACHED_CONF_HEAD, "server.timeout"
					, RESCACHED_DEF_TIMEOUT);
	if (_rto <= 0) {
		_rto = RESCACHED_DEF_TIMEOUT;
	}

	_nc._cache_max = cfg.get_number(RESCACHED_CONF_HEAD, "cache.max"
					, RESCACHED_CACHE_MAX);
	if (_nc._cache_max <= 0) {
		_nc._cache_max = RESCACHED_CACHE_MAX;
	}

	_nc._cache_thr = cfg.get_number(RESCACHED_CONF_HEAD, "cache.threshold"
					, RESCACHED_DEF_THRESHOLD);
	if (_nc._cache_thr <= 0) {
		_nc._cache_thr = RESCACHED_DEF_THRESHOLD;
	}

	_cache_minttl = (uint32_t) cfg.get_number(RESCACHED_CONF_HEAD
		, "cache.minttl", RESCACHED_DEF_MINTTL);
	if (_cache_minttl <= 0) {
		_cache_minttl = RESCACHED_DEF_MINTTL;
	}

	_dbg = (int) cfg.get_number(RESCACHED_CONF_HEAD, "debug"
					, RESCACHED_DEF_DEBUG);
	if (_dbg < 0) {
		_dbg = RESCACHED_DEF_DEBUG;
	}

	_show_timestamp = (int) cfg.get_number(RESCACHED_CONF_LOG
						, "show_timestamp"
						, RESCACHED_DEF_LOG_SHOW_TS);

	_show_appstamp = (int) cfg.get_number(RESCACHED_CONF_LOG
					, "show_appstamp"
					, RESCACHED_DEF_LOG_SHOW_STAMP);

	/* environment variable replace value in config file */
	v	= getenv("RESCACHED_DEBUG");
	_dbg	= (!v) ? _dbg : atoi(v);

	if (DBG_LVL_IS_1) {
		dlog.er("cache file        > %s\n", _fdata._v);
		dlog.er("pid file          > %s\n", _fpid._v);
		dlog.er("log file          > %s\n", _flog._v);
		dlog.er("hosts blocked     > %s\n", _fhostsblock._v);
		dlog.er("parent address    > %s\n", _dns_parent._v);
		dlog.er("parent connection > %s\n", _dns_conn._v);
		dlog.er("listening on      > %s:%d\n"
			, _listen_addr._v, _listen_port);
		dlog.er("timeout           > %d seconds\n", _rto);
		dlog.er("cache maximum     > %ld\n", _nc._cache_max);
		dlog.er("cache threshold   > %ld\n", _nc._cache_thr);
		dlog.er("cache min TTL     > %d\n", _cache_minttl);
		dlog.er("debug level       > %d\n", _dbg);
		dlog.er("show timestamp    > %d\n", _show_timestamp);
		dlog.er("show stamp        > %d\n", _show_appstamp);
	}

	return 0;
}

/**
 * @method	: Rescached::bind
 * @return	:
 *	< 0	: success.
 *	< -1	: fail.
 * @desc	: initialize Resolver object and start listening to client
 * connections.
 *
 * (1) Create and run resolver worker object based on `server.parent` and
 * `server.connection` values.
 *
 * (3) Create server for UDP.
 * (4) Create server for TCP.
 * (5) Initialize open fd.
 */
int Rescached::bind()
{
	if (!_running) {
		return 0;
	}

	register int s;

	// (1)
	_RW = ResolverWorker::INIT(&_dns_parent, _dns_conn_t);
	if (!_RW) {
		return -1;
	}

	// (3)
	s = _srvr_udp.create_udp();
	if (s != 0) {
		return -1;
	}

	s = _srvr_udp.bind(_listen_addr._v, _listen_port);
	if (s != 0) {
		return -1;
	}

	// (4)
	s = _srvr_tcp.create();
	if (s != 0) {
		return -1;
	}

	s = _srvr_tcp.bind_listen(_listen_addr._v, _listen_port);
	if (s != 0) {
		return -1;
	}

	// (5)
	FD_ZERO(&_fd_all);
	FD_ZERO(&_fd_read);

	FD_SET(_srvr_udp._d, &_fd_all);
	FD_SET(_srvr_tcp._d, &_fd_all);

	dlog.out("listening on %s:%d.\n", _listen_addr._v
		, _listen_port);

	return 0;
}

/**
 * @method	: Rescached::load_hosts
 * @desc	: load host-ip address in hosts file.
 * @return 1	: unsupported, can not find hosts file in the system.
 * @return 0	: success.
 * @return -1	: fail to open and/or parse hosts file.
 */
int Rescached::load_hosts(const char* fhosts, const uint32_t attrs)
{
	int	s	= 0;
	int	is_ipv4	= 0;
	int	addr	= 0;
	int	cnt	= 0;

	if (!fhosts) {
		return -1;
	}

	dlog.out ("loading '%s'...\n", fhosts);

	SSVReader	reader;
	DNSQuery	qanswer;
	Buffer*		ip = NULL;
	List*		row = NULL;
	Buffer*		c = NULL;
	int x, y;

	reader._comment_c = '#';

	s = reader.load (fhosts);
	if (s != 0) {
		return -1;
	}

	for (x = 0; x < reader._rows->size() && _running; x++) {
		row = (List*) reader._rows->at(x);
		ip = (Buffer*) row->at(0);

		is_ipv4	= inet_pton (AF_INET, ip->chars(), &addr);
		addr	= 0;

		if (is_ipv4 != 1) {
			continue;
		}

		for (y = 1; y < row->size(); y++) {
			c = (Buffer*) row->at(y);

			s = qanswer.create_answer (c->chars()
				, (uint16_t) vos::QUERY_T_ADDRESS
				, (uint16_t) vos::QUERY_C_IN
				, INT_MAX
				, (uint16_t) ip->_i
				, ip->chars()
				, attrs);

			if (s == 0) {
				_nc.insert_copy(&qanswer, 0, 1);
				cnt++;
			}
		}
	}

	dlog.out ("%d addresses loaded.\n", cnt);

	return 0;
}

int Rescached::load_hosts_block ()
{
	int s = 0;

	// Load blocked hosts file from config
	if (! _fhostsblock.is_empty ()) {
		s = File::IS_EXIST (_fhostsblock._v);

		if (s) {
			return load_hosts(_fhostsblock._v
					, vos::DNS_IS_BLOCKED);
		}
	}

	return 0;
}

/**
 * `Rescached::load_cache()` will load cache data from file.
 */
void Rescached::load_cache()
{
	if (!_running) {
		return;
	}

	dlog.out("loading cache '%s'...\n", _fdata._v);

	_nc.load(_fdata._v);

	dlog.out("%d records loaded.\n", _nc._cachel.size());

	if (DBG_LVL_IS_3) {
		_nc.dump();
	}
}

/**
 * @method	: Rescached::run
 * @return	:
 *	< 0	: success.
 *	< -1	: fail.
 * @desc	: run Rescached service.
 */
int Rescached::run()
{
	int			s		= 0;
	struct timeval		timeout;
	DNSQuery*		question	= NULL;
	Socket*			client		= NULL;
	struct sockaddr_in*	addr		= NULL;

	while (_running) {
		_fd_read	= _fd_all;
		timeout.tv_sec	= _rto;
		timeout.tv_usec	= 0;

		s = select(FD_SETSIZE, &_fd_read, NULL, NULL, &timeout);
		if (s <= 0) {
			if (EINTR == errno) {
				s = 0;
				break;
			}
			continue;
		}

		if (FD_ISSET(_srvr_udp._d, &_fd_read)) {
			if (DBG_LVL_IS_2) {
				dlog.out("read server udp.\n");
			}

			addr = (struct sockaddr_in*) calloc(1
							, SockAddr::IN_SIZE);
			if (!addr) {
				dlog.er("error at allocating new address!\n");
				continue;
			}

			s = (int) _srvr_udp.recv_udp(addr);
			if (s <= 0) {
				dlog.er ("error at receiving UDP packet!\n");
				free(addr);
				addr = NULL;
				continue;
			}

			s = DNSQuery::INIT(&question, &_srvr_udp);
			if (s < 0) {
				dlog.er("error at initializing dnsquery!\n");
				free(addr);
				addr = NULL;
				continue;
			}

			s = queue_push(addr, NULL, &question);
			if (s != 0) {
				dlog.er("error at processing client!\n");
				delete question;
				question = NULL;
			}
			if (addr) {
				free (addr);
				addr = NULL;
			}
		} else if (FD_ISSET(_srvr_tcp._d, &_fd_read)) {
			if (DBG_LVL_IS_2) {
				dlog.out("read server tcp.\n");
			}

			client = _srvr_tcp.accept_conn();
			if (! client) {
				dlog.er("error at accepting client TCP connection!\n");
				continue;
			}

			FD_SET(client->_d, &_fd_all);

			s = process_tcp_client();
		} else {
			if (_srvr_tcp._clients) {
				s = process_tcp_client();
			}
		}
	}

	if (DBG_LVL_IS_1) {
		dlog.er("service stopped ...\n");
	}

	return s;
}

int Rescached::process_tcp_client()
{
	int		x		= 0;
	int		s		= 0;
	Socket*		client		= NULL;
	DNSQuery*	question	= NULL;

	if (!_srvr_tcp._clients) {
		return 0;
	}

	for (; x < _srvr_tcp._clients->size(); x++) {
		client = (Socket*) _srvr_tcp._clients->at(x);

		if (FD_ISSET(client->_d, &_fd_read) == 0) {
			continue;
		}

		client->reset();
		s = client->read();

		if (s <= 0) {
			/* client read error or close connection */
			FD_CLR(client->_d, &_fd_all);
			_srvr_tcp.remove_client(client);
			delete client;
		} else {
			s = DNSQuery::INIT(&question, client
						, vos::BUFFER_IS_TCP);
			if (s < 0) {
				continue;
			}

			queue_push(NULL, client, &question);

			question = NULL;
		}
	}

	return 0;
}

/**
 * @method		: Rescached::queue_push
 * @param		:
 *	> udp_client	: address of client, if send from UDP.
 *	> tcp_client	: pointer to client Socket object, if query is send
 *                        from TCP.
 *	> question	: pointer to DNS packet question.
 * @return		:
 *	< 0		: success.
 *	< -1		: fail.
 * @desc		: add client question to queue.
 */
int Rescached::queue_push(struct sockaddr_in* udp_client, Socket* tcp_client
			, DNSQuery** question)
{
	ResQueue* obj = NULL;

	obj = new ResQueue();
	if (!obj) {
		return -1;
	}

	if (udp_client) {
		obj->_udp_client = (struct sockaddr_in *) calloc (1
						, SockAddr::IN_SIZE);
		memcpy (obj->_udp_client, udp_client, SockAddr::IN_SIZE);
	}

	obj->_tcp_client	= tcp_client;
	obj->_qstn		= (*question);

	CW.push_question(obj);

	return 0;
}

/**
 * @method	: Rescached::exit
 * @desc	: release Rescache object.
 */
void Rescached::exit()
{
	_running = 0;

	if (_RW) {
		_RW->stop();
		_RW->join();
	}

	if (_fdata._v) {
		_nc.save(_fdata._v);
	}

	if (_fpid._v) {
		unlink(_fpid._v);
	}
}

} /* namespace::rescached */
// vi: ts=8 sw=8 tw=78:
