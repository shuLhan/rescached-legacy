/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "libvos.hpp"
#include "main.hpp"
#include "Config.hpp"
#include "Resolver.hpp"
#include "NCR.hpp"
#include "NameCache.hpp"
#include "ResThread.hpp"

using vos::File;
using vos::Config;
using vos::Socket;
using vos::Resolver;
using rescached::NCR_List;
using rescached::NCR_Tree;
using rescached::NCR;
using rescached::NameCache;
using rescached::ResQueue;
using rescached::ResThread;

volatile sig_atomic_t	_running_	= 1;
volatile sig_atomic_t	_SIG_lock_	= 0;
int			RESCACHED_DEBUG	= (getenv("RESCACHED_DEBUG") == NULL)
						? 0
						: atoi(getenv("RESCACHED_DEBUG"));

Dlogger			dlog;

long int		_cache_max	= RESCACHED_CACHE_MAX;
long int		_cache_thr	= RESCACHED_DEF_THRESHOLD;
static int		_rt_max		= RESCACHED_DEF_N_THREAD;
static int		_srvr_port	= RESCACHED_DEF_PORT;
static int		_got_signal_	= 0;
static Resolver		_rslvr;
static NameCache	_nc;
static Socket		_srvr_tcp;
static Socket		_srvr_udp;
static Buffer		_file_data;
static Buffer		_file_data_bak;
static Buffer		_file_log;
static Buffer		_file_md;
static Buffer		_file_pid;
static Buffer		_srvr_parent;
static Buffer		_srvr_listen;
static ResThread	*_rt = NULL;

static void rescached_interrupted(int sig_num)
{
	switch (sig_num) {
	case SIGUSR1:
		break;
	case SIGSEGV:
		if (_SIG_lock_) {
			raise(sig_num);
		}
		_SIG_lock_	= 1;
		_got_signal_	= sig_num;
		_running_	= 0;
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
		_SIG_lock_	= 0;
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
 *	< <0	: fail.
 */
static int rescached_load_config(const char *fconf)
{
	int		s	= 0;
	Config		cfg;
	const char	*v	= NULL;

	if (!fconf) {
		fconf = RESCACHED_CONF;
	}

	if (DBG_LVL_IS_1) {
		dlog.it("[RESCACHED] loading config    > %s\n", fconf);
	}

	cfg.load(fconf);

	v = cfg.get(RESCACHED_CONF_HEAD, "file.data", RESCACHED_DATA);
	if (!v) {
		s = _file_data.init_raw(RESCACHED_DATA, 0);
	} else {
		s = _file_data.init_raw(v, 0);
	}
	if (s != 0) {
		return s;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "file.data.backup", NULL);
	if (v) {
		s = _file_data_bak.init_raw(v, 0);
		if (s != 0) {
			return s;
		}
	} else {
		s = _file_data_bak.init_raw(_file_data._v, 0);
		if (s != 0)
			return s;

		s = _file_data_bak.append_raw(RESCACHED_DATA_BAK_EXT, 0);
		if (s < 0)
			return s;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "file.metadata", RESCACHED_MD);
	if (!v) {
		s = _file_md.init_raw(RESCACHED_MD, 0);
	} else {
		s = _file_md.init_raw(v, 0);
	}
	if (s != 0) {
		return s;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "file.pid", RESCACHED_PID);
	if (!v) {
		s = _file_pid.init_raw(RESCACHED_PID, 0);
	} else {
		s = _file_pid.init_raw(v, 0);
	}
	if (s != 0) {
		return s;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "file.log", RESCACHED_LOG);
	if (!v) {
		s = _file_log.init_raw(RESCACHED_LOG, 0);
	} else {
		s = _file_log.init_raw(v, 0);
	}
	if (s != 0) {
		return s;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "server.parent", NULL);
	if (!v) {
		s = _srvr_parent.init_raw(RESCACHED_DEF_PARENT, 0);
	} else {
		s = _srvr_parent.init_raw(v, 0);
	}
	if (s != 0) {
		return s;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "server.listen", RESCACHED_DEF_LISTEN);
	if (!v) {
		s = _srvr_listen.init_raw(RESCACHED_DEF_LISTEN, 0);
	} else {
		s = _srvr_listen.init_raw(v, 0);
	}
	if (s != 0) {
		return s;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "server.listen.port", NULL);
	if (v) {
		_srvr_port = strtol(v, 0, vos::NUM_BASE_10);
		if (_srvr_port <= 0) {
			_srvr_port = RESCACHED_DEF_PORT;
		}
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "server.thread", NULL);
	if (v) {
		_rt_max = strtol(v, 0, vos::NUM_BASE_10);
		if (_rt_max <= 0) {
			_rt_max = sysconf(_SC_NPROCESSORS_CONF);
			if (_rt_max <= 1) {
				_rt_max = RESCACHED_DEF_N_THREAD;
			}
		}
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "cache.max", RESCACHED_CACHE_MAX_S);
	if (v) {
		_cache_max = strtoul(v, 0, 10);
		if (_cache_max <= 0)
			_cache_max = RESCACHED_CACHE_MAX;
	}

	v = cfg.get(RESCACHED_CONF_HEAD, "cache.threshold", NULL);
	if (v) {
		_cache_thr = strtoul(v, 0, 10);
		if (_cache_thr <= 0)
			_cache_thr = RESCACHED_DEF_THRESHOLD;
	}

	return 0;
}

/**
 * @desc: initialize Rescached object.
 *
 * @param:
 *	> fconf : config file to read.
 *
 * @return:
 *	< 0	: success.
 *	< <0	: fail.
 */
static int rescached_init(const char *fconf)
{
	int s;

	s = rescached_load_config(fconf);
	if (s != 0)
		return s;

	s = dlog.open(_file_log._v);
	if (s != 0)
		return s;

	if (DBG_LVL_IS_1) {
		dlog.er("[RESCACHED] cache file        > %s\n", _file_data._v);
		dlog.er("[RESCACHED] cache file backup > %s\n", _file_data_bak._v);
		dlog.er("[RESCACHED] cache metadata    > %s\n", _file_md._v);
		dlog.er("[RESCACHED] pid file          > %s\n", _file_pid._v);
		dlog.er("[RESCACHED] log file          > %s\n", _file_log._v);
		dlog.er("[RESCACHED] parent address    > %s\n", _srvr_parent._v);
		dlog.er("[RESCACHED] listening on      > %s\n", _srvr_listen._v);
		dlog.er("[RESCACHED] listening on port > %d\n", _srvr_port);
		dlog.er("[RESCACHED] max thread        > %d\n", _rt_max);
		dlog.er("[RESCACHED] cache maximum     > %ld\n", _cache_max);
		dlog.er("[RESCACHED] cache threshold   > %ld\n", _cache_thr);
	}

	_rslvr.init();
	_rslvr.set_server(_srvr_parent._v);

	s = _srvr_udp.create_udp();
	if (s != 0)
		return s;

	s = _srvr_udp.bind(_srvr_listen._v, _srvr_port);
	if (s != 0)
		return s;

	s = _srvr_tcp.create_tcp();
	if (s != 0)
		return s;

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
	s = _nc.load(_file_data._v, _file_md._v, _cache_max);
	if (s != 0 || _nc._n_cache == 0) {
		s = _nc.load(_file_data_bak._v, _file_md._v, _cache_max);
		if (s != 0) {
			if (s == -vos::E_FILE_OPEN)
				return 0;
			return s;
		}
	}

	if (DBG_LVL_IS_1) {
		dlog.er("[RESCACHED] %d record loaded\n", _nc._n_cache);
		if (DBG_LVL_IS_2)
			_nc.dump();
	}

	return 0;
}

static int rescached_init_write_pid()
{
	int	s;
	File	fpid;

	s = fpid.open_wo(_file_pid._v);
	if (s != 0)
		return s;

	s = getpid();

	s = fpid.appendi(s);
	if (s != 0)
		return s;

	return 0;
}

/**
 * @desc	: create backup of cache file.
 *		 - do not create backup if the original file is empty.
 *
 * @return	:
 *	< 0	: success.
 *	< !0	: fail.
 */
static int rescached_create_backup()
{
	int	s;
	File	r;
	File	w;

	s = r.open_ro(_file_data._v);
	if (s != 0) {
		return s;
	}

	s = r.get_size();
	if (s <= 0) {
		return 0;
	}

	s = w.open_wo(_file_data_bak._v);
	if (s != 0) {
		return s;
	}

	s = r.read();
	while (s > 0) {
		s = w.write(&r);
		if (s < 0)
			break;
		s = r.read();
	}

	return 0;
}

/**
 * @return	:
 *	< 0	: success.
 *	< !0	: fail.
 */
static int rescached_exit()
{
	int s = 0;

	if (DBG_LVL_IS_1) {
		dlog.er("\n[RESCACHED] saving caches ...\n");
	}

	if (_file_pid._v) {
		unlink(_file_pid._v);
	}

	if (_file_data._v) {
		s = _nc.save(_file_data._v, _file_md._v);
		if (s != 0)
			return s;
	}

	s = rescached_create_backup();

	return s;
}

static int process_tcp_clients(fd_set *tcp_fd_all, fd_set *tcp_fd_read)
{
	int		rqt_idx		= 0;
	int		s		= 0;
	Socket		*unused_client	= NULL;
	Socket		*client		= NULL;
	Socket		*next		= NULL;
	DNSQuery	*question	= NULL;

	_srvr_tcp.lock_client();
	client			= _srvr_tcp._clients;
	_srvr_tcp._clients	= NULL;
	_srvr_tcp.unlock_client();

	while (client) {
		next		= client->_next;
		client->_next	= NULL;
		if (next) {
			next->_prev = NULL;
		}

		s = FD_ISSET(client->_d, tcp_fd_read);
		if (0 == s) {
			unused_client = Socket::ADD_CLIENT(unused_client,
								client);
			client = next;
			continue;
		}

		client->reset();
		s = client->read();

		/* client close connection */
		if (s == 0) {
			FD_CLR(client->_d, tcp_fd_all);

			_rt[rqt_idx].push_query_r(NULL, client, NULL);
			rqt_idx = (rqt_idx + 1) % _rt_max;
		} else {
			s = DNSQuery::INIT(&question, client, vos::BUFFER_IS_TCP);
			if (s != 0) {
				client = client->_next;
				continue;
			}
			_rt[rqt_idx].push_query_r(NULL, client, question);
			rqt_idx = (rqt_idx + 1) % _rt_max;
			question = NULL;
		}
		client = next;
	}

	_srvr_tcp.add_client_r(unused_client);

	return s;
}

/**
 * @desc	: run tcp server on its own process.
 */
static void * rescached_tcp_server(void *arg)
{
	int		s		= 0;
	int		maxfds		= 0;
	fd_set		tcp_fd_all;
	fd_set		tcp_fd_read;
	Socket		*client		= NULL;
	ResThread	*rqt		= (ResThread *) arg;

	FD_ZERO(&tcp_fd_all);
	FD_ZERO(&tcp_fd_read);

	FD_SET(_srvr_tcp._d, &tcp_fd_all);
	maxfds = _srvr_tcp._d + 1;

	while (1) {
		if (!rqt->is_still_running_r()) {
			break;
		}

		tcp_fd_read = tcp_fd_all;

		if (DBG_LVL_IS_1) {
			dlog.er("[RESCACHED] tcp: waiting for client...\n");
		}

		s = select(maxfds, &tcp_fd_read, NULL, NULL, NULL);
		if (s <= 0) {
			continue;
		}

		if (FD_ISSET(_srvr_tcp._d, &tcp_fd_read)) {
			client = _srvr_tcp.accept_conn();
			if (! client)
				continue;

			if (client->_d > maxfds) {
				maxfds = client->_d;
			}
			FD_SET(client->_d, &tcp_fd_all);

			tcp_fd_read = tcp_fd_all;

			process_tcp_clients(&tcp_fd_all, &tcp_fd_read);
		} else {
			/* must be tcp clients */
			tcp_fd_read = tcp_fd_all;
			process_tcp_clients(&tcp_fd_all, &tcp_fd_read);
		}
	}

	if (DBG_LVL_IS_1) {
		dlog.er("[RESCACHED] tcp server exit ...\n");
	}

	return (0);
}

static int rescached_udp_server()
{
	int		s		= 0;
	int		maxfds		= 0;
	int		rqt_idx		= 0;
	fd_set		udp_fd_all;
	fd_set		udp_fd_read;
	DNSQuery	*question	= NULL;
	struct sockaddr	*addr		= NULL;

	FD_ZERO(&udp_fd_all);
	FD_ZERO(&udp_fd_read);

	FD_SET(_srvr_udp._d, &udp_fd_all);
	maxfds = _srvr_udp._d + 1;

	while (_running_) {
		udp_fd_read = udp_fd_all;

		if (DBG_LVL_IS_1) {
			dlog.er("[RESCACHED] udp: waiting for client...\n");
		}

		s = select(maxfds, &udp_fd_read, NULL, NULL, NULL);
		if (s <= 0) {
			if (s < 0 && errno != EINTR)
				s = -vos::E_SOCK_SELECT;
			else
				s = 0;
			continue;
		}

		if (!FD_ISSET(_srvr_udp._d, &udp_fd_read))
			continue;

		addr = (struct sockaddr *) calloc(1, sizeof(struct sockaddr));
		if (!addr)
			continue;

		s = _srvr_udp.recv_udp(addr);
		if (s <= 0) {
			free(addr);
			continue;
		}

		s = DNSQuery::INIT(&question, &_srvr_udp, vos::BUFFER_IS_UDP);
		if (s != 0) {
			free(addr);
			continue;
		}

		_rt[rqt_idx].push_query_r(addr, NULL, question);
		rqt_idx = (rqt_idx + 1) % _rt_max;
	}

	if (DBG_LVL_IS_1) {
		dlog.er("[RESCACHED] udp server exit ...\n");
	}

	return s;
}

static void process_queue(ResQueue *queue, Socket *udp_server,
				Resolver *rslvr, DNSQuery *answer,
				Buffer *bfr)
{
	int			s		= 0;
	int			idx		= 0;
	uint16_t		len		= 0;
	Buffer			*bfr_answer	= NULL;
	DNSQuery		*question	= NULL;
	DNSQuery		*p_answer	= NULL;
	NCR_Tree		*node		= NULL;
	Socket			*tcp_client	= NULL;
	struct sockaddr		*udp_client	= NULL;

	udp_client	= queue->_udp_client;
	tcp_client	= queue->_tcp_client;
	question	= queue->_qstn;

	/* client close connection */
	if (!question) {
		if (tcp_client) {
			delete tcp_client;
		}
		return;
	}

	question->extract_header();
	question->extract_question();

	if (DBG_LVL_IS_1) {
		dlog.er(">> QUERY: %s\n", question->_name._v);
	}

	idx = _nc.get_answer_from_cache_r(&node, &question->_name);

	if (idx < 0) {
		answer->reset(vos::DNSQ_DO_ALL);

		s = rslvr->send_query(question, answer);
		if (s != 0) {
			return;
		}

		s = _nc.insert_raw_r(question->_bfr_type, &question->_name,
					question->_bfr, answer->_bfr);

		if (s == 0 && DBG_LVL_IS_1) {
			dlog.er("[RESCACHED] inserting '%s' (%ld)\n",
				question->_name._v, _nc._n_cache);
		}

		/* keep send the answer even if error at inserting answer to
		 * cache */
		if (answer->_bfr_type == vos::BUFFER_IS_TCP) {
			tcp_client->reset();
			tcp_client->send(answer->_bfr);
		} else {
			udp_server->send_udp(udp_client, answer->_bfr);
		}
	} else {
		p_answer = node->_rec->_answ;
		p_answer->set_id(question->_id);

		bfr_answer = p_answer->_bfr;

		if (DBG_LVL_IS_1) {
			dlog.er("[RESCACHED] udp: got one on cache ...\n");
		}

		if (question->_bfr_type == vos::BUFFER_IS_UDP) {
			if (!udp_client)
				return;

			if (p_answer->_bfr_type == vos::BUFFER_IS_UDP) {
				udp_server->send_udp(udp_client, bfr_answer);
			} else {
				udp_server->send_udp_raw(udp_client,
							&bfr_answer->_v[2],
							bfr_answer->_i - 2);
			}
		} else if (question->_bfr_type == vos::BUFFER_IS_TCP) {
			if (!tcp_client)
				return;

			if (p_answer->_bfr_type == vos::BUFFER_IS_UDP) {
				s = bfr->resize(bfr_answer->_i + 2);
				if (s != 0)
					return;

				bfr->reset();

				len = htons(bfr_answer->_i);
				memset(&bfr->_v[0], '\0', 2);
				memcpy(&bfr->_v[0], &len, 2);
				bfr->_i = 2;
				bfr->append(bfr_answer);
				bfr_answer = bfr;
			}

			tcp_client->reset();
			tcp_client->send(bfr_answer);

			_srvr_tcp.add_client_r(tcp_client);
		} else {
			fprintf(stderr, "[RESCACHED] unknown buffer type %d\n",
							question->_bfr_type);
			return;
		}

		_nc.increase_stat_and_rebuild_r((NCR_List *) node->_p_list);
	}
	if (DBG_LVL_IS_2) {
		_nc.dump_r();
	}
}

static void * rescached_client_handle(void *arg)
{
	Resolver	rslvr;
	DNSQuery	answer;
	Buffer		bfr;
	Socket		udp_server;
	ResQueue	*queue		= NULL;
	ResQueue	*next		= NULL;
	ResThread	*rt		= NULL;

	rt = (ResThread *) arg; 

	rslvr.init();
	rslvr.set_server(_srvr_parent._v);

	udp_server._d = _srvr_udp._d;

	answer.init(NULL);

	while (1) {
		rt->wait();

		rt->lock();
		if (!rt->_running) {
			rt->unlock();
			break;
		}
		queue		= rt->_q_query;
		rt->_q_query	= NULL;
		rt->unlock();

		while (queue) {
			next		= queue->_next;
			queue->_next	= NULL;

			process_queue(queue, &udp_server, &rslvr, &answer,
					&bfr);
			delete queue;

			queue = next;
		}
	}

	udp_server._d = 0;

	return ((void *) 0);
}

static int rescached_start_client_handle()
{
	int i = 0;
	int s = 0;

	_rt = new ResThread[_rt_max + 1];

	if (!_rt) {
		return -vos::E_MEM;
	}

	for (i = 0; i < _rt_max; i++) {
		s = pthread_create(&_rt[i]._id, NULL,
					rescached_client_handle,
					(void *) &_rt[i]);
		if (s != 0) {
			return s;
		}
	}

	return 0;
}

static void rescached_stop_client_handle()
{
	int i = 0;

	for (; i < _rt_max; ++i) {
		if (_rt && _rt[i]._id) {
			_rt[i].set_running_r(0);
			_rt[i].wakeup();
			pthread_join(_rt[i]._id, NULL);
		}
	}
}

static int rescached_start_tcp_server(ResThread *rqt)
{
	int		s;

	rqt->set_running_r(_running_);

	s = pthread_create(&rqt->_id, NULL, rescached_tcp_server,
				(void *) rqt);

	return s;
}

static void rescached_stop_tcp_server(ResThread *rqt)
{
	rqt->set_running_r(0);
	pthread_kill(rqt->_id, SIGUSR1);
	pthread_join(rqt->_id, NULL);
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
		s = -vos::E_INVALID_PARAM;
	}
	if (s != 0) {
		goto err;
	}

	rescached_set_signal_handle();
	rescached_init_write_pid();

	s = rescached_start_client_handle();
	if (s) {
		goto err;
	}

	s = rescached_start_tcp_server(&_rt[_rt_max]);
	if (s) {
		goto err;
	}

	rescached_udp_server();

	rescached_stop_tcp_server(&_rt[_rt_max]);
err:
	rescached_stop_client_handle();

	if (s) {
		if (s < 0) {
			s = -s;
		}
		if (s > 0 && s < vos::N_ERRCODE) {
			dlog.er(vos::_errmsg[s]);
		}
	}

	if (_rt) {
		delete[] _rt;
	}
	rescached_exit();

	return s;
}
