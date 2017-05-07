/**
 * Copyright 2017 M. Shulhan (ms@kilabit.info). All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "main.hh"

Resolver R;
Dlogger D;

void usage()
{
	int x = 0;

	D.er("resolver [@parent-resolver[:port]] [type] hostname\n\n");
	D.er("Supported type:\n\t");

	for (; x < vos::DNSRecordType::SIZE; x++) {
		if (x > 0) {
			D.er(" ");
		}
		D.er("%s", vos::DNSRecordType::NAMES[x]);
	}
	D.er("\n");

	exit(1);
}

int query(List* ns, const char* stype, const char* sname)
{
	int s = 0;
	int x = 0;
	DNSQuery Q;
	DNSQuery A;
	const char* address = NULL;

	while (x < ns->size()) {
		address = ns->at(x)->chars();

		s = R.add_server(address);
		if (s) {
			D.er("Invalid server address: %s\n", address);
			return -1;
		}

		x++;
	}

	s = vos::DNSRecordType::GET_VALUE(stype);
	if (s < 0) {
		D.er("Invalid type: %s\n", stype);
		return -1;
	}

	R.init();

	Q.create_question(sname, s);

	Resolver::N_TRY = 3;

	s = R.resolve(&Q, &A);
	if (s) {
		exit(1);
	}

	D.out("%s\n", A.chars());

	return 0;
}

List* load_etc_resolv()
{
	int x = 0;
	List* ns = NULL;
	List* cols = NULL;
	Rowset* rs = NULL;
	const char* v = NULL;

	SSVReader reader('#');

	x = reader.load(DEF_ETC_RESOLV);
	if (x) {
		D.er("Fail to load %s!\n", DEF_ETC_RESOLV);
		return NULL;
	}

	ns = new List();
	rs = reader._rows;

	x = 0;
	while (x < rs->size()) {
		cols = (List*) rs->at(x);

		if (cols->size() <= 1) {
			D.er("Col size <= 1\n");
			goto cont;
		}

		v = cols->at(0)->chars();
		if (strcasecmp(v,  "nameserver") != 0) {
			D.er("%s is not nameserver\n", v);
			goto cont;
		}

		v = cols->at(1)->chars();
		if (! SockAddr::IS_IPV4(v)) {
			D.er("%s is not IPv4\n", v);
			goto cont;
		}

		ns->push_tail(new Buffer(v));
cont:
		x++;
	}

	if (ns->size() <= 0) {
		D.er("No nameserver defined in %s\n", DEF_ETC_RESOLV);
		delete ns;
		ns = NULL;
	}

	return ns;
}

int main(int argc, char* argv[])
{
	int s = 1;
	List* ns = NULL;

	switch (argc) {
	// $ resolver
	case 1:
		goto err;
	case 2:
		//
		// $ resolver @string
		//
		if (argv[1][0] == '@') {
			goto err;
		}

		ns = load_etc_resolv();
		if (!ns) {
			return 1;
		}

		//
		// $ resolver hostname
		//
		s = query(ns, DEF_QTYPE, argv[1]);
		break;
	case 3:
		//
		// $ resolver @127.0.0.1 hostname
		//
		if (argv[1][0] == '@') {
			ns = new List();

			ns->push_tail(new Buffer(&argv[1][1]));

			s = query(ns, DEF_QTYPE, argv[2]);
		//
		// $ resolver type hostname
		//
		} else {
			ns = load_etc_resolv();
			if (!ns) {
				return 1;
			}

			s = query(ns, argv[1], argv[2]);
		}
		break;
	case 4:
		//
		// $ resolver @127.0.0.1 TYPE hostname
		//
		if (argv[1][0] != '@') {
			goto err;
		}

		ns = new List();

		ns->push_tail(new Buffer(&argv[1][1]));

		s = query(ns, argv[2], argv[3]);
	}
err:
	if (ns) {
		delete ns;
	}
	if (s) {
		usage();
	}

	return 0;
}
