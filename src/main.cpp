/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "main.hpp"

using vos::File;
using vos::Config;
using vos::SockAddr;
using vos::Socket;
using vos::SockServer;
using vos::Resolver;
using namespace rescached;

volatile sig_atomic_t	_running_	= 1;
volatile sig_atomic_t	_SIG_lock_	= 0;

static int		_rto		= RESCACHED_DEF_TIMEOUT;
static int		_srvr_port	= RESCACHED_DEF_PORT;
static int		_got_signal_	= 0;
static int		_maxfds		= 0;
static fd_set		_fd_all;
static fd_set		_fd_read;
static NameCache	_nc;
static SockServer	_srvr_tcp;
static SockServer	_srvr_udp;
static Buffer		_file_data;
static Buffer		_file_data_bak;
static Buffer		_file_log;
static Buffer		_file_pid;
static Buffer		_srvr_parent;
static Buffer		_srvr_listen;
static Resolver		_rslvr_udp;
static ResQueue*	_queue;

static void rescached_exit();

static void rescached_interrupted(int sig_num)
{
	switch (sig_num) {
	case SIGUSR1:
		/* send interrupt to select() */
		break;
	case SIGSEGV:
		if (_SIG_lock_) {
			raise(sig_num);
		}
		_SIG_lock_	= 1;
		_got_signal_	= sig_num;
		_running_	= 0;
		rescached_exit();
		_SIG_lock_	= 0;

		signal(sig_num, SIG_DFL);
		break;
	case SIGTERM:
	case SIGINT:
	case SIGQUIT:
		if (_SIG_lock_) {
			raise(sig_num);
		}
		_SIG_lock_	= 1;
		_got_signal_ 	= sig_num;
		_running_	= 0;
		rescached_exit();
		_SIG_lock_	= 0;
		exit(0);
                break;
        }
}

static void rescached_set_signal_handle()
{
	struct sigaction sig_new;

	memset(&sig_new, 0, sizeof(struct sigaction));
	sig_new.sa_handler = rescached_interrupted;
	sigemptyset(&sig_new.sa_mask);
	sig_new.sa_flags = 0;

	sigaction(SIGINT, &sig_new, 0);
	sigaction(SIGQUIT, &sig_new, 0);
	sigaction(SIGTERM, &sig_new, 0);
	sigaction(SIGSEGV, &sig_new, 0);
	sigaction(SIGUSR1, &sig_new, 0);
}

/**
 * @desc	: get user configuration from file.
 *
 * @param:
 *	> fconf : config file to read.
 *
 * @return	:
 *	< 0	: success.
 *	< -1	: fail.
 */
static int rescached_load_config(const char* fconf)
{
	int		s	= 0;
	Config		cfg;
	const char*	v	= NULL;

	if (!fconf) {
		fconf = RESCACHED_CONF;
	}

	if (DBG_LVL_IS_1) {
		dlog.it("[RESCACHED] loading config    > %s\n", fconf);
	}

	cfg.load(fconf);

	v = cfg.get(RESCACHED_CONF_HEAD, "file.data", RESCACHED_DATA);
	if (!v) {
		s = _file_data.copy_raw(RESCACHED_DATA);
	} else {
		s = _file_data.copy_raw(v);
	}
	if (s != 0) {
		return -1;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "file.data.backup");
	if (v) {
		s = _file_data_bak.copy_raw(v);
		if (s != 0) {
			return -1;
		}
	} else {
		s = _file_data_bak.copy(&_file_data);
		if (s != 0) {
			return -1;
		}

		s = _file_data_bak.append_raw(RESCACHED_DATA_BAK_EXT);
		if (s < 0) {
			return -1;
		}
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "file.pid", RESCACHED_PID);
	if (!v) {
		s = _file_pid.copy_raw(RESCACHED_PID);
	} else {
		s = _file_pid.copy_raw(v);
	}
	if (s != 0) {
		return -1;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "file.log", RESCACHED_LOG);
	if (!v) {
		s = _file_log.copy_raw(RESCACHED_LOG);
	} else {
		s = _file_log.copy_raw(v);
	}
	if (s != 0) {
		return -1;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "server.parent");
	if (!v) {
		s = _srvr_parent.copy_raw(RESCACHED_DEF_PARENT);
	} else {
		s = _srvr_parent.copy_raw(v);
	}
	if (s != 0) {
		return -1;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "server.listen", RESCACHED_DEF_LISTEN);
	if (!v) {
		s = _srvr_listen.copy_raw(RESCACHED_DEF_LISTEN);
	} else {
		s = _srvr_listen.copy_raw(v);
	}
	if (s != 0) {
		return -1;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "server.listen.port");
	if (v) {
		_srvr_port = (int) strtol(v, 0, 0);
		if (_srvr_port <= 0) {
			_srvr_port = RESCACHED_DEF_PORT;
		}
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "server.timeout");
	if (v) {
		_rto = (int) strtol(v, 0, 0);
		if (_rto <= RESCACHED_DEF_TIMEOUT) {
			_rto = RESCACHED_DEF_TIMEOUT;
		}
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "cache.max", RESCACHED_CACHE_MAX_S);
	if (v) {
		_cache_max = strtol(v, 0, 10);
		if (_cache_max <= 0) {
			_cache_max = RESCACHED_CACHE_MAX;
		}
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "cache.threshold");
	if (v) {
		_cache_thr = strtoul(v, 0, 10);
		if (_cache_thr <= 0) {
			_cache_thr = RESCACHED_DEF_THRESHOLD;
		}
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "debug");
	if (v) {
		_debug_lvl = (int) strtol(v, 0, 10);
		if (_debug_lvl < 0) {
			_debug_lvl = RESCACHED_DEF_DEBUG;
		}
	}

	return 0;
}

/**
 * @method	: rescached_init
 * @param	:
 *	> fconf : config file to read.
 * @return	:
 *	< 0	: success.
 *	< -1	: fail.
 * @desc	: initialize Rescached object.
 */
static int rescached_init(const char* fconf)
{
	int s;

	s = rescached_load_config(fconf);
	if (s != 0) {
		return -1;
	}

	s = dlog.open(_file_log._v);
	if (s != 0) {
		return -1;
	}

	if (DBG_LVL_IS_1) {
		dlog.er("[RESCACHED] cache file        > %s\n", _file_data._v);
		dlog.er("[RESCACHED] cache file backup > %s\n", _file_data_bak._v);
		dlog.er("[RESCACHED] pid file          > %s\n", _file_pid._v);
		dlog.er("[RESCACHED] log file          > %s\n", _file_log._v);
		dlog.er("[RESCACHED] parent address    > %s\n", _srvr_parent._v);
		dlog.er("[RESCACHED] listening on      > %s\n", _srvr_listen._v);
		dlog.er("[RESCACHED] listening on port > %d\n", _srvr_port);
		dlog.er("[RESCACHED] timeout           > %d\n", _rto);
		dlog.er("[RESCACHED] cache maximum     > %ld\n", _cache_max);
		dlog.er("[RESCACHED] cache threshold   > %ld\n", _cache_thr);
		dlog.er("[RESCACHED] debug level       > %d\n", _debug_lvl);
	}

	s = _srvr_udp.create_udp();
	if (s != 0) {
		return -1;
	}

	s = _srvr_udp.bind(_srvr_listen._v, _srvr_port);
	if (s != 0) {
		return -1;
	}

	s = _srvr_tcp.create();
	if (s != 0) {
		return -1;
	}

	s = _srvr_tcp.bind_listen(_srvr_listen._v, _srvr_port);
	if (s != 0)
		return s;

	if (DBG_LVL_IS_1) {
		dlog.er("[RESCACHED] loading caches    >\n");
	}

	/**
	 * try loading the default cache file first. if it is fail or no cache
	 * loaded (zero record), try to load backup file.
	 */
	s = _nc.load(_file_data._v, _cache_max);
	if (s != 0 || _nc._n_cache == 0) {
		s = _nc.load(_file_data_bak._v, _cache_max);
		if (s != 0) {
			if (ENOENT == errno) {
				return 0;
			}
			return -1;
		}
	}

	if (DBG_LVL_IS_1) {
		dlog.er("[RESCACHED] %d record loaded\n", _nc._n_cache);
		if (DBG_LVL_IS_2) {
			_nc.dump();
		}
	}

	s = _rslvr_udp.init();
	if (s < 0) {
		return -1;
	}

	s = _rslvr_udp.set_server(_srvr_parent._v);
	if (s < 0) {
		return -1;
	}

	FD_ZERO(&_fd_all);
	FD_ZERO(&_fd_read);

	FD_SET(_srvr_udp._d, &_fd_all);
	FD_SET(_srvr_tcp._d, &_fd_all);
	FD_SET(_rslvr_udp._d, &_fd_all);

	_maxfds = (_srvr_udp._d > _srvr_tcp._d ? _srvr_udp._d : _srvr_tcp._d);
	_maxfds = (_maxfds > _rslvr_udp._d ? _maxfds : _rslvr_udp._d);
	_maxfds++;

	return 0;
}

/**
 * @method	: rescached_init_write_pid
 * @return	:
 *	< 0	: success.
 *	< -1	: fail.
 * @desc	: create rescached PID file.
 */
static int rescached_init_write_pid()
{
	int	s;
	File	fpid;

	s = fpid.open_wo(_file_pid._v);
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
 * @desc	: create backup of cache file.
 *		 - do not create backup if the original file is empty.
 *
 * @return	:
 *	< 0	: success.
 *	< -1	: fail.
 */
static int rescached_create_backup()
{
	int	s;
	File	r;
	File	w;

	s = r.open_ro(_file_data._v);
	if (s != 0) {
		return -1;
	}

	s = (int) r.get_size();
	if (s <= 0) {
		return 0;
	}

	s = w.open_wo(_file_data_bak._v);
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

/**
 * @method		: queue_push
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
static int queue_push(struct sockaddr_in* udp_client, Socket* tcp_client
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

static void queue_clean()
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

static int queue_send_answer(struct sockaddr_in* udp_client
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
		dlog.er("[RESCACHED] queue_send_answer: no client connected!\n");
		return -1;
	}

	if (DBG_LVL_IS_2) {
		_nc.dump_r();
	}

	return 0;
}

/**
 * @method		: queue_process
 * @param		:
 *	> answer	: pointer to DNS packet reply from parent server.
 * @return		:
 *	< 0		: success.
 *	< -1		: fail.
 * @desc		: process queue using 'answer' DNS packet reply.
 */
static int queue_process(DNSQuery* answer)
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

static int process_client(struct sockaddr_in* udp_client
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

	dlog.out("[RESCACHED] process_client: '%s'\n", question->_name.v());

	idx = _nc.get_answer_from_cache(&node, &question->_name);

	if (idx < 0) {
		s = _rslvr_udp.send_udp(question);
		if (s < 0) {
			return -1;
		}

		queue_push(udp_client, tcp_client, question);

		return 0;
	}

	if (DBG_LVL_IS_1) {
		dlog.er("[RESCACHED] process_client: got one on cache ...\n");
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

static int process_tcp_client()
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

static int rescached_server()
{
	int			s		= 0;
	struct timeval		timeout;
	DNSQuery		answer;
	DNSQuery*		question	= NULL;
	Socket*			client		= NULL;
	struct sockaddr_in*	addr		= NULL;

	while (_running_) {
		_fd_read = _fd_all;

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

		if (FD_ISSET(_rslvr_udp._d, &_fd_read)) {
			answer.reset(vos::DNSQ_DO_ALL);

			s = _rslvr_udp.recv_udp(&answer);
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

			s = DNSQuery::INIT(&question, &_srvr_udp
						, vos::BUFFER_IS_UDP);
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
		dlog.er("[RESCACHED] udp server exit ...\n");
	}

	return s;
}

static void rescached_exit()
{
	if (DBG_LVL_IS_1) {
		dlog.er("\n[RESCACHED] saving %d records ...\n", _nc._n_cache);
	}
	if (_file_pid._v) {
		unlink(_file_pid._v);
	}
	if (_file_data._v) {
		_nc.save(_file_data._v);
	}

	rescached_create_backup();

	if (_queue) {
		delete _queue;
	}
}

int main(int argc, char *argv[])
{
	int s = 0;

	if (argc == 1) {
		s = rescached_init(NULL);
	} else if (argc == 2) {
		s = rescached_init(argv[1]);
	} else {
		dlog.er("\n Usage: rescached <rescached-config>\n ");
		s = -1;
	}
	if (s != 0) {
		goto err;
	}

	rescached_set_signal_handle();
	rescached_init_write_pid();

	rescached_server();
err:
	if (s) {
		perror(NULL);
	}
	rescached_exit();

	return s;
}
