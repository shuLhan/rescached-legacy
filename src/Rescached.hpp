/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_HPP
#define	_RESCACHED_HPP	1

#include "Dlogger.hpp"

using vos::Dlogger;

namespace rescached {

#define	RESCACHED_CONF		"rescached.cfg"
#define	RESCACHED_CONF_HEAD	"RESCACHED"
#define	RESCACHED_DATA_BAK_EXT	".bak"
#define	RESCACHED_DATA		"rescached.vos"
#define	RESCACHED_LOG		"rescached.log"
#define	RESCACHED_PID		"rescached.pid"
#define	RESCACHED_CACHE_MAX	100000
#define	RESCACHED_CACHE_MAX_S	"100000"

#define	RESCACHED_DEF_PARENT	"8.8.8.8, 8.8.4.4"
#define	RESCACHED_DEF_LISTEN	"127.0.0.1"
#define	RESCACHED_DEF_PORT	53
#define	RESCACHED_DEF_THRESHOLD	1
#define	RESCACHED_DEF_N_THREAD	2
#define	RESCACHED_DEF_DEBUG	0

extern long int	_cache_max;
extern long int	_cache_thr;
extern int	_debug_lvl;
extern Dlogger	dlog;

#define	DBG_LVL_IS_1	(_debug_lvl >= 1)
#define	DBG_LVL_IS_2	(_debug_lvl >= 2)

#define RESCACHED_MD	\
":name:::'|',"		\
":stat:::'|',"		\
":type:::'|',"		\
":question:::'|':BLOB,"	\
":answer:::'|':BLOB"

} /* namespace::rescached */

#endif
