##
# Copyright (C) 2010 kilabit.org
# Author:
#	- m.shulhan (ms@kilabit.org)
##
SRC_D		=.
BLD_D		=build
SCRIPTS_D		=$(SRC_D)/scripts

INSTALL_BIN_D	=/usr/sbin
INSTALL_CFG_D	=/etc/rescached
INSTALL_CACHE_D	=/var/cache/rescached

LIBVOS_D	=lib

include $(LIBVOS_D)/Makefile

SERVICE_NAME	=rescached

TARGET		=$(BLD_D)/rescached
RESCACHED_CFG	=$(SRC_D)/rescached.cfg
RESCACHED_SVC	=
INSTALL_SVC_D	=
SYSTEM_SVC	=

TARGET_OBJS	=					\
			$(BLD_D)/main.oo		\
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
			$(LIBVOS_BLD_D)/File.oo		\
			$(LIBVOS_BLD_D)/Buffer.oo	\
			$(LIBVOS_BLD_D)/libvos.oo

PRE_TARGET	+= $(BLD_D)

.PHONY: all debug clean install install-arch uninstall

all: libvos-all

debug: libvos-debug

$(BLD_D):
	@$(call do_mkdir,$(BLD_D))

$(BLD_D)/rescached: $(TARGET_OBJS)
	@$(do_build)

$(BLD_D)/%.oo: $(SRC_D)/%.cpp $(SRC_D)/%.hpp
	@$(do_compile)

do-install:
	@if [[ $${UID} != 0 ]]; then					\
		echo " >> User root needed for installation!";		\
		exit 1;							\
	fi;								\
	echo "";							\
	echo " >> Creating directories ...";				\
	$(call do_mkdir,$(INSTALL_BIN_D))				\
	$(call do_mkdir,$(INSTALL_CFG_D))				\
	$(call do_mkdir,$(INSTALL_SVC_D))				\
	$(call do_mkdir,$(INSTALL_CACHE_D))				\
	echo "";							\
	echo " >> Installing program and configuration ...";		\
	$(call do_install,$(TARGET),$(INSTALL_BIN_D))			\
	if [ ! -f $(INSTALL_CFG_D)/$(RESCACHED_CFG) ]; then		\
		$(call do_install,$(RESCACHED_CFG),$(INSTALL_CFG_D))	\
	fi;								\
	echo "";							\
	echo " >> Installing service ...";				\
	$(call do_install,$(RESCACHED_SVC),$(SYSTEM_SVC))		\
	$(call do_link,$(SYSTEM_SVC),$(INSTALL_BIN_D)/rcrescached)


install: INSTALL_SVC_D=/etc/init.d
install: RESCACHED_SVC=$(SCRIPTS_D)/rescached.run
install: SYSTEM_SVC=$(INSTALL_SVC_D)/$(SERVICE_NAME)
install: do-install

install-arch: INSTALL_SVC_D=/etc/rc.d
install-arch: RESCACHED_SVC=$(SCRIPTS_D)/rescached.arch
install-arch: SYSTEM_SVC=$(INSTALL_SVC_D)/$(SERVICE_NAME)
install-arch: do-install

uninstall:
	@if [[ $$(UID) != 0 ]]; then					\
		echo " >> User root needed for installation!";		\
		exit 1;							\
	fi;								\
	rm -f $(INSTALL_BIN_D)/rcrescached;				\
	rm -f $(RESCACHED_SVC);						\
	rm -f $(INSTALL_CFG_D)/rescached.cfg;				\
	rm -f $(INSTALL_BIN_D)/rescached;				\
	rmdir $(INSTALL_CFG_D);

clean:
	@$(call do_rmdir,$(BLD_D))

distclean: libvos-clean clean
