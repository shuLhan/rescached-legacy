//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _RESCACHED_NAME_CACHE_HH
#define _RESCACHED_NAME_CACHE_HH 1

#include "Locker.hh"
#include "Reader.hh"
#include "Writer.hh"
#include "NCR_Tree.hh"
#include "NCR_List.hh"

using vos::Locker;
using vos::List;
using vos::Record;
using vos::RecordMD;
using vos::Reader;
using vos::Writer;

#define RESCACHED_MD	\
":name:::'|',"		\
":stat:::'|',"		\
":ttl:::'|',"		\
":answer:::'|':BLOB"

#define	CACHET_IDX_SIZE		37	/* size of bucket tree, 0-Z + others */
#define	CACHET_IDX_FIRST	'0'

namespace rescached {

struct NCR_Bucket {
	NCR_Tree* _v;
};

class NameCache {
public:
	NameCache();
	~NameCache();

	int bucket_init ();
	NCR_Bucket* bucket_get_by_index (int c);

	int raw_to_ncrecord(Record* raw, NCR** ncr);
	int load(const char* fdata);

	int ncrecord_to_record(const NCR* ncr, Record* row);
	int save(const char* fdata);

	int get_answer_from_cache (const DNSQuery* question
				, DNSQuery** answer
				, NCR_Tree** node);

	void clean_by_threshold(const long int thr);

	int insert (NCR** record, const int do_cleanup
			, const int skip_list);
	int insert_copy (DNSQuery* answer
			, const int do_cleanup, const int skip_list);

	void increase_stat_and_rebuild(NCR_List *list);

	void prune();
	void dump();
	void dump_r();

	long int	_n_cache;
	long int	_cache_max;
	long int	_cache_thr;
	Locker		_locker;
	NCR_Bucket*	_buckets;
	NCR_List*	_cachel;
private:
	NameCache(const NameCache&);
	void operator=(const NameCache&);
};

} /* namespace::rescached */

#endif
// vi: ts=8 sw=8 tw=78:
