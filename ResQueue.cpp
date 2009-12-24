#include "ResQueue.hpp"

namespace rescached {

ResQueue::ResQueue() :
	_udp_client(NULL),
	_tcp_client(NULL),
	_qstn(NULL),
	_next(NULL)
{}

ResQueue::~ResQueue()
{
	if (_udp_client)
		free(_udp_client);
	if (_tcp_client)
		_tcp_client = NULL;
	if (_qstn)
		delete _qstn;
	if (_next)
		delete _next;
}

/**
 * @desc	: push a new node to queue.
 *
 * @param	:
 *	> head	: head of queue.
 *	> node	: pointer to new node.
 *
 * @return	:
 *	< head	: pointer to new head of queue.
 */
ResQueue * ResQueue::PUSH(ResQueue *head, ResQueue *node)
{
	ResQueue *p = head;

	if (!head)
		return node;

	while (p->_next)
		p = p->_next;

	p->_next = node;

	return head;
}

/**
 * @desc	: remove head from queue.
 *
 * @param	:
 *	> head	: pointer to head of queue.
 *	> node	: return value, the current head of queue.
 *
 * @return	:
 *	< head	: a new head of queue.
 */
ResQueue * ResQueue::POP(ResQueue *head, ResQueue **node)
{
	if (!head)
		return NULL;

	(*node)		= head;
	head		= head->_next;
	(*node)->_next	= NULL;

	return head;
}

} /* namespace::rescached */
