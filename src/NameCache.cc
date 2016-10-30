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
,	_locker()
,	_buckets(NULL)
,	_cachel()
{}

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

	_buckets = new NCR_Bucket[CACHET_IDX_SIZE + 1];
	if (!_buckets) {
		return -1;
	}

	for (; s <= CACHET_IDX_SIZE; ++s) {
		_buckets[s]._v = NULL;
	}

	return 0;
}

/**
 * @method	: NameCache::bucket_get_index
 * @desc	: Get bucket index by character 'c'.
 * @param c	: First character of data.
 * @return >= 0	: Index of bucket of 'c'.
 */
NCR_Bucket* NameCache::bucket_get_by_index (int c)
{
	if (isalpha (c)) {
		c = (c - 'A') + 10;
	} else if (isdigit (c)) {
		c = c - CACHET_IDX_FIRST;
	} else {
		c = CACHET_IDX_SIZE;
	}
	return &_buckets[c];
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
	int	s	= 0;
	DSVRecord* name		= NULL;
	DSVRecord* stat		= NULL;
	DSVRecord* ttl		= NULL;
	DSVRecord* answer	= NULL;

	name		= raw->get_column(0);
	stat		= raw->get_column(1);
	ttl		= raw->get_column(2);
	answer		= raw->get_column(3);

	s = NCR::INIT(ncr, name, answer);
	if (0 == s) {
		(*ncr)->_stat	= (int) strtol(stat->_v, 0, 10);
		(*ncr)->_ttl	= (int32_t) strtol(ttl->_v, 0, 10);
	}
	raw->columns_reset();

	return s;
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
	NCR_Tree*	node	= NULL;

	s = R.open_ro(fdata);
	if (s != 0) {
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
	while (s == 1 && (_cachel.size() < _cache_max || 0 == _cache_max)) {
		s = raw_to_ncrecord(row, &ncr);
		if (0 == s) {
			if (ncr->_ttl <= 0 || ncr->_ttl == UINT_MAX) {
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
						dlog.er("[rescached]     load: %3d %6ds %s\n"
							, ncr->_answ->_q_type
							, ncr->_answ->_ans_ttl_max
							, ncr->_name->_v);
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

	row->set_column_number (2, ncr->_ttl);

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
		dlog.er("\n[rescached] saving %d records ...\n"
			, _cachel.size());
	}

	s = W.open_wo(fdata);
	if (s != 0) {
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
		ncr = (NCR*) p->_item;

		ncrecord_to_record(ncr, row);

		s = W.write(row, list_md);
		if (s != 0) {
			break;
		}

		row->columns_reset();

		p = p->_right;
	} while(p != _cachel._tail->_right);

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
					, NCR_Tree** node)
{
	if (!question) {
		return -1;
	}

	_locker.lock();

	Buffer* name = (Buffer*) &question->_name;
	uint16_t qtype = question->_q_type;
	int c = -1;
	NCR_Bucket* bucket = bucket_get_by_index (toupper (name->_v[0]));

	if (!bucket->_v) {
		goto out;
	}

	(*node) = bucket->_v->search_record(name, qtype);
	if (!(*node)) {
		c = -1;
		goto out;
	}

	(*answer) = (*node)->_rec->_answ;
	c = 0;
out:
	_locker.unlock();

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

	NCR_Tree*	node	= NULL;
	NCR_Tree*	ndel	= NULL;
	NCR_Bucket*	bucket	= NULL;

	BNode* p = NULL;
	NCR* ncr = NULL;

	p = _cachel._tail;
	do {
		ncr = (NCR*) p->_item;

		if (ncr->_stat > thr) {
			break;
		}

		if (DBG_LVL_IS_1) {
			dlog.er("[rescached] removing: %3d %s -%d\n"
				, ncr->_answ->_q_type
				, ncr->_name->_v, ncr->_stat);
		}

		bucket = bucket_get_by_index(toupper(ncr->_name->_v[0]));

		if (bucket->_v) {
			node = (NCR_Tree*) ncr->_p_tree;
			ndel = NCR_Tree::RBT_REMOVE(&bucket->_v, node);
			if (ndel) {
				ndel->_p_list = NULL;
				ncr->_p_tree = NULL;
				delete ndel;
				ndel = NULL;
			}
		}

		p = _cachel.node_pop_tail();
		p->_item = NULL;
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
	if (! (*ncr)->_name->_i) {
		return -1;
	}
	if (! (*ncr)->_stat) {
		return -1;
	}
	if (! (*ncr)->_answ) {
		return -1;
	}

	int		s	= 0;
	NCR_Tree*	p_tree	= NULL;
	NCR_Bucket*	bucket	= NULL;
	DNSQuery*	answer	= (*ncr)->_answ;
	time_t		time_now= time(NULL);

	if (answer->_id) {
		answer->set_id(0);
	}

	if (answer->_ans_ttl_max < _cache_minttl) {
		answer->set_rr_answer_ttl (_cache_minttl);
		(*ncr)->_ttl = (uint32_t) (time_now + _cache_minttl);
	} else {
		answer->set_rr_answer_ttl (answer->_ans_ttl_max);
		(*ncr)->_ttl = (uint32_t) (time_now + answer->_ans_ttl_max);
	}

	if (do_cleanup) {
		long int	thr	= _cache_thr;

		while (_cachel.size() > 0
		&& ((_cachel.size() + ((thr * (thr + 1)) / 2)) >= _cache_max)) {
			clean_by_threshold(thr);

			// stop cleaning if (n + summation (thr)) < max
			if ((_cachel.size() + ((thr * (thr + 1)) / 2)) < _cache_max) {
				break;
			}

			++thr;
			if (DBG_LVL_IS_1) {
				dlog.er(
				"[rescached] increasing threshold to %d\n"
				, thr);
			}
		}
	}

	bucket = bucket_get_by_index (toupper ((*ncr)->_name->_v[0]));

	/* add to tree */
	p_tree = new NCR_Tree();
	if (!p_tree) {
		return -1;
	}

	p_tree->_rec = (*ncr);

	s = NCR_Tree::RBT_INSERT(&bucket->_v, p_tree);
	if (s != 0) {
		p_tree->_rec = NULL;
		delete p_tree;
		return -1;
	}

	if (! skip_list) {
		(*ncr)->_p_tree	= p_tree;

		p_tree->_p_list = _cachel.push_tail_sorted((*ncr), 0
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
	NCR_Tree*	node	= NULL;
	Buffer*		name	= (Buffer*) &answer->_name;
	NCR*		ncr	= NULL;
	time_t		time_now= time(NULL);

	if (answer->_i > 512) {
		answer->remove_rr_add();
	}
	if (answer->_i > 512) {
		answer->remove_rr_aut();
	}
	if (DBG_LVL_IS_2) {
		dlog.out("[rescached] answer size: %d\n", answer->_i);
	}

	s = get_answer_from_cache (answer, &lanswer, &node);
	if (s == 0) {
		// replace answer
		lanswer->set (answer);
		lanswer->extract (vos::DNSQ_EXTRACT_RR_AUTH);

		// reset TTL in NCR
		if (answer->_ans_ttl_max < _cache_minttl) {
			node->_rec->_ttl = (uint32_t) (time_now
				+ _cache_minttl);
		} else {
			node->_rec->_ttl = (uint32_t) (time_now
				+ answer->_ans_ttl_max);
		}

		if (!_skip_log && DBG_LVL_IS_1) {
			dlog.out ("[rescached]  renewed: %3d %6ds %s\n"
				, answer->_q_type
				, answer->_ans_ttl_max
				, name->_v);
		}

	} else if (s == -1) {
		// name not found, create new node.
		_locker.lock();

		s = NCR::INIT(&ncr, name, answer);
		if (s != 0) {
			_locker.unlock();
			return s;
		}

		ncr->_answ->_ans_ttl_max = answer->_ans_ttl_max;
		ncr->_answ->_attrs = answer->_attrs;

		s = insert (&ncr, do_cleanup, skip_list);
		if (s != 0) {
			delete ncr;
			ncr = NULL;
		} else {
			if (!_skip_log && DBG_LVL_IS_1) {
				dlog.out ("[rescached]   insert: %3d %6ds %s (%ld)\n"
					, answer->_q_type
					, answer->_ans_ttl_max
					, name->_v, _cachel.size());
			}
		}

		_locker.unlock();
	}

	return 0;
}

void NameCache::increase_stat_and_rebuild(BNode* list_node)
{
	/* In case list is empty, i.e. node is from hosts */
	if (! list_node) {
		return;
	}

	NCR* ncr = NULL;
	NCR_Tree* ncr_tree = NULL;

	_locker.lock();

	ncr = (NCR*) list_node->_item;
	ncr_tree = (NCR_Tree*) ncr->_p_tree;
	ncr->_stat++;

	if (list_node == _cachel._head) {
		goto out;
	}

	_cachel.detach(list_node);
	ncr_tree->_p_list = _cachel.push_tail_sorted(list_node->_item, 0
							, NCR::CMP_BY_STAT);
	list_node->_item = NULL;
	delete list_node;
out:
	_locker.unlock();
}

void NameCache::prune()
{
	int i = 0;
	BNode* node = NULL;

	if (_buckets) {
		for (; i <= CACHET_IDX_SIZE; ++i) {
			if (_buckets[i]._v) {
				_buckets[i]._v->prune();
				delete _buckets[i]._v;
				_buckets[i]._v = NULL;
			}
		}
		delete[] _buckets;
		_buckets = NULL;
	}

	while (_cachel.size() > 0) {
		node = _cachel.node_pop_head();
		if (node->_item) {
			node->_item = NULL;
		}
		delete node;
	}
}

void NameCache::dump()
{
	int i = 0;

	if (_cachel.size() > 0) {
		dlog.write_raw("\n[rescached] NameCache::dump >> LIST\n");
		_cachel.chars();
	}

	if (!_buckets) {
		return;
	}

	dlog.write_raw("\n[rescached] NameCache::dump >> TREE\n");
	for (; i < CACHET_IDX_SIZE; ++i) {
		if (!_buckets[i]._v) {
			continue;
		}

		dlog.writes(
"\n\n\
------------------------------------------------------------------------\n\
 [%c]\n\
------------------------------------------------------------------------\n",
i + CACHET_IDX_FIRST);

		_buckets[i]._v->dump_tree(0);
	}
}

void NameCache::dump_r()
{
	_locker.lock();
	dump();
	_locker.unlock();
}

} /* namespace::rescached */
// vi: ts=8 sw=8 tw=78:
