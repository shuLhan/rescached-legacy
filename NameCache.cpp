/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "NameCache.hpp"

namespace rescached {

NameCache::NameCache() :
	_n_cache(0),
	_cachet(NULL),
	_cachel(NULL)
{}

NameCache::~NameCache()
{
	NCR_List *next = NULL;
	NCR_List *node = NULL;

	if (_cachet) {
		for (int i = 0; i <= CACHET_IDX_SIZE; ++i)
			_cachet[i].prune();
		delete[] _cachet;
	}

	node = _cachel;
	while (node) {
		next = node->_down;
		delete node;
		node = next;
	}
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

	_cachel = NULL;

	if (! _cachet) {
		_cachet = new NCR_Tree[CACHET_IDX_SIZE + 1];
		if (!_cachet)
			return -vos::E_MEM;
	}

	s = R.open_ro(fdata);
	if (s != 0)
		return 0;

	s = RecordMD::INIT_FROM_FILE(&rmd, fmetadata);
	if (s != 0)
		return s;

	s = Record::INIT_ROW(&row, rmd->_n_md, Buffer::DFLT_SIZE);
	if (s != 0)
		return s;

	s = R.read(row, rmd);
	while (s == 1 && _n_cache < max) {
		ncr	= NULL;
		s	= raw_to_ncrecord(row, &ncr);
		if (0 == s) {
			s = insert(ncr);
			if (s != 0) {
				delete ncr;
			}
		}
		s = R.read(row, rmd);
	}

	dlog.er("[RESCACHED] number of cache loaded > %ld\n", _n_cache);

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
		if (p->_rec) {
			ncrecord_to_record(p->_rec, row);
			W.write(row, rmd);
			row->columns_reset();
		}

		p = p->_down;
	}

	delete row;
	delete rmd;

	return 0;
}

NCR *NameCache::get_answer_from_cache(Buffer *name)
{
	int		s	= 0;
	NCR_Tree	*p	= NULL;

	if (! name)
		return NULL;

	if (! isalpha(name->_v[0])) {
		s = CACHET_IDX_SIZE;
	} else {
		s = toupper(name->_v[0]) - 'A';
	}

	p = &_cachet[s];

	while (p) {
		if (p->_rec) {
			s = p->_rec->_name->like(name);
			if (0 == s) {
				return p->_rec;
			}
			if (s < 0) {
				p = p->_left;
			} else {
				p = p->_right;
			}
		} else {
			break;
		}
	}

	return NULL;
}

/**
 * @desc: insert Record 'record' into cache tree & list.
 *
 * @param:
 *	> record : Name Cache Record object.
 *
 * @return:
 *	< 0	: success.
 *	< -1	: fail.
 */
int NameCache::insert(NCR *record)
{
	int s = 0;
	int c = 0;

	if (!record)
		return 0;

	if (record->_name && record->_name->_i) {
		c = record->_name->_v[0];
		if (isalpha(c)) {
			c = toupper(c) - 'A';
			if (c >= 0 && c < CACHET_IDX_SIZE) {
				s = NCR_List::ADD_RECORD(&_cachel, record);
				if (0 == s) {
					s = _cachet[c].insert_record(record);
				}
			} else {
				dlog.er(" index out of range: %d\n", c);
				s = -1;
			}
		} else {
			s = NCR_List::ADD_RECORD(&_cachel, record);
			if (0 == s) {
				s = _cachet[CACHET_IDX_SIZE].insert_record(record);
			}
		}
	}

	if (0 == s) {
		_n_cache++;
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

void NameCache::dump()
{
	int i = 0;

	printf("\n >> LIST\n");
	if (_cachel) {
		_cachel->dump();
	}

	printf("\n >> TREE\n");
	if (_cachet) {
		for (; i < CACHET_IDX_SIZE; ++i) {
			printf(" [%c]\n", i + 'A');
			_cachet[i].dump();
		}
		printf(" [others]\n");
		_cachet[i].dump();
	}
}

} /* namespace::rescached */
