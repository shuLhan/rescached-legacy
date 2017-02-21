#!/usr/bin/make
##
## Copyright 2017 M. Shulhan (ms@kilabit.info). All rights reserved.
## Use of this source code is governed by a BSD-style license that can be
## found in the LICENSE file.
##

SRC_D		=.
BLD_D		=build

LIBVOS_D	=../lib

include $(LIBVOS_D)/Makefile

PRE_TARGET	+= $(BLD_D)
LIBVOS_OPTS	+=NO_DEFAULT_LIBS

TARGET		=$(BLD_D)/resolver
TARGET_OBJS	=\
		$(BLD_D)/main.oo		\
		$(LIBVOS_BLD_D)/libvos.oo	\
		$(LIBVOS_BLD_D)/Object.oo	\
		$(LIBVOS_BLD_D)/BNode.oo	\
		$(LIBVOS_BLD_D)/List.oo		\
		$(LIBVOS_BLD_D)/Locker.oo	\
		$(LIBVOS_BLD_D)/Buffer.oo	\
		$(LIBVOS_BLD_D)/File.oo		\
		$(LIBVOS_BLD_D)/Dlogger.oo	\
		$(LIBVOS_BLD_D)/DNS_rr.oo	\
		$(LIBVOS_BLD_D)/DNSQuery.oo	\
		$(LIBVOS_BLD_D)/DNSRecordType.oo\
		$(LIBVOS_BLD_D)/Socket.oo	\
		$(LIBVOS_BLD_D)/SockAddr.oo	\
		$(LIBVOS_BLD_D)/Resolver.oo	\
		$(LIBVOS_BLD_D)/List.oo		\
		$(LIBVOS_BLD_D)/Rowset.oo	\
		$(LIBVOS_BLD_D)/SSVReader.oo


.PHONY: all debug clean distclean install uninstall

clean:
	@$(call do_rmdir,$(BLD_D))

distclean: libvos-clean clean

all: libvos-all
	@strip -s $(BLD_D)/resolver;

debug: libvos-debug

$(BLD_D):
	@$(call do_mkdir,$(BLD_D))

$(TARGET): $(TARGET_OBJS)
	@$(do_build)

$(BLD_D)/%.oo: $(SRC_D)/%.cc $(SRC_D)/%.hh
	@$(do_compile)

install:
	@if [[ $${UID} != 0 ]]; then					\
		echo " >> User root needed for installation!";		\
		exit 1;							\
	fi;								\
									\
	echo "";							\
	echo " >> Installing resolver...";				\
	$(call do_mkdir,$(INSTALL_BIN_D))				\
	$(call do_install,$(TARGET),$(INSTALL_BIN_D))

uninstall:
	@if [[ $${UID} != 0 ]]; then					\
		echo " >> User root needed for uninstallation!";	\
		exit 1;							\
	fi;								\

##
## vi: ts=8 sw=8 tw=78:
