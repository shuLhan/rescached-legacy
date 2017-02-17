//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _RESCACHED_COMMON_HH
#define _RESCACHED_COMMON_HH 1

#include "lib/Dlogger.hh"
#include "lib/SockServer.hh"
#include "lib/Resolver.hh"

using vos::Dlogger;
using vos::SockServer;
using vos::Resolver;

namespace rescached {

extern Dlogger	dlog;
extern int	_dbg;
extern uint32_t	_cache_minttl;
extern short	_skip_log;

extern Resolver		_resolver;
extern SockServer	_srvr_udp;
extern uint8_t		_dns_conn_t;
extern uint8_t		_rto;
extern uint8_t		_running;

#define	DBG_LVL_IS_1	(_dbg >= 1)
#define	DBG_LVL_IS_2	(_dbg >= 2)
#define	DBG_LVL_IS_3	(_dbg >= 3)

#define	RESCACHED_DEF_CONN_T	0
#define	RESCACHED_DEF_TIMEOUT	7

} /* namespace::rescached */

#endif
// vi: ts=8 sw=8 tw=78:
