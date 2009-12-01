##
# Copyright (C) 2009 kilabit.org
# Author:
#	- m.shulhan (ms@kilabit.org)
##

SRC_D		=.
BLD_D		=build
LIBVOS_D	=../../libvos/src

include ${LIBVOS_D}/Makefile

TARGET		=${BLD_D}/rescached

RESCACHED_OBJS	=					\
			${BLD_D}/main.oo		\
			${BLD_D}/NameCache.oo		\
			${BLD_D}/NCR.oo			\
			${BLD_D}/NCR_Tree.oo		\
			${BLD_D}/NCR_List.oo		\
			${LIBVOS_BLD_D}/Writer.oo	\
			${LIBVOS_BLD_D}/Reader.oo	\
			${LIBVOS_BLD_D}/Record.oo	\
			${LIBVOS_BLD_D}/RecordMD.oo	\
			${LIBVOS_BLD_D}/Resolver.oo	\
			${LIBVOS_BLD_D}/DNSQuery.oo	\
			${LIBVOS_BLD_D}/DNS_rr.oo	\
			${LIBVOS_BLD_D}/Socket.oo	\
			${LIBVOS_BLD_D}/Dlogger.oo	\
			${LIBVOS_BLD_D}/ConfigData.oo	\
			${LIBVOS_BLD_D}/Config.oo	\
			${LIBVOS_BLD_D}/File.oo		\
			${LIBVOS_BLD_D}/Buffer.oo	\
			${LIBVOS_BLD_D}/Error.oo

.PHONY: all clean

all:: ${BLD_D} ${TARGET}

${BLD_D}:
	@mkdir -p ${BLD_D}

${BLD_D}/rescached: ${RESCACHED_OBJS}
	@${do_build}

${BLD_D}/%.oo: ${SRC_D}/%.cpp ${SRC_D}/%.hpp
	@${do_compile}

clean::
	@rm -rf ${BLD_D}
