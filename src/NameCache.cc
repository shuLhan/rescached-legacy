//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "NameCache.hh"

namespace rescached {

NameCache::NameCache() :
	_cache_max(0)
,	_cache_thr(0)
,	_cachel()
,	_buckets(NULL)
,	__ncr_finder()
{
	delete __ncr_finder._name;
}

NameCache::~NameCache()
{
	prune();
}

/**
 * @method	: NameCache::bucket_init
 * @desc	: Initialize buckets.
 * @return 0	: success.
 * @return -1	: fail.
 */
int NameCache::bucket_init ()
{
	int s = 0;

	_buckets = (RBT**) calloc(CACHET_IDX_FIRST + 1, sizeof(RBT*));
	if (!_buckets) {
		return -1;
	}

	for (; s <= CACHET_IDX_SIZE; ++s) {
		_buckets[s] = new RBT(NCR::CMP, NCR::SWAP_PTREE);
	}

	return 0;
}

/**
 * @method	: NameCache::bucket_get_by_index
 * @desc	: Get bucket index by character 'c'.
 * @param c	: First character of data.
 * @return >= 0	: Index of bucket of 'c'.
 */
RBT* NameCache::bucket_get_by_index(int c)
{
	if (isalpha (c)) {
		c = (c - 'A') + 10;
	} else if (isdigit (c)) {
		c = c - CACHET_IDX_FIRST;
	} else {
		c = CACHET_IDX_SIZE;
	}
	return _buckets[c];
}

/**
 * @method	: NameCache::raw_to_ncrecord
 * @desc	: Convert from DSVRecord to NameCache Record.
 * @param raw	: input.
 * @param ncr	: output.
 * @return 0	: success.
 * @return !0	: fail.
 */
int NameCache::raw_to_ncrecord(DSVRecord* raw, NCR** ncr)
{
	DSVRecord* name = raw->get_column(0);
	DSVRecord* stat = raw->get_column(1);
	DSVRecord* ttl = raw->get_column(2);
	DSVRecord* answer = raw->get_column(3);

	(*ncr) = NCR::INIT(name, answer);
	if (*ncr) {
		(*ncr)->_stat	= (int) strtol(stat->v(), 0, 10);
		(*ncr)->_ttl	= (uint32_t) strtol(ttl->v(), 0, 10);
	}
	raw->columns_reset();

	return 0;
}

/**
 * @method	: NameCache::load
 * @param	:
 *	> fdata	: file where cache resided.
 * @return	:
 *	< 0	: success.
 *	< -1	: fail.
 * @desc	: load cache from file 'fdata' using 'fmetadata' as
 * meta-data.
 */
int NameCache::load(const char* fdata)
{
	int		s;
	DSVReader	R;
	time_t		time_now= time(NULL);
	NCR*		ncr	= NULL;
	DSVRecord*	row	= NULL;
	List*		list_md	= NULL;
	DNSQuery*	lanswer	= NULL;
	TreeNode*	node	= NULL;

	Error err = R.open_ro(fdata);
	if (err != NULL) {
		return -1;
	}

	list_md = DSVRecordMD::INIT(RESCACHED_MD);
	if (!list_md) {
		return -1;
	}

	s = DSVRecord::INIT_ROW(&row, list_md->size());
	if (s != 0) {
		return -1;
	}

	s = R.read(row, list_md);

	while (_running
	&& s == 1
	&& (_cachel.size() < _cache_max || 0 == _cache_max)
	) {
		s = raw_to_ncrecord(row, &ncr);
		if (0 == s) {
			if (ncr->_ttl == 0 || ncr->_ttl == UINT_MAX) {
				ncr->_ttl = (uint32_t) time_now;
			}

			s = get_answer_from_cache (ncr->_answ, &lanswer, &node);

			switch (s) {
			// record with the same name and type already exist.
			case 0:
				delete ncr;
				break;
			// name not found
			case -1:
				s = insert (&ncr, 0, 0);
				if (s != 0) {
					delete ncr;
				} else {
					if (ncr && !_skip_log && DBG_LVL_IS_1) {
						dlog.er("    load: %3d %6ds %s\n"
							, ncr->_answ->_q_type
							, ncr->_answ->_ans_ttl_max
							, ncr->_name->v());
					}
				}
				break;
			}
		}
		s = R.read(row, list_md);
		ncr = NULL;
	}

	delete list_md;
	delete row;

	return 0;
}

/**
 * @method	: NameCache::ncrecord_to_record
 * @desc	: Convert from NCR to DSVRecord.
 * @param NCR	: input.
 * @param DSVRecord: output.
 * @return 0	: success.
 * @return 1	: input or output is null.
 */
int NameCache::ncrecord_to_record(const NCR* ncr, DSVRecord* row)
{
	if (!ncr || !row) {
		return 1;
	}
	if (ncr->_name) {
		row->set_column(0, ncr->_name);
	}
	if (ncr->_stat) {
		row->set_column_number(1, ncr->_stat);
	}

	row->set_column_number (2, int(ncr->_ttl));

	if (ncr->_answ) {
		row->set_column(3, ncr->_answ);
	}

	return 0;
}

/**
 * @return	:
 *	< 0	: success.
 *	< -1	: fail.
 */
int NameCache::save(const char* fdata)
{
	int		s	= 0;
	DSVWriter	W;
	DSVRecord*	row	= NULL;
	List*		list_md	= NULL;
	BNode* p = NULL;
	NCR* ncr = NULL;

	if (_cachel.size() <= 0) {
		return 0;
	}

	if (DBG_LVL_IS_1) {
		dlog.er("\nsaving %d records ...\n"
			, _cachel.size());
	}

	Error err = W.open_wt(fdata);
	if (err != NULL) {
		return -1;
	}

	list_md = DSVRecordMD::INIT(RESCACHED_MD);
	if (!list_md) {
		return -1;
	}

	s = DSVRecord::INIT_ROW(&row, list_md->size());
	if (s != 0) {
		return -1;
	}

	_cachel.lock();

	p = _cachel._head;
	do {
		ncr = (NCR*) p->get_content();

		ncrecord_to_record(ncr, row);

		s = W.write(row, list_md);
		if (s != 0) {
			break;
		}

		row->columns_reset();

		p = p->get_right();
	} while(p != _cachel._tail->get_right());

	_cachel.unlock();

	delete row;
	delete list_md;

	return 0;
}

/**
 * @method	: NameCache::get_answer_from_cache
 * @param question	: hostname that will be search in the tree.
 * @param answer	: return, query answer.
 * @param node		: return value, pointer to DNS answer in tree.
 * @return 0	: success, cached answer that match with question found.
 * @return -1	: fail, question not found.
 * @desc	: search for record with 'name' in the tree and
 * 	with query type in answer, return node found in the tree
 * 	and answer in the record.
 */
int NameCache::get_answer_from_cache (const DNSQuery* question
					, DNSQuery** answer
					, TreeNode** node)
{
	if (!question) {
		return -1;
	}

	lock();

	__ncr_finder._q_type = question->_q_type;
	__ncr_finder._name = (vos::Buffer*) &question->_name;

	int c = -1;
	RBT* bucket = bucket_get_by_index(toupper(__ncr_finder._name->char_at(0)));

	if (!bucket->get_root()) {
		goto out;
	}


	(*node) = bucket->find(&__ncr_finder);
	if (!(*node)) {
		c = -1;
		goto out;
	}

	(*answer) = ((NCR*)(*node)->get_content())->_answ;
	c = 0;
out:
	__ncr_finder._name = 0;

	unlock();

	return c;
}

/**
 * @method	: NameCache::clean_by_threshold()
 * @param	:
 *	> thr	: threshold value.
 * @return	:
 *	< 0	: success.
 *	< <0	: fail.
 * @desc	: remove cache object where status <= threshold.
 */
void NameCache::clean_by_threshold(const long int thr)
{
	if (_cachel.size() <= 0) {
		return;
	}

	TreeNode*	node	= NULL;
	TreeNode*	ndel	= NULL;
	RBT*		bucket	= NULL;

	NCR* ncr = NULL;
	NCR* ncr_del = NULL;
	int idx;

	BNode* p = _cachel._tail;
	do {
		ncr = (NCR*) p->get_content();

		if (ncr->_stat > thr) {
			break;
		}

		idx = toupper(ncr->_name->char_at(0));
		bucket = bucket_get_by_index(idx);

		if (DBG_LVL_IS_2) {
			dlog.er("removing: %3d %s -%d\n"
				, ncr->_answ->_q_type
				, ncr->_name->v()
				, ncr->_stat);
		}

		if (bucket->get_root()) {
			node = (TreeNode*) ncr->_p_tree;
			ndel = bucket->remove(node);

			if (ndel) {
				ncr_del = (NCR*) ndel->get_content();

				if (DBG_LVL_IS_2) {
					dlog.er("removed: %3d %s -%d\n"
						, ncr_del->_answ->_q_type
						, ncr_del->_name->v()
						, ncr_del->_stat);
				}

				ncr->_p_tree = NULL;
				ncr->_p_list = NULL;
				delete ndel;
				ndel = NULL;
			}
		}

		p = _cachel.node_pop_tail();
		p->set_content(NULL);
		delete p;

		p = _cachel._tail;
	} while (p);
}

/**
 * @desc: insert 'record' into cache tree & list.
 *
 * @param:
 *	> record : Name Cache Record object.
 *
 * @return:
 *	< >0	: fail, record already exist.
 *	< 0	: success.
 *	< -1	: fail.
 */
int NameCache::insert (NCR** ncr, const int do_cleanup
		, const int skip_list)
{
	if (! (*ncr)) {
		return -1;
	}
	if (! (*ncr)->_name) {
		return -1;
	}
	if (! (*ncr)->_name->len()) {
		return -1;
	}
	if (! (*ncr)->_stat) {
		return -1;
	}
	if (! (*ncr)->_answ) {
		return -1;
	}

	long int	thr	= _cache_thr;
	TreeNode*	p_tree	= NULL;
	TreeNode*	p_ins	= NULL;
	RBT*		bucket	= NULL;
	DNSQuery*	answer	= (*ncr)->_answ;
	time_t		time_now= time(NULL);

	if (answer->_id) {
		answer->set_id(0);
	}

	if (answer->get_num_answer() == 0) {
		(*ncr)->_ttl = 0;
	} else {
		if (answer->_ans_ttl_max < _cache_minttl) {
			answer->set_rr_answer_ttl (_cache_minttl);
			(*ncr)->_ttl = (uint32_t) (time_now + _cache_minttl);
		} else {
			answer->set_rr_answer_ttl (answer->_ans_ttl_max);
			(*ncr)->_ttl = (uint32_t) (time_now + answer->_ans_ttl_max);
		}
	}

	while (do_cleanup && _cachel.size() >= _cache_max) {
		clean_by_threshold(thr);

		++thr;
	}

	bucket = bucket_get_by_index (toupper ((*ncr)->_name->char_at(0)));

	/* add to tree */
	p_tree = new TreeNode((*ncr));
	if (!p_tree) {
		return -1;
	}

	p_ins = bucket->insert(p_tree, 1);

	if (p_ins != p_tree) {
		p_tree->set_content(NULL);
		delete p_tree;
		return 1;
	}

	if (! skip_list) {
		(*ncr)->_p_tree	= p_tree;
		(*ncr)->_p_list = _cachel.push_tail_sorted((*ncr), 0
						, NCR::CMP_BY_STAT);
	}

	return 0;
}

/**
 * @return	:
 *	< >0	: fail, record with the same name already exist.
 *	< 0	: success.
 *	< -1	: fail.
 */
int NameCache::insert_copy (DNSQuery* answer
		, const int do_cleanup
		, const int skip_list)
{
	if (!answer) {
		return 0;
	}

	int		s	= 0;
	DNSQuery*	lanswer	= NULL;
	TreeNode*	node	= NULL;
	Buffer*		name	= (Buffer*) &answer->_name;
	NCR*		ncr	= NULL;
	time_t		time_now= time(NULL);
	int		n_answer= answer->get_num_answer();

	if (answer->len() > 512) {
		answer->remove_rr_add();
	}
	if (answer->len() > 512) {
		answer->remove_rr_aut();
	}
	if (DBG_LVL_IS_3) {
		dlog.out("answer size: %d\n", answer->len());
	}

	s = get_answer_from_cache (answer, &lanswer, &node);
	if (s == 0) {
		lock();

		// replace answer
		lanswer->set (answer);
		lanswer->extract (vos::DNSQ_EXTRACT_RR_AUTH);

		ncr = (NCR*) node->get_content();

		// reset TTL in NCR
		if (n_answer == 0) {
			ncr->_ttl = 0;
			answer->_ans_ttl_max = 0;
		} else {
			if (answer->_ans_ttl_max < _cache_minttl) {
				ncr->_ttl = (uint32_t) (time_now
					+ _cache_minttl);
			} else {
				ncr->_ttl = (uint32_t) (time_now
					+ answer->_ans_ttl_max);
			}
		}

		if (!_skip_log && DBG_LVL_IS_1) {
			dlog.out (" renewed: %3d %6ds %s\n"
				, answer->_q_type
				, answer->_ans_ttl_max
				, name->v());
		}
		unlock();

	} else if (s == -1) {
		// name not found, create new node.

		ncr = NCR::INIT(name, answer);
		if (!ncr) {
			unlock();
			return s;
		}

		if (n_answer == 0) {
			ncr->_answ->_ans_ttl_max = 0;
		} else {
			ncr->_answ->_ans_ttl_max = answer->_ans_ttl_max;
			ncr->_answ->_attrs = answer->_attrs;
		}

		lock();
		s = insert (&ncr, do_cleanup, skip_list);
		if (s != 0) {
			delete ncr;
			ncr = NULL;
		} else {
			if (!_skip_log && DBG_LVL_IS_1) {
				dlog.out ("  insert: %3d %6ds %s (%ld)\n"
					, answer->_q_type
					, answer->_ans_ttl_max
					, name->v(), _cachel.size());
			}
		}
		unlock();
	}

	return 0;
}

void NameCache::increase_stat_and_rebuild(BNode* list_node)
{
	/* In case list is empty, i.e. node is from hosts */
	if (! list_node) {
		return;
	}

	int s = 0;

	lock();

	NCR* ncr = (NCR*) list_node->get_content();
	ncr->_stat++;

	if (list_node == _cachel._head) {
		goto out;
	}

	// Quick check the stat with upper cache
	s = NCR::CMP_BY_STAT(list_node->get_content()
		, list_node->get_left()->get_content());
	if (s <= 0) {
		goto out;
	}

	if (DBG_LVL_IS_1) {
		dlog.out(" rebuild: %3d %6ds %s +%d\n"
			, ncr->_answ->_q_type, ncr->_answ->_ans_ttl_max
			, ncr->_answ->_name.v(), ncr->_stat);
	}

	_cachel.detach(list_node);
	_cachel.node_push_tail_sorted(list_node, 0, NCR::CMP_BY_STAT);
out:
	unlock();
}

void NameCache::prune()
{
	BNode* node = NULL;

	if (_buckets) {
		for (int i = 0; i <= CACHET_IDX_SIZE; ++i) {
			delete _buckets[i];
		}
		free(_buckets);
		_buckets = NULL;
	}

	while (_cachel.size() > 0) {
		node = _cachel.node_pop_head();
		if (node->get_content()) {
			node->set_content(NULL);
		}
		delete node;
	}
}

void NameCache::dump()
{
	int i = 0;

	if (_cachel.size() > 0) {
		dlog.write_raw("\nNameCache::dump >> LIST\n");
		_cachel.chars();
	}

	if (!_buckets) {
		return;
	}

	dlog.write_raw("\nNameCache::dump >> TREE\n");
	for (; i < CACHET_IDX_SIZE; ++i) {
		if (!_buckets[i]->get_root()) {
			continue;
		}

		dlog.writef(
"\n\n\
------------------------------------------------------------------------\n\
 [%c]\n\
------------------------------------------------------------------------\n",
i + CACHET_IDX_FIRST);

		_buckets[i]->chars();
	}
}

void NameCache::dump_r()
{
	lock();
	dump();
	unlock();
}

} /* namespace::rescached */
// vi: ts=8 sw=8 tw=78:
