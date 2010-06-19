##
# Copyright (C) 2009 kilabit.org
# Author:
#	- m.shulhan (ms@kilabit.org)
##
SRC_D		=.
BLD_D		=build
SCR_D		=$(SRC_D)/scripts

INSTALL_BIN_D	=/usr/sbin
INSTALL_CFG_D	=/etc/rescached
INSTALL_SVC_D	=/etc/init.d
INSTALL_CACHE_D	=/var/cache/rescached

LIBVOS_D	=lib

include $(LIBVOS_D)/Makefile

TARGET		=$(BLD_D)/rescached
RESCACHED_CFG	=$(SRC_D)/rescached.cfg
RESCACHED_RUN	=$(SCR_D)/rescached.run
RESCACHED_SVC	=$(INSTALL_SVC_D)/rescached

TARGET_OBJS	=					\
			$(BLD_D)/main.oo		\
			$(BLD_D)/Rescached.oo		\
			$(BLD_D)/ResThread.oo		\
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
			$(LIBVOS_BLD_D)/Socket.oo	\
			$(LIBVOS_BLD_D)/SockAddr.oo	\
			$(LIBVOS_BLD_D)/Dlogger.oo	\
			$(LIBVOS_BLD_D)/ConfigData.oo	\
			$(LIBVOS_BLD_D)/Config.oo	\
			$(LIBVOS_BLD_D)/File.oo		\
			$(LIBVOS_BLD_D)/Buffer.oo	\
			$(LIBVOS_BLD_D)/libvos.oo

PRE_TARGET	+= $(BLD_D)

.PHONY: all debug clean install uninstall

all: libvos-all

debug: libvos-debug

$(BLD_D):
	@$(call do_mkdir,$(BLD_D))

$(BLD_D)/rescached: $(TARGET_OBJS)
	@$(do_build)

$(BLD_D)/%.oo: $(SRC_D)/%.cpp $(SRC_D)/%.hpp
	@$(do_compile)

install:
	@if [[ $$(UID) != 0 ]]; then					\
		echo " >> User root needed for installation!";		\
		exit 1;							\
	fi;								\
	mkdir -p $(INSTALL_BIN_D);					\
	mkdir -p $(INSTALL_CFG_D);					\
	mkdir -p $(INSTALL_SVC_D);					\
	mkdir -p $(INSTALL_CACHE_D);					\
	echo " >> Installing program and configuration ...";		\
	$(call do_install,$(TARGET),$(INSTALL_BIN_D))			\
	if [ ! -f $(INSTALL_CFG_D)/$(RESCACHED_CFG) ]; then		\
		$(call do_install,$(RESCACHED_CFG),$(INSTALL_CFG_D))	\
	fi;								\
	echo "";							\
	echo " >> Installing service ...";				\
	$(call do_install,$(RESCACHED_RUN),$(RESCACHED_SVC))		\
	$(call do_link,$(RESCACHED_SVC),$(INSTALL_BIN_D)/rcrescached)	\

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
