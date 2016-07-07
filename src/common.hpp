/*
 * Copyright 2010-2016 Mhd Sulhan (ms@kilabit.info)
 */

#ifndef _RESCACHED_COMMON_HPP
#define	_RESCACHED_COMMON_HPP	1

#include "lib/Dlogger.hpp"

using vos::Dlogger;

namespace rescached {

extern Dlogger	dlog;
extern int	_dbg;
extern uint32_t	_cache_minttl;
extern short	_skip_log;

#define	DBG_LVL_IS_1	(_dbg >= 1)
#define	DBG_LVL_IS_2	(_dbg >= 2)
#define	DBG_LVL_IS_3	(_dbg >= 3)

} /* namespace::rescached */

#endif
