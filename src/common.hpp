/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_COMMON_HPP
#define	_RESCACHED_COMMON_HPP	1

#include "lib/Dlogger.hpp"

using vos::Dlogger;

namespace rescached {

enum _cache_mode_idx {
	CACHE_IS_PERMANENT	= 1
,	CACHE_IS_TEMPORARY	= 2
};

extern Dlogger	dlog;
extern int	_dbg;
extern int	_cache_mode;

#define	DBG_LVL_IS_1	(_dbg >= 1)
#define	DBG_LVL_IS_2	(_dbg >= 2)

} /* namespace::rescached */

#endif
