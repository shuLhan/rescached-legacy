/**
 * Copyright 2017 M. Shulhan (ms@kilabit.info). All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ClientWorker.hh"
#include "ResolverWorkerUDP.hh"

namespace rescached {

NameCache _nc;
extern Resolver _WorkerTCP;
extern ResolverWorkerUDP *_WorkerUDP;

ClientWorker::ClientWorker()
: Thread(NULL)
, _queue_questions()
, _queue_answers()
{}

ClientWorker::~ClientWorker()
{}

/**
 * `is_already_asked` will check previous queue if the same question has been
 * asked.
 *
 * This is to minimized request to parent resolver and remove duplicate
 * renew on old cache.
 */
int ClientWorker::_is_already_asked(BNode* qnode, ResQueue* q)
{
	BNode* p = qnode;
	ResQueue* rq = NULL;

	while (p != _queue_questions.tail()) {
		rq = (ResQueue*) p->get_content();

		if (rq->get_state() != IS_RESOLVING) {
			goto cont;
		}
		if (rq->_qstn->_name.like(&q->_qstn->_name) != 0) {
			goto cont;
		}
		if (rq->_qstn->_q_type != q->_qstn->_q_type) {
			goto cont;
		}

		return 1;
cont:
		p = p->get_left();
	}

	return 0;
}

int ClientWorker::_queue_ask_question(BNode* qnode, ResQueue* q)
{
	int s = 0;

	s = _is_already_asked(qnode, q);
	if (s) {
		dlog.out("%8s: %3d %6d %s\n", TAG_SKIP
			, q->_qstn->_q_type
			, 0
			, q->_qstn->_name.chars());

		q->set_state(IS_RESOLVING);

		return s;
	}

	if (_dns_conn_t == vos::IS_UDP) {
		s = _WorkerUDP->ask(q->_qstn);

		q->set_state(IS_RESOLVING);

		return s;
	}

	s = _WorkerTCP.send_tcp(q->_qstn);
	if (s) {
		q->set_state(IS_ERROR);

		return -1;
	}

	DNSQuery *answer = new DNSQuery();

	s = _WorkerTCP.recv_tcp(answer);
	if (s) {
		q->set_state(IS_ERROR);

		delete answer;
		return -1;
	}

	s = _nc.insert_copy(answer, 1, 0);

	if (DBG_LVL_IS_2) {
		dlog.out("%s: push answer %s\n", __cname
			, answer->_name.chars());
	}

	q->set_state(IS_RESOLVING);

	_WorkerTCP.reset();
	_WorkerTCP.close();

	_queue_answers.push_tail(answer);

	return s;
}

int ClientWorker::_queue_check_ttl(BNode* qnode, ResQueue* q, NCR* ncr)
{
	time_t now = time(NULL);
	int diff = (int) difftime(now, ncr->_ttl);

	if (diff >= 0) {
		// Renew the question.
		int s = _queue_ask_question(qnode, q);

		if (DBG_LVL_IS_1 && s == 0) {
			dlog.out("%8s: %3d %6ds %s\n"
				, TAG_RENEW
				, q->_qstn->_q_type
				, diff
				, q->_qstn->_name.chars()
				);
		}

		return -1;
	}

	diff = (int) difftime(ncr->_ttl, now);

	_nc.increase_stat_and_rebuild(ncr->_p_list);

	return diff;
}

/**
 * Method `_queue_answer(q, answer)` will process queue item `q` by replying
 * with `answer`.
 */
int ClientWorker::_queue_answer(ResQueue* q, DNSQuery* answer)
{
	int s = 0;

	answer->set_id(q->_qstn->_id);

	if (q->_tcp_client) {
		if (answer->_bfr_type == vos::BUFFER_IS_UDP) {
			s = answer->to_tcp();
			if (s < 0) {
				goto out;
			}
		}

		q->_tcp_client->reset();
		q->_tcp_client->write(answer);

	} else if (q->_udp_client) {
		if (answer->_bfr_type == vos::BUFFER_IS_UDP) {
			_srvr_udp.send_udp(q->_udp_client, answer);
		} else {
			_srvr_udp.send_udp_raw(q->_udp_client
						, answer->v(2)
						, answer->len() - 2);
		}
	} else {
		dlog.er("queue_answer: no client connected!\n");
		s = -1;
	}

out:
	q->set_state(IS_RESOLVED);

	return s;
}

int ClientWorker::_queue_process_new(BNode* qnode, ResQueue* q)
{
	int s = 0;
	const char* tag = NULL;
	DNSQuery* answer = NULL;
	TreeNode* tnode = NULL;
	NCR* ncr = NULL;

	s = _nc.get_answer_from_cache(q->_qstn, &answer, &tnode);

	// Question is not found in cache
	if (s < 0) {
		if (DBG_LVL_IS_1) {
			dlog.out("%8s: %3d %s\n"
				, TAG_QUERY
				, q->_qstn->_q_type
				, q->_qstn->_name.chars()
				);
		}

		s = _queue_ask_question(qnode, q);

		return -1;
	}

	ncr = (NCR*) tnode->get_content();

	switch (answer->_attrs) {
	case vos::DNS_IS_QUERY:
		s = _queue_check_ttl(qnode, q, ncr);

		if (s < 0) {
			// Question is being renewed.
			return -1;
		}

		// Update answer TTL with time difference.
		answer->set_rr_answer_ttl((unsigned int) s);

		tag = TAG_CACHED;
		break;

	case vos::DNS_IS_LOCAL:
		tag = TAG_LOCAL;
		break;

	case vos::DNS_IS_BLOCKED:
		tag = TAG_BLOCKED;
		break;
	}

	if (DBG_LVL_IS_1) {
		dlog.out("%8s: %3d %6ds %s +%d\n"
			, tag
			, q->_qstn->_q_type
			, s
			, q->_qstn->_name.chars()
			, ncr->_stat
			);
	}

	_queue_answer(q, answer);

	return 0;
}

int ClientWorker::_queue_process_old(BNode* qnode, ResQueue* q)
{
	time_t t = time(NULL);
	int difft = (int) difftime(t, q->_timeout);

	if (difft >= _rto) {
		if (DBG_LVL_IS_1) {
			dlog.out("%8s: %3d -%6d %s %d %d\n"
				, TAG_TIMEOUT
				, q->_qstn->_q_type
				, difft
				, q->_qstn->_name.v()
				, t, q->_timeout
				);
		}

		q->set_state(IS_TIMEOUT);

		return -1;
	}

	_queue_process_new(qnode, q);

	return 0;
}

int ClientWorker::_queue_process_questions()
{
	int x = 0;
	BNode* pnext = NULL;
	ResQueue* q = NULL;

	BNode* p = _queue_questions.head();

	while (x < _queue_questions.size()) {
		pnext = p->get_right();
		q = (ResQueue*) p->get_content();

		switch (q->get_state()) {
		case IS_NEW:
			_queue_process_new(p, q);
			break;
		case IS_RESOLVING:
			_queue_process_old(p, q);
			break;
		case IS_RESOLVED:
		case IS_TIMEOUT:
		case IS_ERROR:
			break;
		}

		if (q->get_state() == IS_RESOLVED
		||  q->get_state() == IS_TIMEOUT
		||  q->get_state() == IS_ERROR
		) {
			_queue_questions.node_remove_unsafe(p);
			delete q;
			x--;
		}

		p = pnext;
		x++;
	}

	return 0;
}

int ClientWorker::_queue_process_answers()
{
	BNode* p_question = NULL;
	BNode* p_question_next = NULL;
	DNSQuery* answer = NULL;
	ResQueue* rq = NULL;

	BNode* p_answer = _queue_answers.node_pop_head();

	while (p_answer) {
		answer = (DNSQuery*) p_answer->get_content();

		int y = 0;
		p_question = _queue_questions.head();

		while (p_question && y < _queue_questions.size()) {
			p_question_next = p_question->get_right();
			rq = (ResQueue*) p_question->get_content();

			if (answer->_name.like(&rq->_qstn->_name) != 0) {
				p_question = p_question_next;
				y++;
				continue;
			}

			if (answer->_q_type != rq->_qstn->_q_type) {
				p_question = p_question_next;
				y++;
				continue;
			}

			_queue_answer(rq, answer);

			_queue_questions.node_remove_unsafe(p_question);
			delete rq;
			p_question = p_question_next;
		}

		delete p_answer;
		p_answer = _queue_answers.node_pop_head();
	}

	return 0;
}

void* ClientWorker::run(void* arg)
{
	ClientWorker* cw = (ClientWorker*) arg;

	while (cw->is_running()) {
		cw->lock();
		cw->_queue_process_answers();
		cw->_queue_process_questions();
		cw->unlock();
		cw->wait();
	}

	return 0;
}

void ClientWorker::push_question(ResQueue* q)
{
	lock();
	_queue_questions.push_tail(q);
	unlock();
	wakeup();
}

void ClientWorker::push_answer(DNSQuery* answer)
{
	lock();
	_queue_answers.push_tail(answer);
	unlock();
	wakeup();
}

} // namespace::rescached
