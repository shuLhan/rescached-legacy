/**
 * Copyright 2017 M. Shulhan (ms@kilabit.info). All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _RESOLVER_HH
#define	_RESOLVER_HH	1

#include "libvos.hh"
#include "Buffer.hh"
#include "Dlogger.hh"
#include "Resolver.hh"

#define DEF_SERVER "127.0.0.1:53"
#define DEF_QTYPE "A"

using vos::Buffer;
using vos::Dlogger;
using vos::DNSQuery;
using vos::Resolver;

#endif
