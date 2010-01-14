#include "Rescached.hpp"

namespace rescached {

int			RESCACHED_DEBUG	= (getenv("RESCACHED_DEBUG") == NULL)
						? 0
						: atoi(getenv("RESCACHED_DEBUG"));
Dlogger			dlog;
long int		_cache_max	= RESCACHED_CACHE_MAX;
long int		_cache_thr	= RESCACHED_DEF_THRESHOLD;

} /* namespace::rescached */
