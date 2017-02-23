#!/usr/bin/make
##
## Copyright 2009-2016 M. Shulhan (ms@kilabit.info). All rights reserved.
## Use of this source code is governed by a BSD-style license that can be
## found in the LICENSE file.
##

SRC_D		=.
BLD_D		=build
SCRIPTS_D	=$(SRC_D)/scripts
DATA_D		=..
DOC_D		=$(DATA_D)/doc
DESTDIR		=

export INSTALL_BIN_D	=$(DESTDIR)/usr/bin
INSTALL_CFG_D	=$(DESTDIR)/etc/rescached
INSTALL_CACHE_D	=$(DESTDIR)/var/cache/rescached
INSTALL_DATA_D	=$(DESTDIR)/usr/share/rescached
INSTALL_MAN_D	=$(DESTDIR)/usr/share/man

LIBVOS_D	=lib

include $(LIBVOS_D)/Makefile

SERVICE_NAME	=rescached

TARGET		=$(BLD_D)/rescached

RESCACHED_MAN=		$(DOC_D)/rescached.1.gz
RESCACHED_CONF_MAN=	$(DOC_D)/rescached.conf.5.gz
RESOLVER_MAN=		$(DOC_D)/resolver.1.gz

DOC_TARGET=		$(RESCACHED_MAN)		\
			$(RESCACHED_CONF_MAN)		\
			$(RESOLVER_MAN)

RESCACHED_CFG	=$(SRC_D)/rescached.cfg
RESCACHED_SVC	=
SVC_INIT_D	=/etc/init.d
SVC_SYSTEMD	=/usr/lib/systemd/system
SVC_D		=
INSTALL_SVC_D	=
SYSTEM_SVC	=

##
## METADATA_CHANGE: If set to non-zero, old cache file will be removed on
## installation due to change in cache metadata format.
##
METADATA_CHANGE	=0

TARGET_OBJS	=					\
			$(BLD_D)/main.oo		\
			$(BLD_D)/common.oo		\
			$(BLD_D)/Rescached.oo		\
			$(BLD_D)/ClientWorker.oo	\
			$(BLD_D)/ResolverWorker.oo	\
			$(BLD_D)/ResQueue.oo		\
			$(BLD_D)/NameCache.oo		\
			$(BLD_D)/NCR.oo			\
			$(LIBVOS_BLD_D)/RBT.oo		\
			$(LIBVOS_BLD_D)/TreeNode.oo	\
			$(LIBVOS_BLD_D)/DSVWriter.oo	\
			$(LIBVOS_BLD_D)/DSVReader.oo	\
			$(LIBVOS_BLD_D)/DSVRecord.oo	\
			$(LIBVOS_BLD_D)/DSVRecordMD.oo	\
			$(LIBVOS_BLD_D)/Resolver.oo	\
			$(LIBVOS_BLD_D)/DNSQuery.oo	\
			$(LIBVOS_BLD_D)/DNS_rr.oo	\
			$(LIBVOS_BLD_D)/DNSRecordType.oo\
			$(LIBVOS_BLD_D)/SockAddr.oo	\
			$(LIBVOS_BLD_D)/Socket.oo	\
			$(LIBVOS_BLD_D)/SockServer.oo	\
			$(LIBVOS_BLD_D)/Dlogger.oo	\
			$(LIBVOS_BLD_D)/ConfigData.oo	\
			$(LIBVOS_BLD_D)/Config.oo	\
			$(LIBVOS_BLD_D)/SSVReader.oo	\
			$(LIBVOS_BLD_D)/Rowset.oo	\
			$(LIBVOS_BLD_D)/File.oo		\
			$(LIBVOS_BLD_D)/Buffer.oo	\
			$(LIBVOS_BLD_D)/List.oo		\
			$(LIBVOS_BLD_D)/BNode.oo	\
			$(LIBVOS_BLD_D)/Thread.oo	\
			$(LIBVOS_BLD_D)/Locker.oo	\
			$(LIBVOS_BLD_D)/Object.oo	\
			$(LIBVOS_BLD_D)/libvos.oo

PRE_TARGET	+= $(BLD_D)
LIBVOS_OPTS	+=NO_DEFAULT_LIBS

.PHONY: all doc debug clean install install-arch uninstall

all: libvos-all
	@strip -s $(BLD_D)/rescached;
	$(MAKE) -C resolver

debug: libvos-debug
	$(MAKE) -C resolver debug


$(BLD_D):
	@$(call do_mkdir,$(BLD_D))

$(BLD_D)/rescached: $(TARGET_OBJS)
	@$(do_build)

##
## Build documentation (man pages)
##
doc: $(DOC_TARGET)

$(RESCACHED_MAN): $(DATA_D)/README.adoc
	@echo ">> Generating man page for rescached ..."
	@a2x -v --doctype manpage --format manpage -D $(DOC_D) $< 2>/dev/null
	@gzip -f $(DOC_D)/rescached.1

$(RESCACHED_CONF_MAN): $(DOC_D)/rescached.conf.adoc
	@echo ">> Generating man page for rescached.conf ..."
	@a2x -v --doctype manpage --format manpage -D $(DOC_D) $< 2>/dev/null
	@gzip -f $(DOC_D)/rescached.conf.5

$(RESOLVER_MAN): $(DOC_D)/resolver.adoc
	@echo ">> Generating man page for resolver ..."
	@a2x -v --doctype manpage --format manpage -D $(DOC_D) $< 2>/dev/null
	@gzip -f $(DOC_D)/resolver.1


$(BLD_D)/%.oo: $(SRC_D)/%.cc $(SRC_D)/%.hh
	@$(do_compile)

do-install:
	@if [[ $${UID} != 0 ]]; then					\
		echo " >> User root needed for installation!";		\
		exit 1;							\
	fi;								\
									\
	if [[ $(METADATA_CHANGE) != 0 ]]; then				\
		echo "";						\
		echo " >> removing old caches ...";			\
		$(call do_rm,$(INSTALL_CACHE_D)/rescached.vos)		\
	fi;								\
									\
	echo "";							\
	echo " >> Creating directories ...";				\
	$(call do_mkdir,$(INSTALL_BIN_D))				\
	$(call do_mkdir,$(INSTALL_CFG_D))				\
	$(call do_mkdir,$(INSTALL_SVC_D))				\
	$(call do_mkdir,$(INSTALL_CACHE_D))				\
	$(call do_rmdir,$(INSTALL_DATA_D))				\
	$(call do_mkdir,$(INSTALL_DATA_D))				\
	$(call do_mkdir,$(INSTALL_MAN_D))				\
									\
	echo "";							\
	echo " >> Installing program and configuration ...";		\
	$(call do_install,$(BLD_D)/rescached,$(INSTALL_BIN_D))		\
	$(call do_install,$(SCRIPTS_D)/rescached-update-hosts-block.sh,$(INSTALL_BIN_D))\
	if [ ! -f $(INSTALL_CFG_D)/$(RESCACHED_CFG) ]; then		\
		$(call do_install,$(RESCACHED_CFG),$(INSTALL_CFG_D))	\
	fi;								\
	$(call do_install,$(SRC_D)/hosts.block,$(INSTALL_CFG_D))	\
									\
	echo "";							\
	echo " >> Installing service ...";				\
	$(call do_install,$(RESCACHED_SVC),$(SYSTEM_SVC))		\
									\
	echo "";							\
	echo " >> Installing data ...";					\
	$(call do_install,$(DATA_D)/LICENSE,$(INSTALL_DATA_D))		\
	$(call do_install,$(DATA_D)/README.adoc,$(INSTALL_DATA_D))	\
	$(call do_install_dir,$(DATA_D)/doc/dev,$(INSTALL_DATA_D))	\
	$(call do_install,$(RESCACHED_MAN),$(INSTALL_MAN_D)/man1)
	$(call do_install,$(RESCACHED_CONF_MAN),$(INSTALL_MAN_D)/man5)
	$(call do_install,$(RESOLVER_MAN),$(INSTALL_MAN_D)/man1)


do-uninstall:
	@if [[ $${UID} != 0 ]]; then					\
		echo " >> User root needed for installation!";		\
		exit 1;							\
	fi;								\
									\
	$(call do_rm,$(INSTALL_BIN_D)/rcrescached)			\
	$(call do_rm,$(SYSTEM_SVC))					\
	$(call do_rm,$(INSTALL_CFG_D)/rescached.cfg)			\
	$(call do_rm,$(INSTALL_BIN_D)/rescached)			\
	$(call do_rmdir,$(INSTALL_CACHE_D))				\
	$(call do_rmdir,$(INSTALL_CFG_D))				\
	$(call do_rmdir,$(INSTALL_DATA_D))


install: SVC_D=$(SVC_SYSTEMD)
install: INSTALL_SVC_D=$(DESTDIR)$(SVC_D)
install: RESCACHED_SVC=$(SCRIPTS_D)/rescached.service
install: SYSTEM_SVC=$(INSTALL_SVC_D)/rescached.service
install: all do-install
	$(MAKE) -C resolver install

install-init: SVC_D=$(SVC_INIT_D)
install-init: INSTALL_SVC_D=$(DESTDIR)$(SVC_D)
install-init: RESCACHED_SVC=$(SCRIPTS_D)/rescached.run
install-init: SYSTEM_SVC=$(INSTALL_SVC_D)/$(SERVICE_NAME)
install-init: all do-install

uninstall: SVC_D=$(SVC_SYSTEMD)
uninstall: INSTALL_SVC_D=$(DESTDIR)$(SVC_D)
uninstall: SYSTEM_SVC=$(INSTALL_SVC_D)/rescached.service
uninstall: do-uninstall

uninstall-init: SVC_D=$(SVC_INIT_D)
uninstall-init: INSTALL_SVC_D=$(DESTDIR)$(SVC_D)
uninstall-init: SYSTEM_SVC=$(INSTALL_SVC_D)/$(SERVICE_NAME)
uninstall-init: do-uninstall

clean:
	@$(call do_rmdir,$(BLD_D))
	$(MAKE) -C resolver clean

distclean: libvos-clean clean
	$(MAKE) -C resolver distclean

## vi: ts=8 sw=8 tw=78:
