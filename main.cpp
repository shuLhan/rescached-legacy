/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include <signal.h>
#include "Resolver.hpp"
#include "NameCache.hpp"

using vos::Socket;
using vos::DNSQuery;
using vos::Resolver;
using rescached::NameCache;
using rescached::NCR;
/*
#define	DNS_SERVERS		"209.128.95.2, 4.2.2.1, 69.111.95.106"
*/
#define	DNS_SERVERS		"222.124.204.34, 203.130.196.5"
#define	NAMECACHE_FILE		"namecache.vos"
#define	NAMECACHE_MD		"namecache.vmd"
#define	RESCACHED_DEF_ADDR	"127.0.0.1"
#define	RESCACHED_DEF_PORT	5333

volatile sig_atomic_t	_running_	= 1;
volatile sig_atomic_t	_SIG_lock_	= 0;

static void rescached_interrupted(int sig_num)
{
	if (_SIG_lock_)
		raise(sig_num);

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
}

static void process_tcp_clients(Resolver *resolver, NameCache *nc, Socket *srvr, fd_set *allfds)
{
	int		s;
	DNSQuery	*dns_qst	= new DNSQuery();
	DNSQuery	*dns_ans	= NULL;
	NCR		*ncr_ans	= NULL;
	Socket		*client		= NULL;
	Socket		*p		= NULL;

	client = srvr->_clients;
	while (client) {
		s = FD_ISSET(client->_d, &srvr->_readfds);
		if (!s) {
			goto next;
		}

		s = client->read();

		/* client close connection */
		if (s == 0) {
			FD_CLR(client->_d, allfds);

			p = client->_next;
			srvr->remove_client(client);
			client = p;
			continue;
		}

		dns_qst->extract(client, vos::BUFFER_IS_TCP);

		ncr_ans = nc->get_answer_from_cache(&dns_qst->_name);
		if (! ncr_ans) {
			dns_ans = new DNSQuery();

			s = resolver->send_query_tcp(dns_qst,
						(DNSQuery **) &dns_ans);
			if (s) {
				delete dns_ans;
				goto next;
			}

			if (dns_ans->_n_ans) {
				nc->insert(&dns_qst->_name, dns_qst->_bfr,
						dns_ans->_bfr);
			}

			client->send(dns_ans->_bfr);

			delete dns_ans;
		} else {
			ncr_ans->_stat++;
			ncr_ans->_answ->set_id(dns_qst->_id);

			printf("[RESCACHED] got one on cache  ...\n");
			printf("[RESCACHED] setting answer id to: %d \n", dns_qst->_id);
			printf("[RESCACHED] increasing stat to  : %d \n", ncr_ans->_stat);

			client->send(ncr_ans->_answ->_bfr);
		}
next:
		client = client->_next;
	}

	delete dns_qst;
}

static void process_udp_clients(Resolver *resolver, NameCache *nc, Socket *srvr, fd_set *allfds)
{
	int		s;
	struct sockaddr	addr;
	DNSQuery	*dns_qst	= new DNSQuery();
	DNSQuery	*dns_ans	= NULL;
	NCR		*ncr_ans	= NULL;

	s = srvr->recv_udp(&addr);
	if (s > 0) {
		dns_qst->extract(srvr);

		printf(">> QUERY: %s\n", dns_qst->_name._v);
		ncr_ans = nc->get_answer_from_cache(&dns_qst->_name);
		if (! ncr_ans) {
			dns_ans = new DNSQuery();

			s = resolver->send_query_udp(dns_qst, dns_ans);
			if (s) {
				delete dns_ans;
				goto out;
			}

			if (dns_ans->_n_ans) {
				nc->insert(&dns_qst->_name, dns_qst->_bfr, dns_ans->_bfr);
			}

			srvr->send_udp(&addr, dns_ans->_bfr);

			delete dns_ans;
		} else {
			ncr_ans->_stat++;
			ncr_ans->_answ->set_id(dns_qst->_id);

			printf("[RESCACHED] got one on cache  ...\n");
			printf("[RESCACHED] setting answer id to: %d \n", dns_qst->_id);
			printf("[RESCACHED] increasing stat to  : %d \n", ncr_ans->_stat);

			srvr->send_udp(&addr, ncr_ans->_answ->_bfr);
		}
	}

out:
	srvr->reset();

	delete dns_qst;
}

int main(int argc, char *argv[])
try
{
	int		maxfd;
	int		s;
	fd_set		readfds;
	fd_set		allfds;
	NameCache	nc;
	Resolver	resolver;
	Socket		srvr_tcp;
	Socket		srvr_udp;
	Socket		*client	= NULL;

	FD_ZERO(&allfds);
	FD_ZERO(&readfds);

	rescached_set_signal_handle();

	resolver.set_server(DNS_SERVERS);

	printf("[RESCACHED] loading caches ...\n");
	nc.load(NAMECACHE_FILE, NAMECACHE_MD);
	nc.dump();

	srvr_udp.create_udp();
	srvr_udp.bind_listen(RESCACHED_DEF_ADDR, RESCACHED_DEF_PORT);
	FD_SET(srvr_udp._d, &allfds);

	srvr_tcp.create_tcp();
	srvr_tcp.bind_listen(RESCACHED_DEF_ADDR, RESCACHED_DEF_PORT);
	FD_SET(srvr_tcp._d, &allfds);

	maxfd = (srvr_tcp._d > srvr_udp._d ? srvr_tcp._d : srvr_udp._d) + 1;

	while (_running_) {
		readfds = allfds;

		printf("[RESCACHED] waiting for client...\n");
		s = select(maxfd, &readfds, NULL, NULL, NULL);
		if (s <= 0) {
			continue;
		}

		if (FD_ISSET(srvr_tcp._d, &srvr_tcp._readfds)) {
			client = srvr_tcp.accept_conn();
			if (client->_d > maxfd) {
				maxfd = client->_d;
			}
			FD_SET(client->_d, &allfds);
			process_tcp_clients(&resolver, &nc, &srvr_tcp, &allfds);
		}

		if (FD_ISSET(srvr_udp._d, &readfds)) {
			srvr_udp._readfds = readfds;
			process_udp_clients(&resolver, &nc, &srvr_udp, &allfds);
		}
	}

	printf("[RESCACHED] saving caches ...\n");
	nc.dump();
	nc.save(NAMECACHE_FILE, NAMECACHE_MD);

	return 0;
}
catch (Error &e) {
	e.print();
}
