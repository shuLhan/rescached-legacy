/*
 * Copyright 2010-2016 Mhd Sulhan (ms@kilabit.info)
 */

#include "NameCache.hpp"

namespace rescached {

NameCache::NameCache() :
	_n_cache(0)
,	_cache_max(0)
,	_cache_thr(0)
,	_lock()
,	_buckets(NULL)
,	_cachel(NULL)
{
	pthread_mutex_init(&_lock, NULL);
}

NameCache::~NameCache()
{
	prune();
	pthread_mutex_destroy(&_lock);
}

inline void NameCache::lock()
{
	while (pthread_mutex_trylock(&_lock) != 0)
		;
}

void NameCache::unlock()
{
	while (pthread_mutex_unlock(&_lock) != 0)
		;
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
 * @desc	: Convert from Record to NameCache Record.
 * @param raw	: input.
 * @param ncr	: output.
 * @return 0	: success.
 * @return !0	: fail.
 */
int NameCache::raw_to_ncrecord(Record* raw, NCR** ncr)
{
	int	s		= 0;
	Record*	name		= NULL;
	Record*	stat		= NULL;
	Record* ttl		= NULL;
	Record*	answer		= NULL;

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
	Reader		R;
	time_t		time_now= time(NULL);
	NCR*		ncr	= NULL;
	Record*		row	= NULL;
	RecordMD*	rmd	= NULL;
	DNSQuery*	lanswer	= NULL;
	NCR_Tree*	node	= NULL;

	s = R.open_ro(fdata);
	if (s != 0) {
		return -1;
	}

	s = RecordMD::INIT(&rmd, RESCACHED_MD);
	if (s != 0) {
		return -1;
	}

	s = Record::INIT_ROW(&row, rmd->_n_md);
	if (s != 0) {
		return -1;
	}

	s = R.read(row, rmd);
	while (s == 1 && (_n_cache < _cache_max || 0 == _cache_max)) {
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
			// name exist but no type found
			case -2:
				if (node && node->_rec) {
					node->_rec->answer_push (ncr->_answ);
				}

				if (!_skip_log && DBG_LVL_IS_1) {
					dlog.out("[rescached] load-add: %3d %6ds %s\n"
						, ncr->_answ->_q_type
						, ncr->_answ->_ans_ttl_max
						, ncr->_answ->_name._v);
				}

				ncr->_answ = NULL;
				delete ncr;
				break;
			}
		}
		s = R.read(row, rmd);
		ncr = NULL;
	}

	delete rmd;
	delete row;

	return 0;
}

/**
 * @method	: NameCache::ncrecord_to_record
 * @desc	: Convert from NCR to Record.
 * @param NCR	: input.
 * @param Record: output.
 * @return 0	: success.
 * @return 1	: input or output is null.
 */
int NameCache::ncrecord_to_record(const NCR* ncr, Record* row)
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
	Writer		W;
	NCR_List*	p	= NULL;
	Record*		row	= NULL;
	RecordMD*	rmd	= NULL;

	s = W.open_wo(fdata);
	if (s != 0) {
		return -1;
	}

	s = RecordMD::INIT(&rmd, RESCACHED_MD);
	if (s != 0) {
		return -1;
	}

	s = Record::INIT_ROW(&row, rmd->_n_md);
	if (s != 0) {
		return -1;
	}

	p = _cachel;
	while (p) {
		if (p->_rec && p->_rec->_name && p->_rec->_name->_i) {
			while (p->_rec->_answ) {
				ncrecord_to_record (p->_rec, row);

				s = W.write(row, rmd);
				if (s != 0) {
					break;
				}

				row->columns_reset();

				p->_rec->answer_pop ();
			}

		}
		p = p->_down;
	}

	delete row;
	delete rmd;

	return 0;
}

/**
 * @method	: NameCache::get_answer_from_cache
 * @param question	: hostname that will be search in the tree.
 * @param answer	: return, query answer.
 * @param node		: return value, pointer to DNS answer in tree.
 * @return >=0	: success, return index of buckets tree.
 * @return -1	: fail, name not found.
 * @return -2	: fail, query type not found.
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

	lock();

	Buffer* name = (Buffer*) &question->_name;
	int c = 0;
	NCR_Bucket* bucket = bucket_get_by_index (toupper (name->_v[0]));

	if (bucket->_v) {
		(*node) = bucket->_v->search_record_name (name);
		if (!(*node)) {
			c = -1;
		} else {
			/* find answer by type */
			(*answer) = (*node)->_rec->search_answer_by_type (
						question->_q_type);

			if (! (*answer)) {
				c = -2;
			}
		}
	} else {
		c = -1;
	}

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
	NCR_List*	p	= NULL;
	NCR_List*	up	= NULL;
	NCR_Tree*	node	= NULL;
	NCR_Tree*	ndel	= NULL;
	NCR_Bucket*	bucket	= NULL;

	p = _cachel->_last;
	while (p) {
		if (p->_rec->_stat > thr) {
			break;
		}

		if (DBG_LVL_IS_1) {
			dlog.er("[rescached] removing: %3d %s -%d\n"
				, p->_rec->_answ->_q_type
				, p->_rec->_name->_v, p->_rec->_stat);
		}

		bucket = bucket_get_by_index (toupper (p->_rec->_name->_v[0]));

		if (bucket->_v) {
			node = (NCR_Tree *) p->_p_tree;
			ndel = NCR_Tree::RBT_REMOVE(&bucket->_v, node);
			if (ndel) {
				ndel->_p_list	= NULL;
				delete ndel;
				ndel = NULL;
			}
		}

		if (p == _cachel) {
			_cachel = NULL;
			up	= NULL;
		} else {
			up = p->_up;
			if (p == _cachel->_last) {
				_cachel->_last = up;
			}
			up->_down = NULL;
		}

		p->_up		= NULL;
		p->_p_tree	= NULL;
		p->_rec		= NULL;

		delete p;
		--_n_cache;

		p = up;
	}
}

/**
 * @desc: insert Record 'record' into cache tree & list.
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
	NCR_List*	p_list	= NULL;
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

		while (_cachel
		&& ((_n_cache + ((thr * (thr + 1)) / 2)) >= _cache_max)) {
			clean_by_threshold(thr);

			// stop cleaning if (n + summation (thr)) < max
			if ((_n_cache + ((thr * (thr + 1)) / 2)) < _cache_max) {
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
		/* add to list */
		p_list = new NCR_List();
		if (!p_list) {
			return -1;
		}

		p_list->_rec	= (*ncr);
		p_list->_p_tree	= p_tree;
		p_tree->_p_list	= p_list;

		NCR_List::ADD(&_cachel, NULL, p_list);

		++_n_cache;
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
		lock();

		s = NCR::INIT(&ncr, name, answer);
		if (s != 0) {
			unlock();
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
					, name->_v, _n_cache);
			}
		}

		unlock();
	} else if (s == -2) {
		// name exist but no type found
		DNSQuery* nu_answer = answer->duplicate ();

		if (! nu_answer) {
			return -1;
		}

		if (node && node->_rec) {
			node->_rec->answer_push (nu_answer);
		}

		if (!_skip_log && DBG_LVL_IS_1) {
			dlog.out("[rescached]      add: %3d %s\n"
				, nu_answer->_q_type, nu_answer->_name.v());
		}
	}

	return 0;
}

void NameCache::increase_stat_and_rebuild(NCR_List* list)
{
	/* In case list is empty, i.e. node is from hosts */
	if (! list) {
		return;
	}

	lock();
	list->_rec->_stat++;
	NCR_List::REBUILD(&_cachel, list);
	unlock();
}

void NameCache::prune()
{
	int i = 0;
	NCR_List* next = NULL;
	NCR_List* node = NULL;

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

	node = _cachel;
	while (node) {
		next = node->_down;
		delete node;
		node = next;
	}
	_cachel		= NULL;
	_n_cache	= 0;
}

void NameCache::dump()
{
	int i = 0;

	if (_cachel) {
		dlog.write_raw("\n[rescached] NameCache::dump >> LIST\n");
		_cachel->dump();
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
	lock();
	dump();
	unlock();
}

} /* namespace::rescached */
