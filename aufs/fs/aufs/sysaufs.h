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
 * sysfs interface
 *
 * $Id: sysaufs.h,v 1.21 2009/01/26 06:24:45 sfjro Exp $
 */

#ifndef __SYSAUFS_H__
#define __SYSAUFS_H__

#ifdef __KERNEL__

#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/version.h>

/* ---------------------------------------------------------------------- */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 22)
typedef struct kset au_subsys_t;
#define au_subsys_to_kset(subsys) (subsys)
#else
typedef struct subsystem au_subsys_t;
#define au_subsys_to_kset(subsys) ((subsys).kset)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
static inline struct kset *subsys_get(struct kset *s)
{
	return kset_get(s);
}

static inline void subsys_put(struct kset *s)
{
	kset_put(s);
}
#endif

/* ---------------------------------------------------------------------- */

/* entries under sysfs per super block */
enum {
	SysaufsSb_XINO,
#ifdef CONFIG_AUFS_EXPORT
	SysaufsSb_XIGEN,
#endif
	SysaufsSb_MNTPNT1,
#if 0
	SysaufsSb_PLINK,
	SysaufsSb_files,
#endif
	SysaufsSb_Last
};

struct sysaufs_sbinfo {
#ifdef CONFIG_AUFS_SYSAUFS
	//todo: try kset directly
	au_subsys_t		subsys;
	struct attribute	attr[SysaufsSb_Last];
	struct kref		ref;
	int			added;
	char 			compat_name[32];
#endif
};

/* 'brN' entry under sysfs per super block */
struct sysaufs_br {
#ifdef CONFIG_AUFS_SYSAUFS
	char			name[8];
	struct attribute	attr;
	struct kref		ref;
	int			added;
#endif
};

/* ---------------------------------------------------------------------- */

struct au_branch;
struct au_sbinfo;
#ifdef CONFIG_AUFS_SYSAUFS
extern struct mutex au_sbilist_mtx;
extern struct list_head au_sbilist;

static inline void au_sbilist_lock(void)
{
	mutex_lock(&au_sbilist_mtx);
}

static inline void au_sbilist_unlock(void)
{
	mutex_unlock(&au_sbilist_mtx);
}

static inline void au_sbilist_del(struct au_sbinfo *sbinfo)
{
	list_del(&sbinfo->si_list);
}

static inline void au_sbilist_add(struct au_sbinfo *sbinfo)
{
	/* the order in this list is important */
	list_add_tail(&sbinfo->si_list, &au_sbilist);
}

struct sysaufs_br *sysaufs_br_alloc(void);
void sysaufs_br_get(struct au_branch *br);
void sysaufs_br_put(struct au_branch *br);
void sysaufs_brs_add(struct super_block *sb);
void sysaufs_brs_del(struct super_block *sb);
int sysaufs_si_init(struct au_sbinfo *sbinfo);
void sysaufs_sbi_get(struct super_block *sb);
void sysaufs_sbi_put(struct super_block *sb);
void sysaufs_sbi_add(struct super_block *sb);
void sysaufs_sbi_del(struct super_block *sb);
int __init sysaufs_init(void);
void sysaufs_fin(void);

#else

#define au_sbilist_lock()	do {} while (0)
#define au_sbilist_unlock()	do {} while (0)

static inline void au_sbilist_del(struct au_sbinfo *sbinfo)
{
	/* empty */
}

static inline void au_sbilist_add(struct au_sbinfo *sbinfo)
{
	/* empty */
}

static inline struct sysaufs_br *sysaufs_br_alloc(void)
{
	return (void *)-1; //todo: poison
}

static inline void sysaufs_br_get(struct au_branch *br)
{
	/* nothing */
}

static inline void sysaufs_br_put(struct au_branch *br)
{
	/* nothing */
}

static inline void sysaufs_brs_add(struct super_block *sb)
{
	/* nothing */
}

static inline void sysaufs_brs_del(struct super_block *sb)
{
	/* nothing */
}

static inline int sysaufs_si_init(struct au_sbinfo *sbinfo)
{
	return 0;
}

static inline void sysaufs_sbi_get(struct super_block *sb)
{
	/* nothing */
}

static inline void sysaufs_sbi_put(struct super_block *sb)
{
	/* nothing */
}

static inline void sysaufs_sbi_add(struct super_block *sb)
{
	/* nothing */
}

static inline void sysaufs_sbi_del(struct super_block *sb)
{
	/* nothing */
}

static inline int sysaufs_init(void)
{
	sysaufs_brs = 0;
	return 0;
}

#define sysaufs_fin()			do {} while (0)

#endif /* CONFIG_AUFS_SYSAUFS */

#endif /* __KERNEL__ */
#endif /* __SYSAUFS_H__ */
