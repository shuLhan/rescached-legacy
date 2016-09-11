/*
 * Copyright 2010-2016 Mhd Sulhan (ms@kilabit.info)
 */

#include "Rescached.hh"

namespace rescached {

Rescached::Rescached() :
	_fdata()
,	_fdatabak()
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
,	_queue(NULL)
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
	s = dlog.open(_flog._v, 2048000);
	if (s != 0) {
		return -1;
	}

	s = File::WRITE_PID (_fpid.chars());
	if (s != 0) {
		return -1;
	}

	s = _nc.bucket_init ();
	if (s != 0) {
		return -1;
	}

	s = load_hosts (NULL, 0);
	if (s < 0) {
		return -1;
	}

	s = load_hosts_block ();
	if (s < 0) {
		return -1;
	}

	s = load_cache();
	if (s < 0) {
		return -1;
	}

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

	const char* v = cfg->get(RESCACHED_CONF_HEAD, "server.listen"
				, RESCACHED_DEF_LISTEN);
	s = _listen_addr.copy_raw(v);
	if (s != 0) {
		return -1;
	}

	List* addr_port = _listen_addr.split_by_char(':', 1);

	// (1)
	if (addr_port->_n == 1) {
		_listen_port = RESCACHED_DEF_PORT;
		return 0;
	}

	Buffer* addr = (Buffer*) addr_port->at(0);
	Buffer* port = (Buffer*) addr_port->at(1);

	_listen_addr.copy(addr);
	_listen_port = port->to_lint();

	delete addr_port;
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

	if (DBG_LVL_IS_1) {
		dlog.it("[rescached] loading config    > %s\n", fconf);
	}

	s = cfg.load(fconf);
	if (s < 0) {
		dlog.er("[rescached] cannot open config file '%s'!", fconf);
		return -1;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "file.data", RESCACHED_DATA);
	s = _fdata.copy_raw(v);
	if (s != 0) {
		return -1;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "file.data.backup"
			, RESCACHED_DATA_BAK);
	s = _fdatabak.copy_raw(v);
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

	/* environment variable replace value in config file */
	v	= getenv("RESCACHED_DEBUG");
	_dbg	= (!v) ? _dbg : atoi(v);

	if (DBG_LVL_IS_1) {
		dlog.er("[rescached] cache file        > %s\n", _fdata._v);
		dlog.er("[rescached] cache file backup > %s\n", _fdatabak._v);
		dlog.er("[rescached] pid file          > %s\n", _fpid._v);
		dlog.er("[rescached] log file          > %s\n", _flog._v);
		dlog.er("[rescached] hosts blocked     > %s\n", _fhostsblock._v);
		dlog.er("[rescached] parent address    > %s\n", _dns_parent._v);
		dlog.er("[rescached] parent connection > %s\n", _dns_conn._v);
		dlog.er("[rescached] listening on      > %s\n", _listen_addr._v);
		dlog.er("[rescached] listening on port > %d\n", _listen_port);
		dlog.er("[rescached] timeout           > %d\n", _rto);
		dlog.er("[rescached] cache maximum     > %ld\n", _nc._cache_max);
		dlog.er("[rescached] cache threshold   > %ld\n", _nc._cache_thr);
		dlog.er("[rescached] cache min TTL     > %d\n", _cache_minttl);
		dlog.er("[rescached] debug level       > %d\n", _dbg);
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

	dlog.out("[rescached] listening on %s:%d.\n", _listen_addr._v
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
int Rescached::load_hosts (const char* file, const short is_blocked)
{
	int	s	= 0;
	int	is_ipv4	= 0;
	int	addr	= 0;
	int	cnt	= 0;
	uint32_t attrs = 0;
	Buffer	fhosts;

#ifdef __unix
	if (! file) {
		fhosts.copy_raw ("/etc/hosts");
	} else {
		fhosts.copy_raw (file);
	}
#endif

	if (fhosts.is_empty ()) {
		return 1;
	}

	if (is_blocked) {
		attrs = vos::DNS_IS_BLOCKED;
	} else {
		attrs = vos::DNS_IS_LOCAL;
	}

	dlog.out ("[rescached] loading '%s'...\n", fhosts._v);

	SSVReader	reader;
	DNSQuery	qanswer;
	Record*		ip;
	Record*		r;
	Record*		c;

	reader._comment_c = '#';

	s = reader.load (fhosts._v);
	if (s != 0) {
		return -1;
	}

	r = reader._rows;
	while (r) {
		ip	= r;

		is_ipv4	= inet_pton (AF_INET, ip->_v, &addr);
		addr	= 0;

		if (is_ipv4 == 1) {
			c = ip->_next_col;
			while (c) {
				s = qanswer.create_answer (c->_v
					, (uint16_t) vos::QUERY_T_ADDRESS
					, (uint16_t) vos::QUERY_C_IN
					, INT_MAX
					, (uint16_t) ip->_i
					, ip->_v
					, attrs);

				if (s == 0) {
					_nc.insert_copy (&qanswer, 0, 1);
					cnt++;
				}

				c = c->_next_col;
			}
		}

		r = r->_next_row;
	}

	dlog.out ("[rescached] %d addresses loaded.\n", cnt);

	return 0;
}

int Rescached::load_hosts_block ()
{
	int s = 0;

	// Check blocked hosts file in current directory.
	s = File::IS_EXIST (RESCACHED_HOSTS_BLOCK);

	if (s) {
		dlog.out ("[rescached] blocked hosts: %s\n", RESCACHED_HOSTS_BLOCK);
		return load_hosts (RESCACHED_HOSTS_BLOCK, 1);
	}

	// Load blocked hosts file from config
	if (! _fhostsblock.is_empty ()) {
		s = File::IS_EXIST (_fhostsblock._v);

		if (s) {
			return load_hosts (_fhostsblock._v, 1);
		}
	}

	return 0;
}

/**
 * @method	: Rescached::load_cache
 * @return	:
 *	< 0	: success.
 *	< -1	: fail.
 * @desc	: load cache data from file.
 * Try loading the default cache file first. if it is fail or no cache
 * loaded (zero record), try to load backup file.
 */
int Rescached::load_cache()
{
	register int s;

	dlog.out("[rescached] loading cache ...\n");

	s = _nc.load(_fdata._v);
	if (s != 0 || _nc._n_cache == 0) {
		s = _nc.load(_fdatabak._v);
		if (s != 0) {
			if (ENOENT != errno) {
				return -1;
			}
		}
	}

	dlog.out("[rescached] %d records loaded.\n", _nc._n_cache);

	if (DBG_LVL_IS_3) {
		_nc.dump();
	}

	return 0;
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
	DNSQuery*		answer		= new DNSQuery ();
	DNSQuery*		question	= NULL;
	Socket*			client		= NULL;
	struct sockaddr_in*	addr		= NULL;

	while (_running) {
		// TCP: if resolver connection is open, add it to selector.
		if (1 == _dns_conn_t && _resolver._status > 0) {
			FD_SET (_resolver._d, &_fd_all);

			if (DBG_LVL_IS_2) {
				dlog.out ("[rescached] adding open resolver.\n");
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
			if (_dns_conn_t == 0) {
				if (DBG_LVL_IS_2) {
					dlog.out("[rescached] read resolver udp.\n");
				}

				s = _resolver.recv_udp (answer);
			} else {
				if (DBG_LVL_IS_2) {
					dlog.out("[rescached] read resolver tcp.\n");
				}

				s = _resolver.recv_tcp (answer);

				if (s <= 0) {
					FD_CLR (_resolver._d, &_fd_all);
					_resolver.close();
				} else {
					// convert answer to UDP.
					answer->to_udp ();
				}
			}

			if (DBG_LVL_IS_2) {
				dlog.out("[rescached] read resolver status %d.\n", s);
			}

			if (s > 0) {
				s = queue_process (answer);
			}
		} else if (FD_ISSET(_srvr_udp._d, &_fd_read)) {
			if (DBG_LVL_IS_2) {
				dlog.out("[rescached] read server udp.\n");
			}

			addr = (struct sockaddr_in*) calloc(1
							, SockAddr::IN_SIZE);
			if (!addr) {
				dlog.er("[rescached] error at allocating new address!\n");
				continue;
			}

			s = (int) _srvr_udp.recv_udp(addr);
			if (s <= 0) {
				dlog.er ("[rescached] error at receiving UDP packet!\n");
				free(addr);
				addr = NULL;
				continue;
			}

			s = DNSQuery::INIT(&question, &_srvr_udp);
			if (s < 0) {
				dlog.er("[rescached] error at initializing dnsquery!\n");
				free(addr);
				addr = NULL;
				continue;
			}

			s = process_client(addr, NULL, &question);
			if (s != 0) {
				dlog.er("[rescached] error at processing client!\n");
				delete question;
				question = NULL;
			}
			if (addr) {
				free (addr);
				addr = NULL;
			}
		} else if (FD_ISSET(_srvr_tcp._d, &_fd_read)) {
			if (DBG_LVL_IS_2) {
				dlog.out("[rescached] read server tcp.\n");
			}

			client = _srvr_tcp.accept_conn();
			if (! client) {
				dlog.er("[rescached] error at accepting client TCP connection!\n");
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
		dlog.er("[rescached] service stopped ...\n");
	}

	delete answer;

	return s;
}

/**
 * @method	: Rescached::queue_clean()
 * @desc	: remove client queue that has reached timeout value.
 */
void Rescached::queue_clean()
{
	double		x = 0;
	time_t		t = time(NULL);
	ResQueue*	q = _queue;
	ResQueue*	n = NULL;

	while (q) {
		n = q->_next;

		x = difftime(t, q->_timeout);
		if (x >= _rto) {
			ResQueue::REMOVE(&_queue, q);
		}

		q = n;
	}
}

/**
 * @method		: Rescached::queue_process
 * @param		:
 *	> answer	: pointer to DNS packet reply from parent server.
 * @return		:
 *	< 0		: success.
 *	< -1		: fail.
 * @desc		: process queue using 'answer' DNS packet reply.
 */
int Rescached::queue_process(DNSQuery* answer)
{
	if (!answer) {
		return -1;
	}

	int		s;
	ResQueue* 	q = NULL;
	ResQueue*	n = NULL;

	/* search queue by id and name only */
	q = _queue;
	while (q) {
		if (q->_qstn
		&& q->_qstn->_id == answer->_id
		&& q->_qstn->_q_type == answer->_q_type) {
			s = q->_qstn->_name.like(&answer->_name);
			if (s == 0) {
				s = _nc.insert_copy (answer, 1, 0);
				break;
			}
		}
		q = q->_next;
	}
	if (!q) {
		return -1;
	}

	s = queue_send_answer(q->_udp_client, q->_tcp_client, q->_qstn, answer);

	ResQueue::REMOVE(&_queue, q);

	/* send reply to all queue with the same name and type */
	q = _queue;
	while (q) {
		n = q->_next;

		if (q->_qstn
		&& q->_qstn->_q_type == answer->_q_type) {
			s = q->_qstn->_name.like(&answer->_name);
			if (s == 0) {
				s = queue_send_answer(q->_udp_client
							, q->_tcp_client
							, q->_qstn
							, answer);
				ResQueue::REMOVE(&_queue, q);
			}
		}
		q = n;
	}
	return 0;
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
		dlog.er("[rescached] queue_send_answer: no client connected!\n");
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
	NCR_Tree*	node	= NULL;
	DNSQuery*	answer	= NULL;
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

	// Check if TTL outdated.
	if (answer->_attrs == vos::DNS_IS_QUERY) {
		time_t	now	= time(NULL);

		diff = (int) difftime (now, node->_rec->_ttl);

		// TTL is outdated.
		if (diff >= 0) {
			if (DBG_LVL_IS_1) {
				dlog.out ("[rescached]    renew: %3d %6ds %s\n"
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
		diff = (int) difftime (node->_rec->_ttl, now);
		answer->set_rr_answer_ttl(diff);
	}

	if (DBG_LVL_IS_1) {
		dlog.out("[rescached] %8s: %3d %6ds %s +%d\n"
			, (answer->_attrs == vos::DNS_IS_BLOCKED
				? "host blocked" : "cached")
			, (*question)->_q_type
			, diff
			, (*question)->_name.chars()
			, node->_rec->_stat
			);
	}

	_nc.increase_stat_and_rebuild ((NCR_List *) node->_p_list);

	s = queue_send_answer(udp_client, tcp_client, (*question), answer);

	delete (*question);
	(*question) = NULL;

	return s;
}

int Rescached::process_tcp_client()
{
	int		s		= 0;
	Socket*		client		= NULL;
	Socket*		next		= NULL;
	DNSQuery*	question	= NULL;

	client = _srvr_tcp._clients;

	while (client) {
		next = client->_next;

		if (FD_ISSET(client->_d, &_fd_read) == 0) {
			client = next;
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
				client = next;
				continue;
			}

			process_client(NULL, client, &question);

			question = NULL;
		}

		client = next;
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

	obj = ResQueue::NEW();
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
		dlog.out ("[rescached]    queue: %3d %6ds %s\n"
			, (*question)->_q_type, 0, (*question)->_name._v);
	}

	ResQueue::PUSH(&_queue, obj);

	return 0;
}

/**
 * @method	: Rescached::exit
 * @desc	: release Rescache object.
 */
void Rescached::exit()
{
	if (DBG_LVL_IS_1) {
		dlog.er("\n[rescached] saving %d records ...\n", _nc._n_cache);
	}

	_running = 0;

	if (_fdata._v) {
		_nc.save(_fdata._v);
	}

	create_backup();

	if (_queue) {
		delete _queue;
		_queue = NULL;
	}
	if (_fpid._v) {
		unlink(_fpid._v);
	}
}

/**
 * @method	: Rescached::create_backup()
 * @return	:
 *	< 0	: success.
 *	< -1	: fail.
 * @desc	: create backup of cache file.
 *		 - do not create backup if the original file is empty.
 */
int Rescached::create_backup()
{
	int	s;
	File	r;
	File	w;

	s = r.open_ro(_fdata._v);
	if (s != 0) {
		return -1;
	}

	s = (int) r.get_size();
	if (s <= 0) {
		return 0;
	}

	s = w.open_wo(_fdatabak._v);
	if (s != 0) {
		return -1;
	}

	s = r.read();
	while (s > 0) {
		s = w.write(&r);
		if (s < 0) {
			break;
		}
		s = r.read();
	}

	return 0;
}

} /* namespace::rescached */
