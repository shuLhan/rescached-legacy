//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _RESCACHED_NAME_CACHE_HH
#define _RESCACHED_NAME_CACHE_HH 1

#include "lib/DSVReader.hh"
#include "lib/DSVWriter.hh"
#include "lib/RBT.hh"
#include "NCR.hh"

using vos::Locker;
using vos::BNode;
using vos::List;
using vos::DSVRecord;
using vos::DSVRecordMD;
using vos::DSVReader;
using vos::DSVWriter;
using vos::RBT;
using vos::TreeNode;

#define RESCACHED_MD	\
":name:::'|',"		\
":stat:::'|',"		\
":ttl:::'|',"		\
":answer:::'|':BLOB"

#define	CACHET_IDX_SIZE		37	/* size of bucket tree, 0-Z + others */
#define	CACHET_IDX_FIRST	'0'

namespace rescached {

class NameCache : public Locker {
public:
	NameCache();
	~NameCache();

	int bucket_init ();
	RBT* bucket_get_by_index(int c);

	int raw_to_ncrecord(DSVRecord* raw, NCR** ncr);
	int load(const char* fdata);

	int ncrecord_to_record(const NCR* ncr, DSVRecord* row);
	int save(const char* fdata);

	int get_answer_from_cache (const DNSQuery* question
				, DNSQuery** answer
				, TreeNode** node);

	void clean_by_threshold(const long int thr);

	int insert (NCR** record, const int do_cleanup
			, const int skip_list);
	int insert_copy (DNSQuery* answer
			, const int do_cleanup, const int skip_list);

	void increase_stat_and_rebuild(BNode* list_node);

	void prune();
	void dump();
	void dump_r();

	long int	_cache_max;
	long int	_cache_thr;
	List		_cachel;
	RBT**		_buckets;
private:
	NameCache(const NameCache&);
	void operator=(const NameCache&);

	NCR __ncr_finder;
};

} /* namespace::rescached */

#endif
// vi: ts=8 sw=8 tw=78:
