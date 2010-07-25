/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "Rescached.hpp"

namespace rescached {

int			_debug_lvl	= (getenv("RESCACHED_DEBUG") == NULL)
						? RESCACHED_DEF_DEBUG
						: atoi(getenv("RESCACHED_DEBUG"));
Dlogger			dlog;
long int		_cache_max	= RESCACHED_CACHE_MAX;
long int		_cache_thr	= RESCACHED_DEF_THRESHOLD;

} /* namespace::rescached */
