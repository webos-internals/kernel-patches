
# Copyright (C) 2005-2009 Junjiro Okajima
#
# This program, aufs is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

# $Id: local.mk,v 1.75 2009/01/26 06:24:45 sfjro Exp $

KDIR = /lib/modules/$(shell uname -r)/build
SUBLEVEL = $(shell ${MAKE} -s -C ${KDIR} kernelversion | cut -f3 -d. | cut -f1 -d-)
EXTRAVERSION = $(shell ${MAKE} -s -C ${KDIR} kernelversion | cut -f3 -d. | cut -f2- -d-)
Conf1=${KDIR}/include/config/auto.conf
Conf2=${KDIR}/.config
ifeq "t" "$(shell test -e ${Conf1} && echo t)"
include ${Conf1}
else ifeq "t" "$(shell test -e ${Conf2} && echo t)"
include ${Conf2}
else
$(warning could not find kernel config file. internal auto-config may fail)
endif

########################################
# default values. see ./Kconfig.in or ./fs/aufs/Kconfig after 'make kconfig'
# in detail.
# instead of setting 'n', leave it blank when you disable it.
export CONFIG_AUFS = m
export CONFIG_AUFS_BRANCH_MAX_127 = y
export CONFIG_AUFS_BRANCH_MAX_511 =
export CONFIG_AUFS_BRANCH_MAX_1023 =
#export CONFIG_AUFS_BRANCH_MAX_32767 =
export CONFIG_AUFS_EXPORT =
export CONFIG_AUFS_HINOTIFY =
export CONFIG_AUFS_ROBR =
export CONFIG_AUFS_SHWH =
export CONFIG_AUFS_DLGT =
#export CONFIG_AUFS_XATTR =
export CONFIG_AUFS_RR_SQUASHFS = y
export CONFIG_AUFS_SEC_PERM_PATCH =
export CONFIG_AUFS_SPLICE_PATCH =
export CONFIG_AUFS_PUT_FILP_PATCH =
export CONFIG_AUFS_LHASH_PATCH =
export CONFIG_AUFS_FSYNC_SUPER_PATCH =
export CONFIG_AUFS_DENY_WRITE_ACCESS_PATCH =
export CONFIG_AUFS_KSIZE_PATCH =
export CONFIG_AUFS_WORKAROUND_FUSE =
export CONFIG_AUFS_DEBUG = y
export CONFIG_AUFS_DEBUG_LOCK =
export CONFIG_AUFS_LOCAL = y
export CONFIG_AUFS_COMPAT =

# linux-2.6.24 and ealier
export CONFIG_AUFS_SYSAUFS = y

# linux-2.6.25 and later
export CONFIG_AUFS_STAT =

# unionfs patch
export CONFIG_AUFS_UNIONFS23_PATCH =
# 22 is for fs/aufs25 and later
export CONFIG_AUFS_UNIONFS22_PATCH =

AUFS_DEF_CONFIG =
-include priv_def.mk

########################################
# automatic configurations

export CONFIG_AUFS_INO_T_64 =
ifdef CONFIG_64BIT
CONFIG_AUFS_INO_T_64 = y
endif
ifdef CONFIG_ALPHA
CONFIG_AUFS_INO_T_64 =
else ifdef CONFIG_S390
CONFIG_AUFS_INO_T_64 =
endif

export CONFIG_AUFS_HIN_OR_DLGT =
ifdef CONFIG_AUFS_HINOTIFY
CONFIG_AUFS_HIN_OR_DLGT = y
else ifdef CONFIG_AUFS_DLGT
CONFIG_AUFS_HIN_OR_DLGT = y
endif

export CONFIG_AUFS_BR_NFS =
ifdef CONFIG_NFS_FS
CONFIG_AUFS_BR_NFS = y
ifeq "t" "$(shell test ${SUBLEVEL} -lt 16 && echo t)"
CONFIG_AUFS_BR_NFS =
else ifeq "t" "$(shell test ${SUBLEVEL} -ge 19 \
	-a x${CONFIG_AUFS_LHASH_PATCH} = x \
	&& echo t)"
CONFIG_AUFS_BR_NFS =
endif
endif

export CONFIG_AUFS_BR_NFS_V4 =
ifdef CONFIG_NFS_V4
ifdef CONFIG_AUFS_BR_NFS
CONFIG_AUFS_BR_NFS_V4 = y
ifeq "t" "$(shell test x${CONFIG_AUFS} = xm \
	-a x${CONFIG_AUFS_PUT_FILP_PATCH} = x \
	&& echo t)"
CONFIG_AUFS_BR_NFS_V4 =
endif
endif
endif

export CONFIG_AUFS_BR_XFS =
ifdef CONFIG_XFS_FS
ifeq "t" "$(shell test ${SUBLEVEL} -ge 24 && echo t)"
CONFIG_AUFS_BR_XFS = y
endif
endif

export CONFIG_AUFS_GETATTR =
ifdef CONFIG_AUFS_HINOTIFY
CONFIG_AUFS_GETATTR = y
else ifdef CONFIG_AUFS_WORKAROUND_FUSE
CONFIG_AUFS_GETATTR = y
else ifdef CONFIG_AUFS_BR_NFS
CONFIG_AUFS_GETATTR = y
endif

export CONFIG_AUFS_MAGIC_SYSRQ =
ifeq "t" "$(shell test ${SUBLEVEL} -ge 25 \
	-a x${CONFIG_AUFS_DEBUG} = xy \
	-a x${CONFIG_MAGIC_SYSRQ} = xy \
	&& echo t)"
CONFIG_AUFS_MAGIC_SYSRQ = y
else ifeq "t" "$(shell test ${SUBLEVEL} -ge 18 \
	-a x${CONFIG_AUFS_DEBUG} = xy \
	-a x${CONFIG_AUFS_SYSAUFS} = xy \
	-a x${CONFIG_MAGIC_SYSRQ} = xy \
	&& echo t)"
CONFIG_AUFS_MAGIC_SYSRQ = y
endif

ifeq "t" "$(shell test ${SUBLEVEL} -ge 25 && echo t)"
ifdef CONFIG_AUFS_UNIONFS23_PATCH
CONFIG_AUFS_UNIONFS22_PATCH = y
endif
endif

ifeq "t" "$(shell test ${SUBLEVEL} -ge 23 && echo t)"
ifdef CONFIG_AUFS_UNIONFS23_PATCH
CONFIG_AUFS_SPLICE_PATCH = y
endif
endif

########################################

define conf
ifdef $(1)
AUFS_DEF_CONFIG += -D$(1)
endif
endef

$(foreach i, BRANCH_MAX_127 BRANCH_MAX_511 BRANCH_MAX_1023 \
	BRANCH_MAX_32767 \
	STAT SYSAUFS HINOTIFY EXPORT INO_T_64 \
	ROBR DLGT HIN_OR_DLGT \
	SHWH RR_SQUASHFS \
	LHASH_PATCH SPLICE_PATCH BR_NFS BR_NFS_V4 \
	BR_XFS \
	WORKAROUND_FUSE GETATTR \
	DEBUG MAGIC_SYSRQ DEBUG_LOCK \
	LOCAL COMPAT UNIONFS22_PATCH UNIONFS23_PATCH, \
	$(eval $(call conf,CONFIG_AUFS_$(i))))

ifeq (${CONFIG_AUFS}, m)
AUFS_DEF_CONFIG += -DCONFIG_AUFS_MODULE -UCONFIG_AUFS
# $(foreach i, PUT_FILP_PATCH KSIZE_PATCH, \
# 	$(eval $(call conf,CONFIG_AUFS_$(i))))
ifdef CONFIG_AUFS_SEC_PERM_PATCH
AUFS_DEF_CONFIG += -DCONFIG_AUFS_SEC_PERM_PATCH
endif
ifdef CONFIG_AUFS_PUT_FILP_PATCH
AUFS_DEF_CONFIG += -DCONFIG_AUFS_PUT_FILP_PATCH
endif
ifdef CONFIG_AUFS_FSYNC_SUPER_PATCH
AUFS_DEF_CONFIG += -DCONFIG_AUFS_FSYNC_SUPER_PATCH
endif
ifdef CONFIG_AUFS_DENY_WRITE_ACCESS_PATCH
AUFS_DEF_CONFIG += -DCONFIG_AUFS_DENY_WRITE_ACCESS_PATCH
endif
ifdef CONFIG_AUFS_KSIZE_PATCH
AUFS_DEF_CONFIG += -DCONFIG_AUFS_KSIZE_PATCH
endif
else
AUFS_DEF_CONFIG += -UCONFIG_AUFS_MODULE -DCONFIG_AUFS
endif

EXTRA_CFLAGS += -I${CURDIR}/include
EXTRA_CFLAGS += ${AUFS_DEF_CONFIG}
EXTRA_CFLAGS += -DLKTRHidePrePath=\"${CURDIR}/${TgtPath}\"
#export

########################################
# fake top level make

TgtUtil = aufs.5 aufind.sh mount.aufs auplink aulchown umount.aufs etc_default_aufs auchk
# auctl
Tgt = aufs.ko ${TgtUtil}

# the environment variables are not inherited since 2.6.23
MAKE += CONFIG_AUFS=${CONFIG_AUFS} \
	AUFS_EXTRA_CFLAGS="$(shell echo ${EXTRA_CFLAGS} | sed -e 's/\"/\\\\\\"/g')"

TgtPath = fs/aufs
ifeq "t" "$(shell test ${SUBLEVEL} -ge 25 && echo t)"
TgtPath = fs/aufs25
endif

all: ${Tgt}
FORCE:

########################################

aufs.ko: ${TgtPath}/aufs.ko
	test ! -e $@ && ln -s $< $@ || :
${TgtPath}/aufs.ko: FORCE
	@echo ${TgtPath}
	${MAKE} -C ${KDIR} M=${CURDIR}/${TgtPath} modules

.SILENT: ${TgtPath}/Kconfig
${TgtPath}/Kconfig: mtime = $(shell ls -l --time-style=+%s Kconfig.in | cut -f6 -d' ')
${TgtPath}/Kconfig: ver_str = "These options are for"
${TgtPath}/Kconfig: Kconfig.in
	echo generating $@... if it fails, refer to README please.
	${CROSS_COMPILE}cpp -undef -nostdinc -P -I${KDIR}/include $< | \
		sed -e 's/"#"//' -e 's/^[[:space:]]*$$//' -e 's/^ /	/' | \
		uniq > $@
	touch -d @$$((${mtime} - 1)) $@
	echo checking $@...
	fgrep -q ${ver_str} $@
	fgrep ${ver_str} $@ | fgrep -vq UTS_RELEASE
	touch $@
	echo done.
kconfig: ${TgtPath}/Kconfig
	@echo copy ./${TgtPath} as fs/aufs and ./include to your linux kernel source tree.
	@echo add \'obj-\$$\(CONFIG_AUFS\) += aufs/\' to linux/fs/Makefile.
	@echo add \'source \"fs/aufs/Kconfig\"\' to linux/fs/Kconfig.
	@echo then, try \'make menuconfig\' and go to Filesystem menu.
	@echo when you upgrade your kernel source,
	@echo you need to re-run make $@ to re-generate ${TgtPath}/Kconfig.
	@echo also you will need to install some helper scripts,
	@echo /sbin/mount,aufs and others. refer to aufs README in detail.

########################################

clean:
	${MAKE} -C ${KDIR} M=${CURDIR}/${TgtPath} $@
	${MAKE} -C util $@
	${RM} ${Tgt}
	find . -type f \( -name '*~' -o -name '.#*[0-9]' \) | xargs -r ${RM}
#	-test -e ${TgtPath}/Kconfig && echo remove ${TgtPath}/Kconfig manually, if necessary.

export EXTRA_CFLAGS
build_util:
	${MAKE} -C util
${TgtUtil}: build_util
	-test -e $@ || ln -s util/$@ $@

-include priv.mk
