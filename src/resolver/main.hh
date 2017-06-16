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
#include "List.hh"
#include "SSVReader.hh"
#include "Resolver.hh"

#define DEF_ETC_RESOLV "/etc/resolv.conf"
#define DEF_QTYPE "A"

using vos::Error;
using vos::Buffer;
using vos::Dlogger;
using vos::DNSQuery;
using vos::List;
using vos::Rowset;
using vos::SSVReader;
using vos::Resolver;
using vos::SockAddr;

#endif
