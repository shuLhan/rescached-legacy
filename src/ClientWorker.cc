/**
 * Copyright 2017 M. Shulhan (ms@kilabit.info). All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ClientWorker.hh"

namespace rescached {

NameCache _nc;

ClientWorker::ClientWorker()
: Thread(NULL)
, _queue_questions()
, _queue_answers()
{}

ClientWorker::~ClientWorker()
{}

int ClientWorker::_queue_ask_question(ResQueue* q)
{
	int s = 0;

	if (_dns_conn_t == 0) {
		s = _resolver.send_udp(q->_qstn);
	} else {
		s = _resolver.send_tcp(q->_qstn);
	}
	q->set_state(queue_state::IS_RESOLVING);

	return s;
}

int ClientWorker::_queue_check_ttl(ResQueue* q, NCR* ncr)
{
	time_t now = time(NULL);
	int diff = (int) difftime(now, ncr->_ttl);

	if (diff >= 0) {
		if (DBG_LVL_IS_1) {
			dlog.out("%8s: %3d %s %d %d %d\n"
				, TAG_RENEW
				, q->_qstn->_q_type
				, q->_qstn->_name.chars()
				, now, ncr->_ttl, diff
				);
		}

		// Renew the question.
		_queue_ask_question(q);

		return -1;
	}

	diff = (int) difftime(ncr->_ttl, now);

	_nc.increase_stat_and_rebuild(ncr->_p_list);

	return diff;
}

int ClientWorker::_queue_answer(ResQueue* q, DNSQuery* answer)
{
	int s;

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
						, &answer->_v[2]
						, answer->_i - 2);
		}
	} else {
		dlog.er("queue_answer: no client connected!\n");
		s = -1;
	}

out:
	q->set_state(queue_state::IS_RESOLVED);

	return s;
}

int ClientWorker::_queue_process_new(ResQueue* q)
{
	int s = 0;
	const char* tag = NULL;
	DNSQuery* answer = NULL;
	TreeNode* tnode = NULL;
	NCR* ncr = NULL;

	s = _nc.get_answer_from_cache(q->_qstn, &answer, &tnode);

	// Question is not found in cache
	if (s < 0) {
		_queue_ask_question(q);
		return -1;
	}

	ncr = (NCR*) tnode->get_content();

	switch (answer->_attrs) {
	case vos::DNS_IS_QUERY:
		s = _queue_check_ttl(q, ncr);

		if (s < 0) {
			// Question is being renewed.
			return -1;
		}

		// Update answer TTL with time difference.
		answer->set_rr_answer_ttl(s);

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

int ClientWorker::_queue_process_old(ResQueue* q)
{
	time_t t = time(NULL);
	int difft = (int) difftime(t, q->_timeout);

	if (difft >= _rto) {
		if (DBG_LVL_IS_1) {
			dlog.out("%8s: %3d %s %d %d %d\n"
				, TAG_TIMEOUT
				, q->_qstn->_q_type
				, q->_qstn->_name._v
				, t, q->_timeout, difft
				);
		}

		q->set_state(queue_state::IS_TIMEOUT);

		return -1;
	}

	_queue_process_new(q);

	return 0;
}

int ClientWorker::_queue_process_questions()
{
	int x = 0;
	BNode* p = NULL;
	BNode* pnext = NULL;
	ResQueue* q = NULL;

	p = _queue_questions._head;

	while (x < _queue_questions.size()) {
		pnext = p->_right;
		q = (ResQueue*) p->get_content();

		switch (q->get_state()) {
		case queue_state::IS_NEW:
			_queue_process_new(q);
			break;
		case queue_state::IS_RESOLVING:
			_queue_process_old(q);
			break;
		case queue_state::IS_RESOLVED:
		case queue_state::IS_TIMEOUT:
			_queue_questions.node_remove_unsafe(p);
			delete q;
			x--;
			break;
		}

		if (q->get_state() == queue_state::IS_RESOLVED
		||  q->get_state() == queue_state::IS_TIMEOUT
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
	int y = 0;
	BNode* p_answer = NULL;
	BNode* p_question = NULL;
	BNode* p_question_next = NULL;
	DNSQuery* answer = NULL;
	ResQueue* rq = NULL;

	p_answer = _queue_answers.node_pop_head();

	while (p_answer) {
		answer = (DNSQuery*) p_answer->get_content();

		y = 0;
		p_question = _queue_questions._head;

		while (p_question && y < _queue_questions.size()) {
			p_question_next = p_question->_right;
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