//
// Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "common.hh"

namespace rescached {

Dlogger		dlog;
int		_dbg		= 0;
uint32_t	_cache_minttl	= 60;
short		_skip_log	= 1;

Resolver	_resolver;
SockServer	_srvr_udp;
uint8_t		_dns_conn_t = RESCACHED_DEF_CONN_T;
uint8_t		_rto = RESCACHED_DEF_TIMEOUT;
uint8_t		_running = 0;

} /* namespace::rescached */
// vi: ts=8 sw=8 tw=78:
