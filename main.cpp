/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include <signal.h>
#include "main.hpp"
#include "Error.hpp"
#include "Config.hpp"
#include "Resolver.hpp"
#include "NCR.hpp"
#include "NameCache.hpp"

using vos::Error;
using vos::File;
using vos::Config;
using vos::Socket;
using vos::Resolver;
using rescached::NCR_List;
using rescached::NCR_Tree;
using rescached::NCR;
using rescached::NameCache;


volatile sig_atomic_t	_running_	= 1;
volatile sig_atomic_t	_SIG_lock_	= 0;
int			RESCACHED_DEBUG	= (getenv("RESCACHED_DEBUG") == NULL)
						? 0
						: atoi(getenv("RESCACHED_DEBUG"));

Dlogger			dlog;

long int		_cache_max	= RESCACHED_CACHE_MAX;
long int		_cache_thr	= RESCACHED_DEF_THRESHOLD;
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

static void rescached_interrupted(int sig_num)
{
	if (_SIG_lock_)
		raise(sig_num);

	_got_signal_ = sig_num;
	switch (sig_num) {
	case SIGSEGV:
		_SIG_lock_	= 1;
		_running_	= 0;
		_SIG_lock_	= 0;

		signal(sig_num, SIG_DFL);
		break;
	case SIGTERM:
	case SIGINT:
	case SIGQUIT:
		_SIG_lock_	= 1;
		_running_	= 0;
		_SIG_lock_	= 0;
                break;
	case SIGUSR1:
		_SIG_lock_	= 1;
		_nc.dump();
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
		dlog.er("[RESCACHED] loading config    > %s\n", fconf);
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
		dlog.er("[RESCACHED] cache maximum     > %ld\n", _cache_max);
		dlog.er("[RESCACHED] cache threshold   > %ld\n", _cache_thr);
	}

	_rslvr.init();
	_rslvr.set_server(_srvr_parent._v);

	s = _srvr_udp.create_udp();
	if (s != 0)
		return s;

	s = _srvr_udp.bind(_srvr_listen._v, RESCACHED_DEF_PORT);
	if (s != 0)
		return s;

	s = _srvr_tcp.create_tcp();
	if (s != 0)
		return s;

	s = _srvr_tcp.bind_listen(_srvr_listen._v, RESCACHED_DEF_PORT);
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

	s = fpid.appendi(s, 10);
	if (s != 0)
		return s;

	return 0;
}

static int process_tcp_clients(DNSQuery *dns_qst, DNSQuery *dns_ans,
				fd_set *tcp_fd_all, fd_set *tcp_fd_read)
{
	int		idx		= 0;
	int		s		= 0;
	uint16_t	len		= 0;
	Buffer		*bfr_ans	= NULL;
	Buffer		*bfr		= NULL;
	NCR		*ncr_ans	= NULL;
	NCR_Tree	*node		= NULL;
	Socket		*client		= NULL;
	Socket		*p		= NULL;

	client = _srvr_tcp._clients;
	while (client) {
		s = FD_ISSET(client->_d, tcp_fd_read);
		if (0 == s) {
			goto next;
		}

		client->reset();
		s = client->read();

		/* client close connection */
		if (s == 0) {
			if (DBG_LVL_IS_1) {
				dlog.er("[RESCACHED] tcp: removing " \
					"client %d\n", client->_d);
			}
			FD_CLR(client->_d, tcp_fd_all);
			p = client->_next;
			_srvr_tcp.remove_client(client);
			client = p;
			continue;
		}

		dns_qst->set_buffer(client, vos::BUFFER_IS_TCP);
		dns_qst->extract_header();
		dns_qst->extract_question();

		if (DBG_LVL_IS_1) {
			dlog.er(">> TCP QUERY: %s\n", dns_qst->_name._v);
		}

		idx = _nc.get_answer_from_cache(&node, &dns_qst->_name);
		if (idx < 0) {
			s = _rslvr.send_query_tcp(dns_qst, dns_ans);
			if (s != 0) {
				goto next;
			}

			s = _nc.insert_raw(vos::BUFFER_IS_TCP, &dns_qst->_name,
						dns_qst->_bfr, dns_ans->_bfr);
			if (s != 0) {
				goto next;
			}
			if (DBG_LVL_IS_1) {
				dlog.er("[RESCACHED] inserting '%s' (%ld)\n",
					dns_qst->_name._v, _nc._n_cache);
			}

			client->reset();
			client->send(dns_ans->_bfr);

			_nc.dump();
		} else {
			ncr_ans = node->_rec;
			ncr_ans->_answ->set_id(dns_qst->_id);

			if (DBG_LVL_IS_1) {
				dlog.er("[RESCACHED] tcp: got one on cache ...\n");
			}

			bfr_ans = ncr_ans->_answ->_bfr;

			if (vos::BUFFER_IS_UDP == ncr_ans->_type) {
				if (bfr) {
					delete bfr;
					bfr = NULL;
				}
				s = Buffer::INIT_SIZE(&bfr, bfr_ans->_i + 2);
				if (s != 0)
					goto err;

				len = htons(bfr_ans->_i);
				memset(&bfr->_v[0], '\0', 2);
				memcpy(&bfr->_v[0], &len, 2);
				bfr->_i = 2;
				bfr->append(bfr_ans);
				bfr_ans = bfr;
			}

			client->reset();
			s = client->send_raw(bfr_ans->_v, bfr_ans->_i);
			if (s <= 0) {
				if (DBG_LVL_IS_1) {
					dlog.er("[RESCACHED] tcp: send to " \
						"client '%d' failed\n",
						client->_d);
				}
				FD_CLR(client->_d, tcp_fd_all);
				p = client->_next;
				_srvr_tcp.remove_client(client);
				client = p;
				continue;
			}

			ncr_ans->_stat++;
			_nc._buckets[idx]._v = NCR_Tree::REBUILD(
							_nc._buckets[idx]._v,
							node);
			NCR_List::REBUILD(&_nc._cachel,
						(NCR_List *) node->_p_list);

			if (DBG_LVL_IS_2 && _nc._buckets[idx]._v) {
				_nc._buckets[idx]._v->dump_tree(0);
			}
		}
next:
		client = client->_next;
	}
err:
	if (bfr) {
		delete bfr;
	}
	return s;
}

static int process_udp_clients(DNSQuery *dns_qst, DNSQuery *dns_ans)
{
	int		idx		= 0;
	int		s		= 0;
	struct sockaddr	addr;
	Buffer		*bfr_ans	= NULL;
	NCR		*ncr_ans	= NULL;
	NCR_Tree	*node		= NULL;

	s = _srvr_udp.recv_udp(&addr);
	if (s <= 0)
		return s;

	dns_qst->set_buffer(&_srvr_udp, vos::BUFFER_IS_UDP);
	dns_qst->extract_header();
	dns_qst->extract_question();

	if (DBG_LVL_IS_1) {
		dlog.er(">> UDP QUERY: %s\n", dns_qst->_name._v);
	}

	idx = _nc.get_answer_from_cache(&node, &dns_qst->_name);
	if (idx < 0) {
		s = _rslvr.send_query_udp(dns_qst, dns_ans);
		if (s != 0) {
			goto out;
		}

		s = _nc.insert_raw(vos::BUFFER_IS_UDP,
					&dns_qst->_name, dns_qst->_bfr,
					dns_ans->_bfr);
		if (s != 0) {
			goto out;
		}
		if (DBG_LVL_IS_1) {
			dlog.er("[RESCACHED] inserting '%s' (%ld)\n",
				dns_qst->_name._v, _nc._n_cache);
		}

		_srvr_udp.send_udp(&addr, dns_ans->_bfr);
	} else {
		ncr_ans = node->_rec;
		ncr_ans->_answ->set_id(dns_qst->_id);
		bfr_ans = ncr_ans->_answ->_bfr;

		if (DBG_LVL_IS_1) {
			dlog.er("[RESCACHED] udp: got one on cache ...\n");
		}

		if (vos::BUFFER_IS_UDP == ncr_ans->_type) {
			s = _srvr_udp.send_udp(&addr, bfr_ans);
		} else if (vos::BUFFER_IS_TCP == ncr_ans->_type) {
			s = _srvr_udp.send_udp_raw(&addr, &bfr_ans->_v[2],
							bfr_ans->_i - 2);
		} else {
			dlog.er("[RESCACHED] udp: unknown buffer type!\n");
			goto out;
		}

		/* rebuild index */
		ncr_ans->_stat++;
		_nc._buckets[idx]._v = NCR_Tree::REBUILD(_nc._buckets[idx]._v,
								node);
		NCR_List::REBUILD(&_nc._cachel, (NCR_List *) node->_p_list);

		if (DBG_LVL_IS_2 && _nc._buckets[idx]._v) {
			_nc._buckets[idx]._v->dump_tree(0);
		}
	}
out:
	_srvr_udp.reset();

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

/**
 * @desc	: run tcp server on its own process.
 */
static void * tcp_start(void *arg)
{
	int		s		= 0;
	int		maxfds		= 0;
	fd_set		tcp_fd_all;
	fd_set		tcp_fd_read;
	DNSQuery	dns_qst;
	DNSQuery	dns_ans;
	Socket		*client		= NULL;

	FD_ZERO(&tcp_fd_all);
	FD_ZERO(&tcp_fd_read);

	FD_SET(_srvr_tcp._d, &tcp_fd_all);
	maxfds = _srvr_tcp._d + 1;

	while (_running_) {
		tcp_fd_read = tcp_fd_all;

		if (DBG_LVL_IS_1) {
			dlog.er("[RESCACHED] tcp: waiting for client...\n");
		}

		s = select(maxfds, &tcp_fd_read, NULL, NULL, NULL);
		if (s <= 0) {
			if (s < 0 && errno != EINTR)
				s = -vos::E_SOCK_SELECT;
			else
				s = 0;
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

			process_tcp_clients(&dns_qst, &dns_ans, &tcp_fd_all,
						&tcp_fd_read);
		} else {
			/* must be tcp clients */
			tcp_fd_read = tcp_fd_all;
			process_tcp_clients(&dns_qst, &dns_ans, &tcp_fd_all,
						&tcp_fd_read);
		}
	}

	if (0) {
		arg = arg;
	}

	if (DBG_LVL_IS_1) {
		dlog.er("[RESCACHED] tcp server exit ...\n");
	}

	return ((void *) s);
}

static int udp_start()
{
	int		s		= 0;
	int		maxfds		= 0;
	fd_set		udp_fd_all;
	fd_set		udp_fd_read;
	DNSQuery	dns_qst;
	DNSQuery	dns_ans;

	FD_ZERO(&udp_fd_all);
	FD_ZERO(&udp_fd_read);

	FD_SET(_srvr_udp._d, &udp_fd_all);
	maxfds = _srvr_udp._d + 1;

	rescached_set_signal_handle();
	rescached_init_write_pid();

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

		if (FD_ISSET(_srvr_udp._d, &udp_fd_read)) {
			process_udp_clients(&dns_qst, &dns_ans);
		}
	}

	if (DBG_LVL_IS_1) {
		dlog.er("[RESCACHED] udp server exit ...\n");
	}

	return s;
}

int main(int argc, char *argv[])
{
	int		s		= 0;
	int		tcp_th_stat	= 0;
	pthread_t	tcp_th;

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

	s = pthread_create(&tcp_th, NULL, tcp_start, NULL);
	if (s != 0) {
		goto err;
	}

	udp_start();

	pthread_kill(tcp_th, _got_signal_);

	s = pthread_join(tcp_th, (void **) &tcp_th_stat);
err:
	if (s) {
		if (s > 0 || s < vos::N_ERRCODE) {
			Error e;
			e.init(s, NULL);
			e.print();
		}
	}

	rescached_exit();

	return s;
}
