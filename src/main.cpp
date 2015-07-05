/*
 * Copyright (C) 2010 kilabit.org
 * Author:
 *	- m.shulhan (ms@kilabit.org)
 */

#include "main.hpp"

using namespace rescached;

volatile sig_atomic_t	_SIG_lock_	= 0;
static int		_got_signal_	= 0;
static Rescached	R;

static void rescached_interrupted(int sig_num)
{
	switch (sig_num) {
	case SIGUSR1:
		/* send interrupt to select() */
		break;
	case SIGSEGV:
		if (_SIG_lock_) {
			::raise(sig_num);
		}
		_SIG_lock_	= 1;
		_got_signal_	= sig_num;
		R.exit();
		_SIG_lock_	= 0;

		::signal(sig_num, SIG_DFL);
		break;
	case SIGTERM:
	case SIGINT:
	case SIGQUIT:
		if (_SIG_lock_) {
			::raise(sig_num);
		}
		_SIG_lock_	= 1;
		_got_signal_ 	= sig_num;
		R._running	= 0;
		_SIG_lock_	= 0;
                break;
        }
}

static void rescached_set_signal_handle()
{
	struct sigaction sig_new;

	memset(&sig_new, 0, sizeof(struct sigaction));
	sig_new.sa_handler = rescached_interrupted;
	sigemptyset(&sig_new.sa_mask);
	sig_new.sa_flags = 0;

	sigaction(SIGINT, &sig_new, 0);
	sigaction(SIGQUIT, &sig_new, 0);
	sigaction(SIGTERM, &sig_new, 0);
	sigaction(SIGSEGV, &sig_new, 0);
	sigaction(SIGUSR1, &sig_new, 0);
}

int main(int argc, char *argv[])
{
	int s = -1;

	if (argc == 1) {
		s = R.init(NULL);
	} else if (argc == 2) {
		s = R.init(argv[1]);
	} else {
		dlog.er("\n Usage: rescached <rescached-config>\n ");
	}
	if (s != 0) {
		goto err;
	}

	_skip_log = 0;

	rescached_set_signal_handle();
	R.run();
err:
	if (s) {
		perror(NULL);
	}
	R.exit();

	return s;
}
