##
## Copyright (C) 2014 kilabit.info
## Author:
##	- m.shulhan (ms@kilabit.info)
##
SRC_D		=.
BLD_D		=build
SCRIPTS_D	=$(SRC_D)/scripts
DATA_D		=..
DESTDIR		=

INSTALL_BIN_D	=$(DESTDIR)/usr/bin
INSTALL_CFG_D	=$(DESTDIR)/etc/rescached
INSTALL_CACHE_D	=$(DESTDIR)/var/cache/rescached
INSTALL_DATA_D	=$(DESTDIR)/usr/share/rescached
INSTALL_MAN_D	=$(DESTDIR)/usr/share/man/man1

LIBVOS_D	=lib

include $(LIBVOS_D)/Makefile

SERVICE_NAME	=rescached

TARGET		=$(BLD_D)/rescached
TARGET		+=$(BLD_D)/rescached.1.gz
RESCACHED_CFG	=$(SRC_D)/rescached.cfg
RESCACHED_SVC	=
SVC_INIT_D	=/etc/init.d
SVC_SYSTEMD	=/etc/systemd/system
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
			$(BLD_D)/ResQueue.oo		\
			$(BLD_D)/NameCache.oo		\
			$(BLD_D)/NCR.oo			\
			$(BLD_D)/NCR_Tree.oo		\
			$(BLD_D)/NCR_List.oo		\
			$(LIBVOS_BLD_D)/Writer.oo	\
			$(LIBVOS_BLD_D)/Reader.oo	\
			$(LIBVOS_BLD_D)/Record.oo	\
			$(LIBVOS_BLD_D)/RecordMD.oo	\
			$(LIBVOS_BLD_D)/Resolver.oo	\
			$(LIBVOS_BLD_D)/DNSQuery.oo	\
			$(LIBVOS_BLD_D)/DNS_rr.oo	\
			$(LIBVOS_BLD_D)/SockAddr.oo	\
			$(LIBVOS_BLD_D)/Socket.oo	\
			$(LIBVOS_BLD_D)/SockServer.oo	\
			$(LIBVOS_BLD_D)/Dlogger.oo	\
			$(LIBVOS_BLD_D)/ConfigData.oo	\
			$(LIBVOS_BLD_D)/Config.oo	\
			$(LIBVOS_BLD_D)/SSVReader.oo	\
			$(LIBVOS_BLD_D)/File.oo		\
			$(LIBVOS_BLD_D)/Buffer.oo	\
			$(LIBVOS_BLD_D)/libvos.oo

PRE_TARGET	+= $(BLD_D)
LIBVOS_OPTS	+=NO_DEFAULT_LIBS

.PHONY: all debug clean install install-arch uninstall

all: libvos-all

debug: libvos-debug

$(BLD_D):
	@$(call do_mkdir,$(BLD_D))

$(BLD_D)/rescached: $(TARGET_OBJS)
	@$(do_build)

$(BLD_D)/rescached.1.gz: $(DATA_D)/README.adoc
	@a2x --doctype manpage --format manpage -D $(BLD_D) $< 2>/dev/null
	@gzip $(BLD_D)/rescached.1

$(BLD_D)/%.oo: $(SRC_D)/%.cpp $(SRC_D)/%.hpp
	@$(do_compile)

do-install:
	@if [[ $${UID} != 0 ]]; then					\
		echo " >> User root needed for installation!";		\
		exit 1;							\
	fi;								\
									\
	if [[ -f $(SYSTEM_SVC) ]]; then					\
		echo "";						\
		echo " >> stopping rescached service ...";		\
		$(SYSTEM_SVC) stop;					\
	fi;								\
									\
	if [[ $(METADATA_CHANGE) != 0 ]]; then				\
		echo "";						\
		echo " >> removing old caches ...";			\
		$(call do_rm,$(INSTALL_CACHE_D)/rescached.vos)		\
		$(call do_rm,$(INSTALL_CACHE_D)/rescached.vos.bak)	\
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
	$(call do_install,$(TARGET),$(INSTALL_BIN_D))			\
	if [ ! -f $(INSTALL_CFG_D)/$(RESCACHED_CFG) ]; then		\
		$(call do_install,$(RESCACHED_CFG),$(INSTALL_CFG_D))	\
	fi;								\
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
	$(call do_install,$(BLD_D)/rescached.1.gz,$(INSTALL_MAN_D))


do-uninstall:
	@if [[ $${UID} != 0 ]]; then					\
		echo " >> User root needed for installation!";		\
		exit 1;							\
	fi;								\
									\
	if [[ -f $(SYSTEM_SVC) ]]; then					\
		echo "";						\
		echo " >> stopping rescached service ...";		\
		$(SYSTEM_SVC) stop;					\
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

distclean: libvos-clean clean
