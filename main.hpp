/**
 * Copyright (C) 2009 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#ifndef _RESCACHED_MAIN_HPP
#define	_RESCACHED_MAIN_HPP	1

#include "Dlogger.hpp"

using vos::Dlogger;

#define	RESCACHED_CONF		"rescached.cfg"
#define	RESCACHED_CONF_HEAD	"RESCACHED"
#define	RESCACHED_DATA_BAK_EXT	".bak"
#define	RESCACHED_DATA		"rescached.vos"
#define	RESCACHED_LOG		"rescached.log"
#define	RESCACHED_MD		"rescached.vmd"
#define	RESCACHED_PID		"rescached.pid"
#define	RESCACHED_CACHE_MAX	100000
#define	RESCACHED_CACHE_MAX_S	"100000"

#define	RESCACHED_DEF_PARENT	"8.8.8.8, 8.8.4.4, 208.67.222.222, 208.67.220.220"
#define	RESCACHED_DEF_LISTEN	"0.0.0.0"
#define	RESCACHED_DEF_PORT	53
#define	RESCACHED_DEF_THRESHOLD	1

extern long int	_cache_max;
extern long int	_cache_thr;
extern int	RESCACHED_DEBUG;
extern Dlogger	dlog;

#endif
