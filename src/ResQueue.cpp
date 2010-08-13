/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "ResQueue.hpp"

namespace rescached {

ResQueue::ResQueue() :
	_udp_client(NULL)
,	_tcp_client(NULL)
,	_qstn(NULL)
,	_next(NULL)
,	_last(this)
{}

ResQueue::~ResQueue()
{
	if (_udp_client) {
		free(_udp_client);
		_udp_client = NULL;
	}
	if (_tcp_client) {
		_tcp_client = NULL;
	}
	if (_qstn) {
		delete _qstn;
		_qstn = NULL;
	}
	if (_next) {
		delete _next;
		_next = NULL;
	}
}

/**
 * @desc	: push a new node to queue.
 * @param	:
 *	> head	: head of queue.
 *	> node	: pointer to new node.
 */
void ResQueue::PUSH(ResQueue** head, ResQueue* node)
{
	if (!(*head)) {
		(*head) = node;
	} else {
		(*head)->_last->_next	= node;
		(*head)->_last		= node;
	}
}

/**
 * @desc	: remove head from queue.
 * @param	:
 *	> head	: pointer to head of queue.
 *	> node	: return value, the current head of queue.
 *
 * @return	:
 *	< head	: a new head of queue.
 */
void ResQueue::POP(ResQueue** head, ResQueue** node)
{
	if (!(*head)) {
		(*node) = NULL;
	} else {
		(*node)		= (*head);
		(*head)		= (*head)->_next;

		if ((*head)) {
			(*head)->_last = (*node)->_last;
		}

		(*node)->_next	= NULL;
		(*node)->_last	= NULL;
	}
}

} /* namespace::rescached */
