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

	D.er("resolver [@parent-resolver[:port]] hostname [type]\n\n");
	D.er("Supported type:\n\t");

	for (; x < vos::RR_TYPE_SIZE; x++) {
		if (x > 0) {
			D.er(" ");
		}
		D.er("%s", vos::RR_TYPE_LIST[x]);
	}
	D.er("\n");

	exit(1);
}

int query(const char* server, const char* sname, const char* stype)
{
	int s;
	DNSQuery Q;
	DNSQuery A;

	s = R.set_server(server);
	if (s) {
		D.er("Invalid server address: %s\n", server);
		return -1;
	}
	s = R.CONVERT_TYPE(stype);
	if (s < 0) {
		D.er("Invalid type: %s\n", stype);
		return -1;
	}

	R.init();

	Q.create_question(sname, s);

	R.resolve(&Q, &A);

	D.out("%s\n", A.chars());

	return 0;
}

int main(int argc, char* argv[])
{
	int s;

	if (argc <= 1) {
	}

	switch (argc) {
	case 1:
		usage();
	case 2:
		s = query(DEF_SERVER, argv[1], DEF_QTYPE);
		if (s) {
			usage();
		}
		break;
	case 3:
		if (argv[1][0] == '@') {
			s = query(&argv[1][1], argv[2], DEF_QTYPE);
		} else {
			s = query(DEF_SERVER, argv[1], argv[2]);
		}
		break;
	}

	return 0;
}
