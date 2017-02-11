/*
 * Copyright 2010-2016 Mhd Sulhan (ms@kilabit.info)
 */

#include "Rescached.hh"

namespace rescached {

Rescached::Rescached() :
	_fdata()
,	_flog()
,	_fpid()
,	_fhostsblock()
,	_dns_parent()
,	_dns_conn()
,	_listen_addr()
,	_listen_port(RESCACHED_DEF_PORT)
,	_rto(RESCACHED_DEF_TIMEOUT)
,	_dns_conn_t (RESCACHED_DEF_CONN_T)
,	_resolver()
,	_srvr_udp()
,	_srvr_tcp()
,	_fd_all()
,	_fd_read()
,	_running(1)
,	_nc()
,	_queue()
,	_show_timestamp(RESCACHED_DEF_LOG_SHOW_TS)
,	_show_appstamp(RESCACHED_DEF_LOG_SHOW_STAMP)
{}

Rescached::~Rescached()
{}

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
		_dns_conn_t = 1;
	}

	s = config_parse_server_listen(&cfg);
	if (s != 0) {
		return -1;
	}

	_rto = (int) cfg.get_number(RESCACHED_CONF_HEAD, "server.timeout"
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
 * (1) Add server list.
 * (2) Create resolver object based on type.
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
	s = _resolver.set_server(_dns_parent._v);
	if (s < 0) {
		return -1;
	}

	// (2)
	if (_dns_conn_t == 0) {
		s = _resolver.init (SOCK_DGRAM);
	} else {
		s = _resolver.init (SOCK_STREAM);
	}
	if (s < 0) {
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

	if (0 == _dns_conn_t) {
		FD_SET(_resolver._d, &_fd_all);
	}

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
 * `_resolver_process()` process queue using DNS packet reply 'answer'.
 * It will return
 *
 * - `-1` only if `anwswer` is NULL,
 * - `0` if queue is empty, or
 * - `n` number of queue that has been processed.
 */
int Rescached::_resolver_process(DNSQuery* answer)
{
	if (!answer) {
		return -1;
	}
	if (!_queue._head) {
		return 0;
	}

	_queue.lock();

	int x = 0;
	int s = 0;
	int n_answer = answer->get_num_answer();
	int n_processed = 0;
	BNode* bnode = _queue._head;
	BNode* bnode_next = NULL;
	ResQueue* q = NULL;

	if (n_answer == 0) {
		if (DBG_LVL_IS_1) {
			dlog.out("%8s: %3d %6ds %s is empty\n", TAG_RESOLVER
				, answer->_q_type, 0, answer->_name._v);
		}
	}

	do {
		bnode_next = bnode->_right;

		q = (ResQueue*) bnode->_item;

		if (q->_qstn->_q_type != answer->_q_type) {
			goto next;
		}

		s = q->_qstn->_name.like(&answer->_name);
		if (s != 0) {
			goto next;
		}

		if (q->_qstn->_id == answer->_id) {
			_nc.insert_copy(answer, 1, 0);
		}

		queue_send_answer(q->_udp_client, q->_tcp_client, q->_qstn
				, answer);
		n_processed++;

		(ResQueue*) _queue.node_remove_unsafe(bnode);
		delete q;
		x--;
next:
		bnode = bnode_next;
		x++;
	} while (x < _queue.size());

	_queue.unlock();

	return n_processed;
}

/**
 * `_resolver_read()` will read answer from parent answer.
 *
 * There are two type of parent server connection: UDP or TCP.
 * If the connection is UDP we read it as normal.
 * If the connection is TCP, we read them and convert it to UDP because our
 * client maybe not using TCP.
 *
 * After that we pass the answer to our clients.
 */
void Rescached::_resolver_read()
{
	int s = 0;
	DNSQuery* answer = new DNSQuery ();

	if (_dns_conn_t == 0) {
		if (DBG_LVL_IS_2) {
			dlog.out("%8s: read udp.\n", TAG_RESOLVER);
		}

		s = _resolver.recv_udp(answer);
	} else {
		if (DBG_LVL_IS_2) {
			dlog.out("%8s: read tcp.\n", TAG_RESOLVER);
		}

		s = _resolver.recv_tcp(answer);

		if (s <= 0) {
			FD_CLR(_resolver._d, &_fd_all);
			_resolver.close();
		} else {
			// convert answer to UDP.
			answer->to_udp();
		}
	}

	if (DBG_LVL_IS_2) {
		dlog.out("%8s: read status %d.\n", TAG_RESOLVER, s);
	}

	if (s > 0) {
		s = _resolver_process(answer);
	}

	delete answer;
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
		// TCP: if resolver connection is open, add it to selector.
		if (1 == _dns_conn_t && _resolver._status > 0) {
			FD_SET (_resolver._d, &_fd_all);

			if (DBG_LVL_IS_2) {
				dlog.out ("adding open resolver.\n");
			}
		}

		_fd_read	= _fd_all;
		timeout.tv_sec	= _rto;
		timeout.tv_usec	= 0;

		s = select(FD_SETSIZE, &_fd_read, NULL, NULL, &timeout);
		if (s <= 0) {
			if (EINTR == errno) {
				s = 0;
				break;
			}
			queue_clean();
			continue;
		}

		if (FD_ISSET(_resolver._d, &_fd_read)) {
			_resolver_read();

		} else if (FD_ISSET(_srvr_udp._d, &_fd_read)) {
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

			s = process_client(addr, NULL, &question);
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

/**
 * @method	: Rescached::queue_clean()
 * @desc	: remove client queue that has reached timeout value.
 */
void Rescached::queue_clean()
{
	if (!_queue._head) {
		return;
	}

	int x = 0;
	double difft = 0;
	time_t t = time(NULL);
	BNode* bnode = NULL;
	BNode* bnode_next = NULL;
	ResQueue* q = NULL;

	_queue.lock();

	if (DBG_LVL_IS_1) {
		dlog.out("%8s: cleaning ...\n", TAG_QUEUE);
	}

	bnode = _queue._head;

	do {
		bnode_next = bnode->_right;

		q = (ResQueue*) bnode->_item;

		difft = difftime(t, q->_timeout);
		if (difft >= _rto) {
			if (DBG_LVL_IS_1) {
				dlog.out("%8s: timeout: %3d %s\n"
					, TAG_QUEUE
					, q->_qstn->_q_type
					, q->_qstn->_name._v);
			}

			_queue.node_remove_unsafe(bnode);
			delete q;
			x--;
		}

		bnode = bnode_next;
		x++;
	} while(x < _queue.size());

	if (DBG_LVL_IS_1) {
		dlog.out("%8s: size %d\n", TAG_QUEUE, _queue.size());
	}

	_queue.unlock();
}

int Rescached::queue_send_answer(struct sockaddr_in* udp_client
				, Socket* tcp_client
				, DNSQuery* question
				, DNSQuery* answer)
{
	int s;

	answer->set_id(question->_id);

	if (tcp_client) {
		if (answer->_bfr_type == vos::BUFFER_IS_UDP) {
			s = answer->to_tcp();
			if (s < 0) {
				return -1;
			}
		}

		tcp_client->reset();
		tcp_client->write(answer);

	} else if (udp_client) {
		if (answer->_bfr_type == vos::BUFFER_IS_UDP) {
			_srvr_udp.send_udp(udp_client, answer);
		} else {
			_srvr_udp.send_udp_raw(udp_client
						, &answer->_v[2]
						, answer->_i - 2);
		}
	} else {
		dlog.er("queue_send_answer: no client connected!\n");
		return -1;
	}

	if (DBG_LVL_IS_3) {
		_nc.dump_r();
	}

	return 0;
}

int Rescached::process_client(struct sockaddr_in* udp_client
				, Socket* tcp_client
				, DNSQuery** question)
{
	if (!(*question)) {
		return -1;
	}

	int		s;
	int		idx;
	TreeNode*	node	= NULL;
	DNSQuery*	answer	= NULL;
	NCR*		ncr	= NULL;
	int		diff	= 0;

	idx = _nc.get_answer_from_cache ((*question), &answer, &node);

	// Question is not found in cache
	if (idx < 0) {
		if (_dns_conn_t == 0) {
			s = _resolver.send_udp((*question));
		} else {
			s = _resolver.send_tcp((*question));
		}
		if (s < 0) {
			return -1;
		}

		queue_push(udp_client, tcp_client, question);
		return 0;
	}

	ncr = (NCR*) node->get_content();

	// Check if TTL outdated.
	if (answer->_attrs == vos::DNS_IS_QUERY) {
		time_t	now	= time(NULL);

		diff = (int) difftime (now, ncr->_ttl);

		// TTL is outdated.
		if (diff >= 0) {
			if (DBG_LVL_IS_1) {
				dlog.out ("%8s: %3d %6ds %s\n"
					, TAG_RENEW
					, (*question)->_q_type
					, diff
					, (*question)->_name.chars());
			}

			if (0 == _dns_conn_t) {
				s = _resolver.send_udp ((*question));
			} else {
				s = _resolver.send_tcp ((*question));
			}
			if (s < 0) {
				return -1;
			}

			queue_push(udp_client, tcp_client, question);
			return 0;
		}

		// Update answer TTL with time difference.
		diff = (int) difftime (ncr->_ttl, now);
		answer->set_rr_answer_ttl(diff);
	}

	if (DBG_LVL_IS_1) {
		const char* p = NULL;

		switch (answer->_attrs) {
		case vos::DNS_IS_QUERY:
			p = TAG_CACHED;
			break;
		case vos::DNS_IS_LOCAL:
			p = TAG_LOCAL;
			break;
		case vos::DNS_IS_BLOCKED:
			p = TAG_BLOCKED;
			break;
		}
		dlog.out("%8s: %3d %6ds %s +%d\n"
			, p
			, (*question)->_q_type
			, diff
			, (*question)->_name.chars()
			, ncr->_stat
			);
	}

	_nc.increase_stat_and_rebuild(ncr->_p_list);

	s = queue_send_answer(udp_client, tcp_client, (*question), answer);

	delete (*question);
	(*question) = NULL;

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

			process_client(NULL, client, &question);

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

	if (DBG_LVL_IS_1) {
		dlog.out ("%8s: %3d %6ds %s\n", TAG_QUEUE
			, (*question)->_q_type, 0, (*question)->_name._v);
	}

	_queue.push_tail(obj);

	return 0;
}

/**
 * @method	: Rescached::exit
 * @desc	: release Rescache object.
 */
void Rescached::exit()
{
	_running = 0;

	if (_fdata._v) {
		_nc.save(_fdata._v);
	}

	if (_fpid._v) {
		unlink(_fpid._v);
	}
}

} /* namespace::rescached */
// vi: ts=8 sw=8 tw=78:
