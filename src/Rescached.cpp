/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "Rescached.hpp"

namespace rescached {

Rescached::Rescached() :
	_fdata()
,	_fdatabak()
,	_flog()
,	_fpid()
,	_dns_parent()
,	_listen_addr()
,	_listen_port(RESCACHED_DEF_PORT)
,	_rto(RESCACHED_DEF_TIMEOUT)
,	_resolver()
,	_srvr_udp()
,	_srvr_tcp()
,	_fd_all()
,	_fd_read()
,	_maxfds(0)
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

	s = dlog.open(_flog._v);
	if (s != 0) {
		return -1;
	}

	s = write_pid();
	if (s != 0) {
		return -1;
	}

	s = bind();
	if (s != 0) {
		return -1;
	}

	s = load_cache();

	return s;
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

	v = cfg.get(RESCACHED_CONF_HEAD, "server.parent"
			, RESCACHED_DEF_PARENT);
	s = _dns_parent.copy_raw(v);
	if (s != 0) {
		return -1;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "server.listen"
			, RESCACHED_DEF_LISTEN);
	s = _listen_addr.copy_raw(v);
	if (s != 0) {
		return -1;
	}

	_listen_port = (int) cfg.get_number(RESCACHED_CONF_HEAD
						, "server.listen.port"
						, RESCACHED_DEF_PORT);
	if (_listen_port <= 0) {
		_listen_port = RESCACHED_DEF_PORT;
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

	_cache_mode = (int) cfg.get_number(RESCACHED_CONF_HEAD, "cache.mode"
					, RESCACHED_DEF_MODE);
	if (_cache_mode <= 0) {
		_cache_mode = RESCACHED_DEF_MODE;
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
		dlog.er("[rescached] parent address    > %s\n", _dns_parent._v);
		dlog.er("[rescached] listening on      > %s\n", _listen_addr._v);
		dlog.er("[rescached] listening on port > %d\n", _listen_port);
		dlog.er("[rescached] timeout           > %d\n", _rto);
		dlog.er("[rescached] cache maximum     > %ld\n", _nc._cache_max);
		dlog.er("[rescached] cache threshold   > %ld\n", _nc._cache_thr);
		dlog.er("[rescached] debug level       > %d\n", _dbg);
	}

	return 0;
}

/**
 * @method	: Rescached::write_pid
 * @return	:
 *	< 0	: success.
 *	< -1	: fail.
 * @desc	: create rescached PID file.
 */
int Rescached::write_pid()
{
	int	s;
	File	fpid;

	s = fpid.open_wo(_fpid._v);
	if (s != 0) {
		return -1;
	}

	s = getpid();

	s = fpid.appendi(s);
	if (s != 0) {
		return -1;
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
 */
int Rescached::bind()
{
	register int s;

	/* start Resolver first */
	s = _resolver.init();
	if (s < 0) {
		return -1;
	}

	s = _resolver.set_server(_dns_parent._v);
	if (s < 0) {
		return -1;
	}

	s = _srvr_udp.create_udp();
	if (s != 0) {
		return -1;
	}

	s = _srvr_udp.bind(_listen_addr._v, _listen_port);
	if (s != 0) {
		return -1;
	}

	s = _srvr_tcp.create();
	if (s != 0) {
		return -1;
	}

	s = _srvr_tcp.bind_listen(_listen_addr._v, _listen_port);
	if (s != 0) {
		return -1;
	}

	FD_ZERO(&_fd_all);
	FD_ZERO(&_fd_read);

	FD_SET(_srvr_udp._d, &_fd_all);
	FD_SET(_srvr_tcp._d, &_fd_all);
	FD_SET(_resolver._d, &_fd_all);

	_maxfds = (_srvr_udp._d > _srvr_tcp._d ? _srvr_udp._d : _srvr_tcp._d);
	_maxfds = (_maxfds > _resolver._d ? _maxfds : _resolver._d);
	_maxfds++;

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

	s = _nc.load(_fdata._v);
	if (s != 0 || _nc._n_cache == 0) {
		s = _nc.load(_fdatabak._v);
		if (s != 0) {
			if (ENOENT != errno) {
				return -1;
			}
		}
	}

	if (DBG_LVL_IS_1) {
		dlog.er("[rescached] %d record loaded\n", _nc._n_cache);
		if (DBG_LVL_IS_2) {
			_nc.dump();
		}
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
	DNSQuery		answer;
	DNSQuery*		question	= NULL;
	Socket*			client		= NULL;
	struct sockaddr_in*	addr		= NULL;

	while (_running) {
		_fd_read	= _fd_all;
		timeout.tv_sec	= _rto;
		timeout.tv_usec	= 0;

		s = select(_maxfds, &_fd_read, NULL, NULL, &timeout);
		if (s <= 0) {
			if (EINTR == errno) {
				s = 0;
				break;
			}
			queue_clean();
			continue;
		}

		if (FD_ISSET(_resolver._d, &_fd_read)) {
			answer.reset(vos::DNSQ_DO_ALL);

			s = _resolver.recv_udp(&answer);
			if (s >= 0) {
				s = queue_process(&answer);
			}
		} else if (FD_ISSET(_srvr_udp._d, &_fd_read)) {
			addr = (struct sockaddr_in*) calloc(1
							, SockAddr::IN_SIZE);
			if (!addr) {
				continue;
			}

			s = (int) _srvr_udp.recv_udp(addr);
			if (s <= 0) {
				free(addr);
				continue;
			}

			s = DNSQuery::INIT(&question, &_srvr_udp);
			if (s < 0) {
				free(addr);
				continue;
			}

			s = process_client(addr, NULL, question);
		} else if (FD_ISSET(_srvr_tcp._d, &_fd_read)) {
			client = _srvr_tcp.accept_conn();
			if (! client) {
				continue;
			}

			FD_SET(client->_d, &_fd_all);

			if (client->_d >= _maxfds) {
				_maxfds = client->_d + 1;
			}

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
		if (q->_qstn && q->_qstn->_id == answer->_id) {
			s = q->_qstn->_name.like(&answer->_name);
			if (s == 0) {
				s = _nc.insert_raw(&answer->_name, answer);
				break;
			}
		}
		q = q->_next;
	}
	if (!q) {
		/* search queue by name only */
		q = _queue;
		while (q) {
			if (q->_qstn) {
				s = q->_qstn->_name.like(&answer->_name);
				if (s == 0) {
					s = _nc.insert_raw(&answer->_name
								, answer);
					break;
				}
			}
			q = q->_next;
		}
		if (!q) {
			return 0;
		}
	}

	s = queue_send_answer(q->_udp_client, q->_tcp_client, q->_qstn, answer);

	ResQueue::REMOVE(&_queue, q);

	/* send reply to all queue with the same name */
	q = _queue;
	while (q) {
		n = q->_next;

		if (q->_qstn) {
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

	if (DBG_LVL_IS_2) {
		_nc.dump_r();
	}

	return 0;
}

int Rescached::process_client(struct sockaddr_in* udp_client
				, Socket* tcp_client
				, DNSQuery* question)
{
	if (!question) {
		return -1;
	}

	int		s;
	int		idx;
	NCR_Tree*	node	= NULL;
	DNSQuery*	answer	= NULL;

	question->extract_header();
	question->extract_question();

	if (DBG_LVL_IS_1) {
		dlog.out("[rescached] process_client: '%s'\n"
			, question->_name.v());
	}

	idx = _nc.get_answer_from_cache(&node, &question->_name);

	if (idx < 0) {
		s = _resolver.send_udp(question);
		if (s < 0) {
			return -1;
		}

		queue_push(udp_client, tcp_client, question);
		return 0;
	}
	if (_cache_mode == CACHE_IS_TEMPORARY) {
		time_t	now	= time(NULL);
		double	diff	= difftime(now, node->_rec->_ttl);

		if (DBG_LVL_IS_2) {
			dlog.out(
"[rescached] process_client: %ld - %ld = %f\n", now, node->_rec->_ttl
, diff);
		}

		if (diff > 0) {
			if (DBG_LVL_IS_2) {
				dlog.out(
"[rescached] process_client: '%s' cache is old, renewed...\n"
, question->_name.v());
			}

			s = _resolver.send_udp(question);
			if (s < 0) {
				return -1;
			}

			queue_push(udp_client, tcp_client, question);
			return 0;
		}
	}

	if (DBG_LVL_IS_1) {
		dlog.er("[rescached] process_client: got one on cache ...\n");
	}

	_nc.increase_stat_and_rebuild((NCR_List *) node->_p_list);
	answer = node->_rec->_answ;

	s = queue_send_answer(udp_client, tcp_client, question, answer);

	if (udp_client) {
		free(udp_client);
	}
	delete question;

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

			process_client(NULL, client, question);

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
			, DNSQuery* question)
{
	ResQueue* obj = NULL;

	obj = new ResQueue();
	if (!obj) {
		return -1;
	}

	obj->_udp_client	= udp_client;
	obj->_tcp_client	= tcp_client;
	obj->_qstn		= question;

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

	_running = 0;
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
