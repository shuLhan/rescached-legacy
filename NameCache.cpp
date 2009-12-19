/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "NameCache.hpp"

namespace rescached {

NameCache::NameCache() :
	_n_cache(0),
	_buckets(NULL),
	_cachel(NULL)
{}

NameCache::~NameCache()
{
	prune();
}

int NameCache::raw_to_ncrecord(Record *raw, NCR **ncr)
{
	int	s		= 0;
	Record	*name		= NULL;
	Record	*stat		= NULL;
	Record	*type		= NULL;
	Record	*question	= NULL;
	Record	*answer		= NULL;

	name		= raw->get_column(0);
	stat		= raw->get_column(1);
	type		= raw->get_column(2);
	question	= raw->get_column(3);
	answer		= raw->get_column(4);

	s = NCR::INIT(ncr, vos::BUFFER_IS_UDP, name, question, answer);
	if (0 == s) {
		(*ncr)->_stat	= strtol(stat->_v, 0, 10);
		(*ncr)->_type	= strtol(type->_v, 0, 10);
	}
	raw->columns_reset();

	return s;
}

/**
 * @desc: load cache from file 'fdata' using 'fmetadata' as meta-data.
 *
 * @param:
 *	> fdata		: file where cache resided.
 *	> fmetadata	: meta-data, how the data is saved in the file.
 * @return:
 *	< 0	: success.
 *	< <0	: fail.
 */
int NameCache::load(const char *fdata, const char *fmetadata, const long int max)
{
	int		s;
	Reader		R;
	NCR		*ncr	= NULL;
	Record		*row	= NULL;
	RecordMD	*rmd	= NULL;

	prune();

	_buckets = new NCR_Bucket[CACHET_IDX_SIZE + 1];
	if (!_buckets)
		return -vos::E_MEM;

	for (s = 0; s <= CACHET_IDX_SIZE; ++s)
		_buckets[s]._v = NULL;

	s = R.open_ro(fdata);
	if (s != 0)
		return s;

	s = RecordMD::INIT_FROM_FILE(&rmd, fmetadata);
	if (s != 0)
		return s;

	s = Record::INIT_ROW(&row, rmd->_n_md, Buffer::DFLT_SIZE);
	if (s != 0)
		return s;

	s = R.read(row, rmd);
	while (s == 1 && (_n_cache < max || 0 == max)) {
		s = raw_to_ncrecord(row, &ncr);
		if (0 == s) {
			s = insert(ncr);
			if (s != 0) {
				delete ncr;
			}
		}
		s = R.read(row, rmd);
		ncr = NULL;
	}

	delete rmd;
	delete row;

	return 0;
}

int NameCache::ncrecord_to_record(NCR *ncr, Record *row)
{
	if (! ncr)
		return 0;
	if (! row)
		return 0;

	if (ncr->_name) {
		row->set_column(0, ncr->_name);
	}
	if (ncr->_stat) {
		row->set_column_number(1, ncr->_stat);
	}
	if (ncr->_type) {
		row->set_column_number(2, ncr->_type);
	} else {
		row->set_column_number(2, vos::BUFFER_IS_UDP);
	}
	if (ncr->_qstn) {
		row->set_column(3, ncr->_qstn->_bfr);
	}
	if (ncr->_answ) {
		row->set_column(4, ncr->_answ->_bfr);
	}

	return 0;
}

/**
 * @return	:
 *	< 0	: success.
 *	< !0	: fail.
 */
int NameCache::save(const char *fdata, const char *fmetadata)
{
	int		s	= 0;
	Writer		W;
	NCR_List	*p	= NULL;
	Record		*row	= NULL;
	RecordMD	*rmd	= NULL;

	s = W.init();
	if (s != 0)
		return s;

	s = W.open_wo(fdata);
	if (s != 0) {
		return s;
	}

	s = RecordMD::INIT_FROM_FILE(&rmd, fmetadata);
	if (s != 0)
		return s;

	s = Record::INIT_ROW(&row, rmd->_n_md, Buffer::DFLT_SIZE);
	if (s != 0)
		return s;

	p = _cachel;
	while (p) {
		if (p->_rec && p->_rec->_name && p->_rec->_name->_i) {
			ncrecord_to_record(p->_rec, row);
			s = W.write(row, rmd);
			if (s != 0)
				break;

			row->columns_reset();
		}
		p = p->_down;
	}

	delete row;
	delete rmd;

	return 0;
}

/**
 * @return	:
 *	< >=0	: success, return index of buckets tree.
 *	< -1	: fail, name not found.
 */
int NameCache::get_answer_from_cache(NCR_Tree **node, Buffer *name)
{
	int c = 0;

	if (!name)
		return -1;

	c = toupper(name->_v[0]);
	if (isalnum(c)) {
		c = c - CACHET_IDX_FIRST;
	} else {
		c = CACHET_IDX_SIZE;
	}

	if (_buckets[c]._v) {
		(*node) = _buckets[c]._v->search_record_name(name);
		if (!(*node)) {
			return -1;
		}
	} else {
		return -1;
	}

	return c;
}

/**
 * @desc	: remove cache object where status <= threshold.
 *
 * @param	:
 *	> thr	: threshold value.
 *
 * @return	:
 *	< 0	: success.
 *	< <0	: fail.
 */
void NameCache::clean_by_threshold(int thr)
{
	int		c	= 0;
	NCR_List	*p	= NULL;
	NCR_List	*up	= NULL;
	NCR_Tree	*node	= NULL;

	p = _cachel->_last;
	while (p) {
		if (p->_rec->_stat > thr)
			break;

		if (DBG_LVL_IS_1) {
			dlog.it("[RESCACHED] removing '%s' - %d\n",
				p->_rec->_name->_v, p->_rec->_stat);
		}

		c = toupper(p->_rec->_name->_v[0]);
		if (isalnum(c)) {
			c = c - CACHET_IDX_FIRST;
		} else {
			c = CACHET_IDX_SIZE;
		}

		if (_buckets[c]._v) {
			node = (NCR_Tree *) p->_p_tree;
			_buckets[c]._v = NCR_Tree::REMOVE(_buckets[c]._v, node);
			if (node) {
				node->_rec = NULL;
				delete node;
				p->_p_tree = NULL;
			}
		}

		if (p == _cachel) {
			_cachel = NULL;
			up	= NULL;
		} else {
			up = p->_up;
			if (p == _cachel->_last)
				_cachel->_last = up;

			up->_down = NULL;
		}

		p->_up		= NULL;
		p->_p_tree	= NULL;

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
 *	< 0	: success.
 *	< <0	: fail.
 */
int NameCache::insert(NCR *record)
{
	int		s	= 0;
	int		c	= 0;
	int		thr	= _cache_thr;
	DNSQuery	*answer	= NULL;
	NCR_List	*p_list	= NULL;
	NCR_Tree	*p_tree	= NULL;

	if (!record)
		return 0;
	if (!record->_name)
		return 1;
	if (!record->_name->_i)
		return 1;
	if (!record->_stat)
		return 1;

	answer = record->_answ;
	if (!answer)
		return 1;

	/* remove authority and additional record */
	if (answer->_rr_ans_p == NULL) {
		s = answer->extract();
		if (s != 0) {
			return s;
		}
	}
	if (answer->_n_add) {
		answer->remove_rr_add();
	}
	if (answer->_n_aut) {
		answer->remove_rr_aut();
	}
	if (answer->_id)
		answer->set_id(0);

	while (_cachel && _n_cache >= _cache_max) {
		clean_by_threshold(thr);

		if (_n_cache < _cache_max)
			break;

		++thr;
		if (DBG_LVL_IS_1) {
			dlog.it("[RESCACHED] increasing threshold to %d\n",
				thr);
		}
	}

	/* add to list */
	p_list = new NCR_List();
	if (!p_list)
		return -vos::E_MEM;

	p_list->_rec = record;

	NCR_List::ADD(&_cachel, p_list);

	/* add to tree */
	p_tree = new NCR_Tree();
	if (!p_tree)
		return -vos::E_MEM;

	p_tree->_rec	= record;
	p_tree->_p_list	= p_list;
	p_list->_p_tree	= p_tree;

	c = toupper(record->_name->_v[0]);
	if (isalnum(c)) {
		c = c - CACHET_IDX_FIRST;
	} else {
		c = CACHET_IDX_SIZE;
	}

	_buckets[c]._v = NCR_Tree::INSERT(_buckets[c]._v, p_tree);
	++_n_cache;

	if (DBG_LVL_IS_1) {
		dlog.it("[RESCACHED] inserting '%s' - %d (%ld)\n",
			record->_name->_v, record->_stat, _n_cache);
	}

	return s;
}

/**
 * @return	:
 *	< 0	: success.
 *	< <0	: fail.
 */
int NameCache::insert_raw(const int type, const Buffer *name,
			const Buffer *question, const Buffer *answer)
{
	int	s	= 0;
	NCR	*ncr	= NULL;

	if (!name)
		return 0;

	s = NCR::INIT(&ncr, type, name, question, answer);
	if (s != 0)
		return s;

	s = insert(ncr);
	if (s != 0) {
		delete ncr;
		s = 0;
	}

	return s;
}

void NameCache::prune()
{
	NCR_List *next = NULL;
	NCR_List *node = NULL;

	if (_buckets) {
		for (int i = 0; i <= CACHET_IDX_SIZE; ++i) {
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
	int i;

	dlog.it("\n >> LIST\n");
	if (_cachel) {
		_cachel->dump();
	}

	dlog.it("\n >> TREE\n");
	if (_buckets) {
		for (i = 0; i < CACHET_IDX_SIZE; ++i) {
			dlog.it(" [%c]\n", i + CACHET_IDX_FIRST);
			if (_buckets[i]._v)
				_buckets[i]._v->dump_tree(0);
		}
	}
}

} /* namespace::rescached */
