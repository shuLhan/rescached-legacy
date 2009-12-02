/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_NAME_CACHE_HPP
#define	_RESCACHED_NAME_CACHE_HPP	1

#include "main.hpp"
#include "Reader.hpp"
#include "Writer.hpp"
#include "NCR_Tree.hpp"
#include "NCR_List.hpp"

using vos::Record;
using vos::RecordMD;
using vos::Reader;
using vos::Writer;

#define	CACHET_IDX_SIZE	26	/* size of bucket tree, A-Z + others */

namespace rescached {

class NameCache {
public:
	NameCache();
	~NameCache();

	int raw_to_ncrecord(Record *raw, NCR **ncr);
	int load(const char *fdata, const char *fmetadata, const long int max);

	int ncrecord_to_record(NCR *ncr, Record *row);
	int save(const char *fdata, const char *fmetadata);

	NCR *get_answer_from_cache(Buffer *name);

	int insert(NCR *record);
	int insert_raw(const int type, const Buffer *name,
			const Buffer *address, const Buffer *answer);

	void cachet_remove(NCR *record);
	void clean_by_threshold(int thr);

	void prune();
	void dump();

	long int	_n_cache;
	NCR_Tree	*_cachet;
	NCR_List	*_cachel;
private:
	DISALLOW_COPY_AND_ASSIGN(NameCache);
};

} /* namespace::rescached */

#endif
