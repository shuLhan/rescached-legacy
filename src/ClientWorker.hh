/**
 * Copyright 2017 M. Shulhan (ms@kilabit.info). All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef	_RESCACHED_CLIENT_WORKER_HH
#define	_RESCACHED_CLIENT_WORKER_HH 1

#include "lib/Thread.hh"
#include "lib/List.hh"
#include "lib/TreeNode.hh"
#include "common.hh"
#include "ResQueue.hh"
#include "NCR.hh"
#include "NameCache.hh"

using vos::Thread;
using vos::BNode;
using vos::List;
using vos::TreeNode;

namespace rescached {

extern NameCache _nc;

#define	TAG_BLOCKED	"blocked"
#define	TAG_LOCAL	"local"
#define	TAG_CACHED	"cached"
#define	TAG_RESOLVER	"resolver"
#define	TAG_QUEUE	"queue"
#define	TAG_RENEW	"renew"

class ClientWorker : public Thread {
public:
	ClientWorker();
	~ClientWorker();

	void* run(void* arg);
	void push_question(ResQueue* q);
	void push_answer(DNSQuery* answer);
private:
	List _queue_questions;
	List _queue_answers;

	int _queue_ask_question(ResQueue* q);
	int _queue_check_ttl(ResQueue* q, NCR* ncr);
	int _queue_answer(ResQueue* q, DNSQuery* answer);
	int _queue_process_new(ResQueue* q);
	int _queue_process_old(ResQueue* q);
	int _queue_process_questions();
	int _queue_process_answers();
};

}

#endif
