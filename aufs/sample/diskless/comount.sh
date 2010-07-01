#!/bin/sh

# aufs sample -- diskless system

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

# $Id: comount.sh,v 1.10 2009/01/26 06:24:45 sfjro Exp $

set -ex
tmp=/tmp/$$
test "$1"
NfsRW="192.168.1.1:/home/jro/tmp/rw"
NfsRO="192.168.1.1:/ext2/diskless"
NfsOpts="tcp,intr"
Timeout=$((3600*24))
NfsActo="actimeo=${Timeout}"

########################################

debian() # sarge | etch
{
	case $HOSTNAME in
	www.domain|hostA|jrous)
		service=http;;
	mail.domain|hostB)
		service=smtp;;
	ftp.domain|hostC)
		service=ftp;;
	*)
		echo unknown host $HOSTNAME
		false
		;;
	esac

	portmap
	sleep 2
	for i in root common $service
	do mount -n -t nfs -o ro,${NfsOpts} ${NfsRO}/$1/$i /branch/$i
	done
	/bin/mount -n -t nfs -o rw,noatime,dirsync,${NfsOpts} ${NfsRW}/$HOSTNAME /branch/$HOSTNAME
	killall portmap
	sleep 2

	br="/branch/${HOSTNAME}=rw"
	br="${br}:/branch/${service}=ro+wh:/branch/common=ro+wh:/branch/root=ro+wh"
	mount -n -t aufs -o br:${br} aufsroot aufs
}

sarge()
{
	debian sarge
}

etch()
{
	debian etch
}

########################################

nfscd() # slax | knoppix | gentoo | edgy | faunos
{
	portmap
	sleep 2
	mount -n -t nfs -o ro,${NfsOpts},${NfsActo} ${NfsRO}/$1 /branch/$1
	killall portmap
	sleep 2

	for i in nls_base nls_iso8859-1 isofs loop
	do
		arg=""
		test $i = loop && arg="max_loop=32"
		f=/branch/$1/$i.ko
		test -f $f && insmod $f $arg
	done
	/bin/mount -n -t iso9660 -o ro,loop /branch/$1/cdrom /branch/cdrom

	mount -t tmpfs none /branch/$HOSTNAME
	opts="nowarn_perm,rdcache=${Timeout},udba=none,br:/branch/$HOSTNAME"
	#opts="nowarn_perm,udba=none,br:/branch/$HOSTNAME"
	d=/branch/$1/kmod
	test -d $d && opts="${opts}:$d"
	mount -n -t aufs -o ${opts} aufsroot aufs
}

########################################

slax()
{
	nfscd slax

	for i in unlzma sqlzma squashfs
	do
		f=/branch/slax/$i.ko
		test $f && insmod $f
	done

	lodev=1
	ls -1 /branch/cdrom/slax/base/* |
	while read i
	do
		dir=/branch/loop/$lodev
		lodev=$((${lodev}+1))
		/bin/mount -n -t squashfs -o ro,loop $i $dir
		/bin/mount -n -o remount,ins:1:$dir aufsroot /aufs
	done
	/bin/mount -n -o remount,warn_perm aufsroot /aufs

	cd /aufs
	for i in lktr lktr_exec
	do test -e /$i && cp -p /$i .
	done
	chroot . usr/sbin/useradd -m jro
	chroot . passwd -d jro
	echo 'S0:12345:respawn:/sbin/agetty -L ttyS0 115200 vt100' >> ./etc/inittab
	cat <<- EOF >> ./etc/securetty
	ttyS0
	tts/0
	EOF
	case $HOSTNAME in
	jrous)
		f=etc/X11/xorg.conf
		sed -e 's:/dev/mouse:/dev/input/mice:' $f > $tmp
		mv $tmp $f
		;;
	esac
	{
	#grep '^aufsroot' /proc/mounts | sed -e 's:/aufs:/:'
	echo aufsroot / aufs rw 0 0
	echo proc /proc proc defaults 0 0
	} > etc/fstab
	#echo 'kern.* /dev/console' >> ./etc/syslog.conf
	f=dev/console
	test -e $f || cp -p /$f $f
	#return

	for i in FireWall acpid alsa cups gpm pcmcia scanluns udev wireless
	do f=etc/rc.d/rc.$i
	test  -x $f && chmod a-x $f
	done

	{ cd / && tar -cf - dev; } | tar -xf -
	cat <<- EOF >> etc/rc.d/rc.local
	echo 8 > /proc/sys/kernel/printk
	{ cd /initrd && tar -cf - dev; } | tar -xmf -
	chmod a+w /dev/null
	seq 0 7 | while read i
	do f=/dev/tty\$i
	test -e \$f || mknod \$f c 4 \$i
	done
	mknod /dev/mem c 1 1
	mknod /dev/agpgart c 10 175
	mknod /dev/fb0 c 29 0
	mknod /dev/fb0current c 29 0
	mknod /dev/fb0autodetect c 29 1
	mknod /dev/full c 1 7
	mkdir /dev/input
	mknod /dev/input/mice c 13 63
	mknod /dev/input/mouce0 c 13 32
	mknod /dev/input/mouce1 c 13 33
	mknod /dev/input/mouce2 c 13 34
	mknod /dev/input/mouce3 c 13 35
	mknod /dev/kmem c 1 2
	mknod /dev/psaux c 10 1
	mknod /dev/random c 1 8
	mknod /dev/urandom c 1 9
	mknod /dev/zero c 1 5
	EOF
}

########################################

knoppix()
{
	nfscd knoppix

	insmod /branch/knoppix/cloop.ko file=/branch/cdrom/KNOPPIX/KNOPPIX
	mount -n -t iso9660 -o ro /dev/cloop0 /branch/loop/1
	/bin/mount -n -o remount,ins:1:/branch/loop/1 aufsroot /aufs

	cd /aufs
	{
	echo aufsroot / aufs rw 0 0
	echo none /dev/shm tmpfs defaults,noauto,size=1m 0 0
	} > etc/fstab

	if [ "$initrd" = "initramfs.gz" ]
	then
	sed -e 's/id:5:initdefault/id:2:initdefault/' ./etc/inittab > $tmp
	mv $tmp ./etc/inittab
	fi
	echo 'S0:12345:respawn:/bin/bash -login < /dev/ttyS0 > /dev/ttyS0 2>&1' >> ./etc/inittab

	case $HOSTNAME in
	jrous)
		f=./etc/rcS.d/S99nfscd
		cat <<- EOF > $f
		#!/bin/sh
		tmp=/tmp/\$\$
		f=/etc/X11/xorg.conf
		sed -e 's/^# USB Mouse not detected/InputDevice "USB Mouse" "CorePointer"/' \
			\$f > \$tmp
		mv \$tmp \$f
		EOF
		chmod +x $f
		;;
	esac
}

########################################

gentoo()
{
	nfscd gentoo

	for i in unlzma sqlzma squashfs
	do
		f=/branch/gentoo/$i.ko
		test $f && insmod $f
	done

	lodev=1
	dir=/branch/loop/$lodev
	/bin/mount -n -t squashfs -o ro,loop /branch/cdrom/image.squashfs $dir
	/bin/mount -n -o remount,ins:1:$dir aufsroot /aufs
	/bin/mount -n -o remount,warn_perm aufsroot /aufs

	cd /aufs
	echo 'config_eth0=( "noop" "192.168.1.5/24" )' >> ./etc/conf.d/net
	chroot . usr/sbin/useradd jro
	chroot . passwd -d jro
	cp -p /branch/gentoo/lndir .
	mkdir ./ext1
	cat <<- EOF >> ./etc/conf.d/local.start
	passwd -d root
	portmap
	mount -o ro 192.168.1.1:/ext1 /ext1
	mount -o ro /dev/hda12 /home
	rm -f /var/lib/init.d/started/net.eth0
	EOF
	echo 'S0:12345:respawn:/sbin/agetty ttyS0 115200 vt100' >> ./etc/inittab
}

########################################

edgy()
{
	nfscd edgy

	for i in unlzma sqlzma squashfs
	do
		f=/branch/edgy/$i.ko
		test $f && insmod $f
	done

	lodev=1
	dir=/branch/loop/$lodev
	/bin/mount -n -t squashfs -o ro,loop /branch/cdrom/casper/filesystem.squashfs $dir
	/bin/mount -n -o remount,ins:1:$dir aufsroot /aufs
	/bin/mount -n -o remount,warn_perm aufsroot /aufs

	cd /aufs
	chroot . usr/sbin/useradd -s /bin/bash jro
	chroot . passwd -d jro

	cp -p /branch/edgy/lndir .
	mkdir ./ext1
	{
	#grep '^aufsroot' /proc/mounts | sed -e 's:/aufs:/:'
	echo aufsroot / aufs rw 0 0
	echo tmpfs /tmp tmpfs nosuid,nodev 0 0
	echo 192.168.1.1:/ext1 /ext1 nfs ro,defaults 0 0
	echo /dev/hda12 /home ext2 ro,defaults 0 0
	} > ./etc/fstab

	chmod a-x ./usr/bin/consolechars
	for i in gdm cupsys hplip
	do echo ./etc/rc2.d/*$i
	done | xargs -r busybox chmod a-x
	sed -e 's/getty 38400 tty1/getty -L ttyS0 115200 vt100/' \
		./etc/event.d/tty1 > ./etc/event.d/ttyS0

	# mkdir ./samba
	# mount -t tmpfs none samba
	# cd samba
	# mkdir ro rw u
	# /bin/mount -n -t aufs -o br:rw:ro none u
	# cd ..
	# cat <<- EOF >> ./etc/samba/smb.conf
	# [aufs]
	# 	comment = aufs
	# 	path = /samba/u
	# 	guest ok = yes
	# 	writable = yes
	# EOF
}

########################################

faunos()
{
	nfscd faunos

	for i in unlzma sqlzma squashfs
	do
		f=/branch/faunos/$i.ko
		test $f && insmod $f
	done

	#HOOKS="base udev pata scsi sata usb splashy larch1 larch2 larch3"

	# hook/udev
	#/etc/start_udev

	# hook/usb

	# hook/splashy

	# hook/larch1
	mkdir /tfs
	mount -t tmpfs tmpfs /tfs
	mkdir -p /tfs/.livesys/livecd
	/bin/mount -n -o bind /branch/cdrom /tfs/.livesys/livecd

	# for i in run-init klibc-rOj3PRLKBA9FcF5ZuoqKQLmOWcA.so
	# do
	# 	tftp -g $SERVER -r `dirname $BOOTFILE`/faunos/$i \
	# 		-l /tfs/$i > /dev/null 2>&1
	# 	chmod a+x /tfs/$i
	# 	ls -l /tfs/$i
	# done

	# hook/larch2

	# hook/larch3
	for i in base etc
	do
		mkdir /tfs/.livesys/$i /tfs/.livesys/rw_$i /tfs/.livesys/ro_$i
		#/bin/losetup "/dev/loop0" "/tfs/.livesys/base.sqf"
		#/bin/mount -r -t squashfs "/dev/loop0" "/tfs/.livesys/base"
		/bin/mount -n -t squashfs -o ro,loop \
			/branch/cdrom/$i.sqf /tfs/.livesys/ro_$i
		br="/tfs/.livesys/rw_${i}:/tfs/.livesys/ro_$i"
		mount -n -t aufs -o br:${br} aufs_$i /tfs/.livesys/$i
	done

	f=/tfs/.livesys/etc/etc/rc.conf
	sed -e 's/ cups / /' \
		-e 's/ ipw3945d / /' \
		-e 's/ gpm laptop-mode splashy kdm//' \
		-e 's/KEYMAP="us"/KEYMAP="jp106"/' \
		-e 's/ crond//' \
		$f > t
	mv t $f
	cp -p /usr/bin/strace /lktr* /tfs
	cat <<- EOF > /tfs/l.sh
	#/bin/sh
	/strace -o /dev/ttyS0 login \$@
	EOF
	chmod a+x /tfs/l.sh
	f=/tfs/.livesys/etc/etc/inittab
#	sed -e 's:vc/2 linux:vc/2 linux -l /l.sh:' $f > t
#	echo 'cS:S12345:respawn:/sbin/agetty 115200 ttyS0 vt100' >> t
	sed -e 's:/sbin/sulogin -p:/sbin/agetty 115200 ttyS0 vt100:' $f > t
	mv t $f
	f=/tfs/.livesys/etc/etc/securetty
	echo 'ttyS0' >> $f
	echo 8 > /proc/sys/kernel/printk
	cat <<- EOF >> /tfs/.livesys/etc/etc/rc.local
	echo 8 > /proc/sys/kernel/printk
	EOF
	f=/tfs/.livesys/etc/etc/resolv.conf
	{ echo domain $DOMAIN; echo nameserver $DNSSRVS; } > $f
	cat <<- EOF > /tfs/a.sh
	#!/bin/sh
	echo 8 > /proc/sys/kernel/printk
	pacman -Sl | cut -f1 -d' ' | uniq -c
	yes n | /strace -etrace=!read,!select -o /tmp/s pacman -Syu > /dev/null
	echo
	#/lktr -p -1
	#pacman -Sl | cut -f1 -d' ' | uniq -c
	#/lktr -r
	cd /var/lib/pacman
	find current extra community -maxdepth 1 -type d | cut -f1 -d/ | uniq -c
	EOF

	cd /tfs/.livesys/base
	for i in *
	do
		mkdir /tfs/${i}
	        /bin/mount -n -o bind /tfs/.livesys/base/${i} /tfs/${i}
	done

	ls lib
	d=lib/modules/2.6.21.3jrousDL
	{ cd /aufs && tar -cf - $d; } | tar -xpf -
	#mkdir -p $d
	#/bin/mount -n --bind /aufs/$d $d

	mkdir /tfs/etc
	/bin/mount -n -o bind /tfs/.livesys/etc/etc /tfs/etc
	mkdir /tfs/dev
	mknod /tfs/dev/console c 5 1
	mknod /tfs/dev/null c 1 3
	mknod /tfs/dev/zero c 1 5
	for i in 0 1 2 3 4 5 6 7
	do mknod /tfs/dev/loop${i} b 7 ${i}
	done
	# LDEV
	#echo /dev/null > /tfs/.livesys/bootdevice
	echo aufs > /tfs/.livesys/utype

	# find /tfs -name passwd
	# chroot /tfs passwd -d root

	#ls -l /tfs/run-init /dev/console /tfs /tfs/sbin/init
}

########################################

$1
if [ ! "$dl_label" = "faunos" ]
then
	/bin/mount -n --bind /tmp /branch
fi
#/bin/sh -i
