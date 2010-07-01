
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

# $Id: Makefile,v 1.15 2009/01/26 06:24:45 sfjro Exp $

all: aufs.5 aulchown etc_default_aufs
# auctl
	test -x ./mount.aufs || chmod a+x ./mount.aufs

aufs.5: aufs.in.5 c2tmac
	${RM} $@
	./c2tmac > $@
	awk '{ \
		gsub(/\140[^\047]*\047/, "\\[oq]&\\[cq]"); \
		gsub(/\\\[oq\]\140/, "\\[oq]"); \
		gsub(/\047\\\[cq\]/, "\\[cq]"); \
		gsub(/\047/, "\\[aq]"); \
		print; \
	}' aufs.in.5 >> $@
	chmod a-w $@

etc_default_aufs: c2sh aufs.shlib
	${RM} $@
	sed -e '/Id: /,$$d' aufs.shlib > $@
	echo '# aufs variables for shell scripts' >> $@
	./c2sh >> $@
	sed -e '0,/Id: /d' aufs.shlib >> $@
	chmod a-w $@

aulchown: LDFLAGS += -s
#aulchown: LDFLAGS += -static
CFLAGS += -O2 -Wall ${EXTRA_CFLAGS} ${ccflags-y}
%.o: %.c ../include/linux/aufs_type.h
.INTERMEDIATE: c2tmac.o c2tmac
.INTERMEDIATE: c2sh.o c2sh

clean:
	${RM} c2tmac c2sh aulchown aufs.5 etc_default_aufs *~ .*~
