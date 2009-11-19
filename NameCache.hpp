/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_NAME_CACHE_HPP
#define	_RESCACHED_NAME_CACHE_HPP	1

#include "Reader.hpp"
#include "Writer.hpp"
#include "NCR_Tree.hpp"
#include "NCR_List.hpp"

using vos::Error;
using vos::_errcode;
using vos::Record;
using vos::RecordMD;
using vos::Reader;
using vos::Writer;

#define	CACHE_MAX	26

namespace rescached {

class NameCache {
public:
	NameCache();
	~NameCache();

	NCR *raw_to_ncrecord(Record *raw);
	int load(const char *fdata, const char *fmetadata);

	int ncrecord_to_record(NCR *ncr, Record *row);
	int save(const char *fdata, const char *fmetadata);

	NCR *get_answer_from_cache(Buffer *name);
	void insert(Buffer *name, Buffer *address, Buffer *answer);
	int insert(NCR *record);

	void dump();

	NCR_Tree	*_cachet;
	NCR_List	*_cachel;
private:
	DISALLOW_COPY_AND_ASSIGN(NameCache);
};

} /* namespace::rescached */

#endif
