/*
 * Copyright (C) 2005-2009 Junjiro Okajima
 *
 * This program, aufs is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 * main header files
 *
 * $Id: aufs.h,v 1.51 2009/01/26 06:24:45 sfjro Exp $
 */

#ifndef __AUFS_H__
#define __AUFS_H__

#ifdef __KERNEL__

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18)
#include <linux/uaccess.h>
#else
#include <asm/uaccess.h>
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 24)
#error you got wrong version
#endif

/* ---------------------------------------------------------------------- */

/* limited support before 2.6.16, curretly 2.6.15 only. */
#include <linux/time.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16)
#define timespec_to_ns(ts)	({ (long long)(ts)->tv_sec; })
#define D_CHILD			d_child
#else
#define D_CHILD			d_u.d_child
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 17)
#include <linux/types.h>
typedef unsigned long blkcnt_t;
#endif

/* introduced linux-2.6.17 */
#include <linux/fs.h>
#ifndef FMODE_EXEC
#define FMODE_EXEC 0
#endif

/* introduced in linux-2.6.21 */
#include <linux/compiler.h>
#ifndef __packed
#define __packed	__attribute__((packed))
#endif
#ifndef __aligned
#define __aligned(x)	__attribute__((aligned(x)))
#endif

/* introduced in linux-2.6.25 */
#ifndef noinline_for_stack
#define noinline_for_stack	/* */
#endif

/* introduced in linux-2.6.27 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
#define WARN_ONCE(cond, fmt ...) WARN_ON(cond)
#endif

/* ---------------------------------------------------------------------- */

#include "debug.h"

#include "branch.h"
#include "cpup.h"
#include "dcsub.h"
#include "dentry.h"
#include "dir.h"
#include "file.h"
#include "hinode.h"
#include "inode.h"
#include "misc.h"
#include "module.h"
#include "opts.h"
#include "super.h"
#include "sysaufs.h"
#include "vfsub.h"
#include "whout.h"
#include "wkq.h"
/* reserved for future use */
/* #include "xattr.h" */

/* ---------------------------------------------------------------------- */

#ifdef CONFIG_AUFS_MODULE

/* call ksize() or not */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22) \
	&& !defined(CONFIG_AUFS_KSIZE_PATCH)
#define ksize(p)	(0U)
#endif

#endif /* CONFIG_AUFS_MODULE */

#endif /* __KERNEL__ */
#endif /* __AUFS_H__ */
