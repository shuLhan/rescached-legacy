/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "NameCache.hpp"

namespace rescached {

NameCache::NameCache() :
	_cachet(NULL),
	_cachel(NULL)
{}

NameCache::~NameCache()
{
	NCR_List *next = NULL;
	NCR_List *node = NULL;

	if (_cachet) {
		for (int i = 0; i < CACHE_MAX; ++i)
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

NCR * NameCache::raw_to_ncrecord(Record *raw)
{
	Buffer	*col = NULL;
	NCR	*ncr = new NCR();
	if (! ncr)
		throw Error(vos::E_MEM);

	ncr->_name	= new Buffer(raw->get_column(0));
	col		= raw->get_column(1);
	ncr->_stat	= strtol(col->_v, 0, 10);
	ncr->_qstn	= new DNSQuery(raw->get_column(2));
	ncr->_answ	= new DNSQuery(raw->get_column(3));

	printf(" >> col stat : %s\n", col->_v);
	printf(" >> read stat: %d\n", ncr->_stat);

	raw->columns_reset();

	return ncr;
}

int NameCache::load(const char *fdata, const char *fmetadata)
try
{
	int		s;
	Reader		R;
	NCR		*ncr	= NULL;
	Record		*rec	= NULL;
	RecordMD	*rmd	= NULL;

	if (! _cachet) {
		_cachet = new NCR_Tree[CACHE_MAX];
		if (! _cachet)
			throw Error(vos::E_MEM);
	}

	R.open_ro(fdata);

	rmd	= RecordMD::INIT_FROM_FILE(fmetadata);
	rec	= Record::INIT_ROW(rmd->_n_md);
	s	= R.read(rec, rmd);
	while (s) {
		ncr = raw_to_ncrecord(rec);
		if (ncr) {
			s = insert(ncr);
			if (s) {
				delete ncr;
			}
		}
		s = R.read(rec, rmd);
	}

	delete rmd;
	delete rec;

	return 0;
}
catch (Error &e) {
	if (vos::E_FILE_OPEN == e._code)
		return 0;
	throw;
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
		row->set_column(1, ncr->_stat);
	}
	if (ncr->_qstn) {
		row->set_column(2, ncr->_qstn->_bfr);
	}
	if (ncr->_answ) {
		row->set_column(3, ncr->_answ->_bfr);
	}

	return 0;
}

int NameCache::save(const char *fdata, const char *fmetadata)
{
	Writer		W;
	NCR_List	*p	= NULL;
	Record		*row	= NULL;
	RecordMD	*rmd	= NULL;

	W.open_wo(fdata);
	rmd = RecordMD::INIT_FROM_FILE(fmetadata);

	row = Record::INIT_ROW(rmd->_n_md);
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

	s = toupper(name->_v[0]) - 'A';
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

void NameCache::insert(Buffer *name, Buffer *question, Buffer *answer)
{
	int	s;
	NCR	*ncr = NULL;

	if (! name)
		return;

	ncr	= new NCR(name, question, answer);
	s	= insert(ncr);
	if (s) {
		delete ncr;
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
 *	< 1	: fail.
 */
int NameCache::insert(NCR *record)
{
	int s;

	if (! record)
		return 1;

	if (record->_name && record->_name->_i) {
		s = toupper(record->_name->_v[0]) - 'A';
		if (s >= 0 && s < CACHE_MAX) {
			printf(">> inserting %s \n", record->_name->_v);
			NCR_List::ADD_RECORD(&_cachel, record);
			_cachet[s].insert_record(record);

			return 0;
		} else {
			printf(" index out of range: %d\n", s);
		}
	}

	return 1;
}

void NameCache::dump()
{
	printf("\n >> LIST\n");
	if (_cachel) {
		_cachel->dump();
	}

	printf("\n >> TREE\n");
	if (_cachet) {
		for (int i = 0; i < CACHE_MAX; ++i) {
			printf(" [%c]\n", i + 'A');
			_cachet[i].dump();
		}
	}
}

} /* namespace::rescached */
