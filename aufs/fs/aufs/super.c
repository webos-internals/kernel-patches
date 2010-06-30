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
 * mount and super_block operations
 *
 * $Id: super.c,v 1.103 2009/01/26 06:24:10 sfjro Exp $
 */

#include <linux/module.h>
#include <linux/buffer_head.h>
#include <linux/seq_file.h>
#include <linux/smp_lock.h>
#include <linux/statfs.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
#include <linux/mnt_namespace.h>
typedef struct mnt_namespace au_mnt_ns_t;
#define au_nsproxy(tsk)	(tsk)->nsproxy
#define au_mnt_ns(tsk)	(tsk)->nsproxy->mnt_ns
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
#include <linux/namespace.h>
typedef struct namespace au_mnt_ns_t;
#define au_nsproxy(tsk)	(tsk)->nsproxy
#define au_mnt_ns(tsk)	(tsk)->nsproxy->namespace
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18)
#include <linux/namespace.h>
typedef struct namespace au_mnt_ns_t;
#define au_nsproxy(tsk)	(tsk)->namespace
#define au_mnt_ns(tsk)	(tsk)->namespace
#endif

#include "aufs.h"

/*
 * super_operations
 */
static struct inode *aufs_alloc_inode(struct super_block *sb)
{
	struct aufs_icntnr *c;

	AuTraceEnter();

	c = au_cache_alloc_icntnr();
	if (c) {
		inode_init_once(&c->vfs_inode);
		c->vfs_inode.i_version = 1; /* sigen(sb); */
		c->iinfo.ii_hinode = NULL;
		return &c->vfs_inode;
	}
	return NULL;
}

static void aufs_destroy_inode(struct inode *inode)
{
	int err;

	LKTRTrace("i%lu\n", inode->i_ino);

	if (!inode->i_nlink || IS_DEADDIR(inode)) {
		/* in nowait task or remount, sbi is write-locked */
		struct super_block *sb = inode->i_sb;
		const int locked = si_noflush_read_trylock(sb);

		err = au_xigen_inc(inode);
		if (unlikely(err))
			AuWarn1("failed resetting i_generation, %d\n", err);
		if (locked)
			si_read_unlock(sb);
	}

	au_iinfo_fin(inode);
	au_cache_free_icntnr(container_of(inode, struct aufs_icntnr,
					  vfs_inode));
}

static void aufs_read_inode(struct inode *inode)
{
	int err;
#if 0
	static struct backing_dev_info bdi = {
		.ra_pages	= 0,	/* No readahead */
		.capabilities	= BDI_CAP_NO_ACCT_DIRTY | BDI_CAP_NO_WRITEBACK
	};
#endif

	LKTRTrace("i%lu\n", inode->i_ino);

	err = au_xigen_new(inode);
	if (!err)
		err = au_iinfo_init(inode);
	//if (LktrCond) err = -1;
	if (!err) {
		inode->i_version++;
		//inode->i_mapping->backing_dev_info = &bdi;
		return; /* success */
	}

	LKTRTrace("intializing inode info failed(%d)\n", err);
	make_bad_inode(inode);
}

/* lock free root dinfo */
static int au_show_brs(struct seq_file *seq, struct super_block *sb)
{
	int err;
	aufs_bindex_t bindex, bend;
	struct au_hdentry *hd;

	AuTraceEnter();

	err = 0;
	bend = au_sbend(sb);
	hd = au_di(sb->s_root)->di_hdentry;
	for (bindex = 0; !err && bindex <= bend; bindex++) {
		err = seq_path(seq, au_sbr_mnt(sb, bindex),
			       hd[bindex].hd_dentry, au_esc_chars);
		if (err > 0) {
			const char *p = au_optstr_br_perm(au_sbr_perm(sb,
								      bindex));
			err = seq_printf(seq, "=%s", p);
		}
		if (!err && bindex != bend)
			err = seq_putc(seq, ':');
	}

	AuTraceErr(err);
	return err;
}

static void au_show_wbr_create(struct seq_file *m, int v,
			       struct au_sbinfo *sbinfo)
{
	au_parser_pattern_t pat;

	AuDebugOn(v == AuWbrCreate_Def);

	seq_printf(m, ",create=");
	pat = au_optstr_wbr_create(v);
	switch (v) {
	case AuWbrCreate_TDP:
	case AuWbrCreate_RR:
	case AuWbrCreate_MFS:
	case AuWbrCreate_PMFS:
		seq_printf(m, pat);
		break;
	case AuWbrCreate_MFSV:
		seq_printf(m, /*pat*/"mfs:%lu",
			   sbinfo->si_wbr_mfs.mfs_expire / HZ);
		break;
	case AuWbrCreate_PMFSV:
		seq_printf(m, /*pat*/"pmfs:%lu",
			   sbinfo->si_wbr_mfs.mfs_expire / HZ);
		break;
	case AuWbrCreate_MFSRR:
		seq_printf(m, /*pat*/"mfsrr:%llu",
			   sbinfo->si_wbr_mfs.mfsrr_watermark);
		break;
	case AuWbrCreate_MFSRRV:
		seq_printf(m, /*pat*/"mfsrr:%llu:%lu",
			   sbinfo->si_wbr_mfs.mfsrr_watermark,
			   sbinfo->si_wbr_mfs.mfs_expire / HZ);
		break;
	}
}

/* seq_file will re-call me in case of too long string */
static int aufs_show_options(struct seq_file *m, struct vfsmount *mnt)
{
	int err, n;
	unsigned int mnt_flags, v;
	struct super_block *sb;
	struct au_sbinfo *sbinfo;
	struct file *xino;

	AuTraceEnter();

	sb = mnt->mnt_sb;
	/* lock free root dinfo */
	si_noflush_read_lock(sb);
	sbinfo = au_sbi(sb);

	/* compatibility for some aufs scripts */
	seq_printf(m, ",si=%p", sbinfo);
	mnt_flags = au_mntflags(sb);
	if (au_opt_test(mnt_flags, XINO)) {
		seq_puts(m, ",xino=");
		xino = sbinfo->si_xib;
		err = seq_path(m, xino->f_vfsmnt, xino->f_dentry, au_esc_chars);
		if (unlikely(err <= 0))
			goto out;
		err = 0;
#define Deleted "\\040(deleted)"
		m->count -= sizeof(Deleted) - 1;
		AuDebugOn(memcmp(m->buf + m->count, Deleted,
				 sizeof(Deleted) - 1));
#undef Deleted
#if 0 //def CONFIG_AUFS_EXPORT /* reserved for future use */
	} else if (au_opt_test(mnt_flags, XINODIR)) {
		seq_puts(m, ",xinodir=");
		seq_path(m, &sbinfo->si_xinodir, au_esc_chars);
#endif
	} else
		seq_puts(m, ",noxino");

#define AuBool(name, str) do { \
	v = au_opt_test(mnt_flags, name); \
	if (v != au_opt_test(AuOpt_Def, name)) \
		seq_printf(m, ",%s" #str, v ? "" : "no"); \
} while (0)

#define AuStr(name, str) do { \
	v = mnt_flags & AuOptMask_##name; \
	if (v != (AuOpt_Def & AuOptMask_##name)) \
		seq_printf(m, "," #str "=%s", au_optstr_##str(v)); \
} while (0)

#ifdef CONFIG_AUFS_COMPAT
#define AuStr_BrOpt	"dirs="
#else
#define AuStr_BrOpt	"br:"
#endif

	AuBool(TRUNC_XINO, trunc_xino);
	AuBool(DIRPERM1, dirperm1);
	AuBool(SHWH, shwh);
	AuBool(PLINK, plink);
	AuStr(UDBA, udba);

	v = sbinfo->si_wbr_create;
	if (v != AuWbrCreate_Def)
		au_show_wbr_create(m, v, sbinfo);

	v = sbinfo->si_wbr_copyup;
	if (v != AuWbrCopyup_Def)
		seq_printf(m, ",cpup=%s", au_optstr_wbr_copyup(v));

	v = au_opt_test(mnt_flags, ALWAYS_DIROPQ);
	if (v != au_opt_test(AuOpt_Def, ALWAYS_DIROPQ))
		seq_printf(m, ",diropq=%c", v ? 'a' : 'w');
	AuBool(REFROF, refrof);
	AuBool(DLGT, dlgt);
	AuBool(WARN_PERM, warn_perm);
	AuBool(VERBOSE, verbose);
	AuBool(SUM, sum);
	/* AuBool(SUM_W, wsum); */

	n = sbinfo->si_dirwh;
	if (n != AUFS_DIRWH_DEF)
		seq_printf(m, ",dirwh=%d", n);
	n = sbinfo->si_rdcache / HZ;
	if (n != AUFS_RDCACHE_DEF)
		seq_printf(m, ",rdcache=%d", n);

	AuStr(COO, coo);

	if (!sysaufs_brs) {
		seq_puts(m, "," AuStr_BrOpt);
		au_show_brs(m, sb);
	}

 out:
	si_read_unlock(sb);
	return 0;

#undef AuBool
#undef AuStr
#undef AuStr_BrOpt
}

#ifndef ULLONG_MAX /* before linux-2.6.18 */
#define ULLONG_MAX	(~0ULL)
#endif

static u64 au_add_till_max(u64 a, u64 b)
{
	u64 old;

	old = a;
	a += b;
	if (old < a)
		return a;
	return ULLONG_MAX;
}

static int au_statfs_sum(struct super_block *sb, struct kstatfs *buf, int dlgt)
{
	int err;
	aufs_bindex_t bend, bindex, i;
	unsigned char shared;
	u64 blocks, bfree, bavail, files, ffree;
	struct super_block *h_sb;

	AuTraceEnter();

	blocks = 0;
	bfree = 0;
	bavail = 0;
	files = 0;
	ffree = 0;

	err = 0;
	bend = au_sbend(sb);
	for (bindex = bend; bindex >= 0; bindex--) {
		h_sb = au_sbr_sb(sb, bindex);
		shared = 0;
		for (i = bindex + 1; !shared && i <= bend; i++)
			shared = au_sbr_sb(sb, i) == h_sb;
		if (shared)
			continue;

		err = vfsub_statfs(h_sb->s_root, buf, dlgt);
		if (unlikely(err))
			goto out;

		blocks = au_add_till_max(blocks, buf->f_blocks);
		bfree = au_add_till_max(bfree, buf->f_bfree);
		bavail = au_add_till_max(bavail, buf->f_bavail);
		files = au_add_till_max(files, buf->f_files);
		ffree = au_add_till_max(ffree, buf->f_ffree);
	}

	buf->f_blocks = blocks;
	buf->f_bfree = bfree;
	buf->f_bavail = bavail;
	buf->f_files = files;
	buf->f_ffree = ffree;

 out:
	AuTraceErr(err);
	return err;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18)
#define StatfsArg(sb)	au_sbr_sb(sb, 0)->s_root
#define StatfsSb(d)	((d)->d_sb)
static int aufs_statfs(struct dentry *arg, struct kstatfs *buf)
#else
#define StatfsArg(sb)	au_sbr_sb(sb, 0)
#define StatfsSb(s)	(s)
static int aufs_statfs(struct super_block *arg, struct kstatfs *buf)
#endif
{
	int err;
	unsigned int mnt_flags;
	unsigned char dlgt;
	struct super_block *sb = StatfsSb(arg);

	AuTraceEnter();

	/* lock free root dinfo */
	si_noflush_read_lock(sb);
	mnt_flags = au_mntflags(sb);
	dlgt = !!au_test_dlgt(mnt_flags);
	if (!au_opt_test(mnt_flags, SUM))
		err = vfsub_statfs(StatfsArg(sb), buf, dlgt);
	else
		err = au_statfs_sum(sb, buf, dlgt);
	//if (LktrCond) err = -1;
	si_read_unlock(sb);
	if (!err) {
		buf->f_type = AUFS_SUPER_MAGIC;
		buf->f_namelen -= AUFS_WH_PFX_LEN;
		//todo: support uuid?
		memset(&buf->f_fsid, 0, sizeof(buf->f_fsid));
	}
	/* buf->f_bsize = buf->f_blocks = buf->f_bfree = buf->f_bavail = -1; */

	AuTraceErr(err);
	return err;
}

static void au_update_mnt(struct vfsmount *mnt, int flags)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18)
	struct vfsmount *pos;
	struct super_block *sb = mnt->mnt_sb;
	struct dentry *root = sb->s_root;
	struct au_sbinfo *sbinfo = au_sbi(sb);
	au_mnt_ns_t *ns;

	AuTraceEnter();
	AuDebugOn(!kernel_locked());

	if (sbinfo->si_mnt != mnt
	    || atomic_read(&sb->s_active) == 1
	    || !au_nsproxy(current))
		return;

	/* no get/put */
	ns = au_mnt_ns(current);
	AuDebugOn(!ns);
	sbinfo->si_mnt = NULL;
	list_for_each_entry(pos, &ns->list, mnt_list)
		if (pos != mnt && pos->mnt_sb->s_root == root) {
			sbinfo->si_mnt = pos;
			break;
		}
	AuDebugOn(!(flags & MNT_DETACH) && !sbinfo->si_mnt);
#endif
}

static void au_fsync_br(struct super_block *sb)
{
#ifdef CONFIG_AUFS_FSYNC_SUPER_PATCH
	aufs_bindex_t bend, bindex;
	int brperm;
	struct au_branch *br;
	struct super_block *h_sb;

	AuTraceEnter();

	si_write_lock(sb);
	bend = au_sbend(sb);
	for (bindex = 0; bindex < bend; bindex++) {
		br = au_sbr(sb, bindex);
		brperm = br->br_perm;
		if (brperm == AuBrPerm_RR || brperm == AuBrPerm_RRWH)
			continue;
		h_sb = br->br_mnt->mnt_sb;
		if (bdev_read_only(h_sb->s_bdev))
			continue;

		lockdep_off();
		down_write(&h_sb->s_umount);
		shrink_dcache_sb(h_sb);
		fsync_super(h_sb);
		up_write(&h_sb->s_umount);
		lockdep_on();
	}
	si_write_unlock(sb);
#endif
}

#define UmountBeginHasMnt	(LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))

#if UmountBeginHasMnt
#define UmountBeginSb(mnt)	((mnt)->mnt_sb)
#define UmountBeginMnt(mnt)	(mnt)
#define UmountBeginFlags	_flags
static void aufs_umount_begin(struct vfsmount *arg, int _flags)
#else
#define UmountBeginSb(sb)	sb
#define UmountBeginMnt(sb)	NULL
#define UmountBeginFlags	0
static void aufs_umount_begin(struct super_block *arg)
#endif
{
	struct super_block *sb = UmountBeginSb(arg);
	struct vfsmount *mnt = UmountBeginMnt(arg);
	struct au_sbinfo *sbinfo;

	AuTraceEnter();

	sbinfo = au_sbi(sb);
	if (!sbinfo)
		return;

	au_fsync_br(sb);

	si_write_lock(sb);
	if (au_opt_test(au_mntflags(sb), PLINK))
		au_plink_put(sb);
#if 0 // remove
	if (au_opt_test(au_mntflags(sb), UDBA_INOTIFY))
		shrink_dcache_sb(sb);
#endif
	au_update_mnt(mnt, UmountBeginFlags);
#if 0
	if (sbinfo->si_wbr_create_ops->fin)
		sbinfo->si_wbr_create_ops->fin(sb);
#endif
	si_write_unlock(sb);
}

/* final actions when unmounting a file system */
static void aufs_put_super(struct super_block *sb)
{
	struct au_sbinfo *sbinfo;

	AuTraceEnter();

	sbinfo = au_sbi(sb);
	if (!sbinfo)
		return;

#if !UmountBeginHasMnt
	/* umount_begin() may not be called. */
	aufs_umount_begin(sb);
#endif
	au_si_free(sb, /*err*/0);
}

/* ---------------------------------------------------------------------- */

/*
 * refresh dentry and inode at remount time.
 */
static int do_refresh(struct dentry *dentry, mode_t type,
		      unsigned int dir_flags)
{
	int err;
	struct dentry *parent;
	struct inode *inode;

	LKTRTrace("%.*s, 0%o\n", AuDLNPair(dentry), type);
	inode = dentry->d_inode;
	AuDebugOn(!inode);

	di_write_lock_child(dentry);
	parent = dget_parent(dentry);
	di_read_lock_parent(parent, AuLock_IR);
	/* returns a number of positive dentries */
	err = au_refresh_hdentry(dentry, type);
	if (err >= 0) {
		err = au_refresh_hinode(inode, dentry);
		if (!err && type == S_IFDIR)
			au_reset_hinotify(inode, dir_flags);
	}
	if (unlikely(err))
		AuErr("unrecoverable error %d, %.*s\n", err, AuDLNPair(dentry));
	di_read_unlock(parent, AuLock_IR);
	dput(parent);
	di_write_unlock(dentry);

	AuTraceErr(err);
	return err;
}

static int test_dir(struct dentry *dentry, void *arg)
{
	return S_ISDIR(dentry->d_inode->i_mode);
}

/* todo: merge with refresh_nondir()? */
static int refresh_dir(struct dentry *root, au_gen_t sgen)
{
	int err, i, j, ndentry, e;
	struct au_dcsub_pages dpages;
	struct au_dpage *dpage;
	struct dentry **dentries;
	struct inode *inode;
	const unsigned int flags = au_hi_flags(root->d_inode, /*isdir*/1);

	LKTRTrace("sgen %d\n", sgen);
	SiMustWriteLock(root->d_sb);
	/* dont trust BKL */
	AuDebugOn(au_digen(root) != sgen || !kernel_locked());

	err = 0;
	list_for_each_entry(inode, &root->d_sb->s_inodes, i_sb_list)
		if (S_ISDIR(inode->i_mode) && au_iigen(inode) != sgen) {
			ii_write_lock_child(inode);
			e = au_refresh_hinode_self(inode);
			ii_write_unlock(inode);
			if (unlikely(e)) {
				LKTRTrace("e %d, i%lu\n", e, inode->i_ino);
				if (!err)
					err = e;
				/* go on even if err */
			}
		}

	e = au_dpages_init(&dpages, GFP_NOFS);
	if (unlikely(e)) {
		if (!err)
			err = e;
		goto out;
	}
	e = au_dcsub_pages(&dpages, root, test_dir, NULL);
	if (unlikely(e)) {
		if (!err)
			err = e;
		goto out_dpages;
	}

	for (i = 0; !e && i < dpages.ndpage; i++) {
		dpage = dpages.dpages + i;
		dentries = dpage->dentries;
		ndentry = dpage->ndentry;
		for (j = 0; !e && j < ndentry; j++) {
			struct dentry *d;
			d = dentries[j];
#ifdef CONFIG_AUFS_DEBUG
			{
				struct dentry *parent;
				parent = dget_parent(d);
				AuDebugOn(!S_ISDIR(d->d_inode->i_mode)
					  || IS_ROOT(d)
					  || au_digen(parent) != sgen);
				dput(parent);
			}
#endif
			if (au_digen(d) != sgen) {
				e = do_refresh(d, S_IFDIR, flags);
				if (unlikely(e && !err))
					err = e;
				/* break on err */
			}
		}
	}

 out_dpages:
	au_dpages_free(&dpages);
 out:
	AuTraceErr(err);
	return err;
}

static int test_nondir(struct dentry *dentry, void *arg)
{
	return !S_ISDIR(dentry->d_inode->i_mode);
}

static int refresh_nondir(struct dentry *root, au_gen_t sgen, int do_dentry)
{
	int err, i, j, ndentry, e;
	struct au_dcsub_pages dpages;
	struct au_dpage *dpage;
	struct dentry **dentries;
	struct inode *inode;

	LKTRTrace("sgen %d\n", sgen);
	SiMustWriteLock(root->d_sb);
	/* dont trust BKL */
	AuDebugOn(au_digen(root) != sgen || !kernel_locked());

	err = 0;
	list_for_each_entry(inode, &root->d_sb->s_inodes, i_sb_list)
		if (!S_ISDIR(inode->i_mode) && au_iigen(inode) != sgen) {
			ii_write_lock_child(inode);
			e = au_refresh_hinode_self(inode);
			ii_write_unlock(inode);
			if (unlikely(e)) {
				LKTRTrace("e %d, i%lu\n", e, inode->i_ino);
				if (!err)
					err = e;
				/* go on even if err */
			}
		}

	if (!do_dentry)
		goto out;

	e = au_dpages_init(&dpages, GFP_NOFS);
	if (unlikely(e)) {
		if (!err)
			err = e;
		goto out;
	}
	e = au_dcsub_pages(&dpages, root, test_nondir, NULL);
	if (unlikely(e)) {
		if (!err)
			err = e;
		goto out_dpages;
	}

	for (i = 0; i < dpages.ndpage; i++) {
		dpage = dpages.dpages + i;
		dentries = dpage->dentries;
		ndentry = dpage->ndentry;
		for (j = 0; j < ndentry; j++) {
			struct dentry *d;
			d = dentries[j];
#ifdef CONFIG_AUFS_DEBUG
			{
				struct dentry *parent;
				parent = dget_parent(d);
				AuDebugOn(S_ISDIR(d->d_inode->i_mode)
					  || au_digen(parent) != sgen);
				dput(parent);
			}
#endif
			inode = d->d_inode;
			if (inode && au_digen(d) != sgen) {
				e = do_refresh(d, inode->i_mode & S_IFMT, 0);
				if (unlikely(e && !err))
					err = e;
				/* go on even err */
			}
		}
	}

 out_dpages:
	au_dpages_free(&dpages);
 out:
	AuTraceErr(err);
	return err;
}

/* stop extra interpretation of errno in mount(8), and strange error messages */
static int cvt_err(int err)
{
	AuTraceErr(err);

	switch (err) {
	case -ENOENT:
	case -ENOTDIR:
	case -EEXIST:
	case -EIO:
		err = -EINVAL;
	}
	return err;
}

/* protected by s_umount */
static int aufs_remount_fs(struct super_block *sb, int *flags, char *data)
{
	int err, rerr;
	au_gen_t sigen;
	struct dentry *root;
	struct inode *inode;
	struct au_opts opts;
	struct au_sbinfo *sbinfo;
	unsigned char dlgt;

	LKTRTrace("flags 0x%x, data %s, len %lu\n",
		  *flags, data ? data : "NULL",
		  (unsigned long)(data ? strlen(data) : 0));

	au_fsync_br(sb);

	err = 0;
	root = sb->s_root;
	if (!data || !*data) {
		aufs_write_lock(root);
		err = verify_opts(sb, *flags, /*pending*/0, /*remount*/1);
		aufs_write_unlock(root);
		goto out; /* success */
	}

	err = -ENOMEM;
	memset(&opts, 0, sizeof(opts));
	opts.opt = (void *)__get_free_page(GFP_NOFS);
	if (unlikely(!opts.opt))
		goto out;
	opts.max_opt = PAGE_SIZE / sizeof(*opts.opt);
	opts.flags = AuOpts_REMOUNT;
	opts.sb_flags = *flags;

	/* parse it before aufs lock */
	err = au_opts_parse(sb, data, &opts);
	if (unlikely(err))
		goto out_opts;

	sbinfo = au_sbi(sb);
	inode = root->d_inode;
	vfsub_i_lock(inode);
	aufs_write_lock(root);

	/* au_do_opts() may return an error */
	err = au_opts_remount(sb, &opts);
	au_opts_free(&opts);

	if (au_ftest_opts(opts.flags, REFRESH_DIR)
	    || au_ftest_opts(opts.flags, REFRESH_NONDIR)) {
		dlgt = !!au_opt_test(sbinfo->si_mntflags, DLGT);
		au_opt_clr(sbinfo->si_mntflags, DLGT);
		au_sigen_inc(sb);
		au_reset_hinotify(inode, au_hi_flags(inode, /*isdir*/1));
		sigen = au_sigen(sb);
		au_fclr_si(sbinfo, FAILED_REFRESH_DIRS);

		DiMustNoWaiters(root);
		IiMustNoWaiters(root->d_inode);
		di_write_unlock(root);

		rerr = refresh_dir(root, sigen);
		if (unlikely(rerr)) {
			au_fset_si(sbinfo, FAILED_REFRESH_DIRS);
			AuWarn("Refreshing directories failed, ignores (%d)\n",
			       rerr);
		}

		if (au_ftest_opts(opts.flags, REFRESH_NONDIR)) {
			rerr = refresh_nondir(root, sigen, !rerr);
			if (unlikely(rerr))
				AuWarn("Refreshing non-directories failed,"
				       " ignores (%d)\n", rerr);
		}

		/* aufs_write_lock() calls ..._child() */
		di_write_lock_child(root);

		au_cpup_attr_all(inode, /*force*/1);
		if (dlgt)
			au_opt_set(sbinfo->si_mntflags, DLGT);
	}

	aufs_write_unlock(root);
	vfsub_i_unlock(inode);

 out_opts:
	free_page((unsigned long)opts.opt);
 out:
	err = cvt_err(err);
	AuTraceErr(err);
	return err;
}

static struct super_operations aufs_sop = {
	.alloc_inode	= aufs_alloc_inode,
	.destroy_inode	= aufs_destroy_inode,
	.read_inode	= aufs_read_inode,
	//.dirty_inode	= aufs_dirty_inode,
	//.write_inode	= aufs_write_inode,
	//void (*put_inode) (struct inode *);
	.drop_inode	= generic_delete_inode,
	//.delete_inode	= aufs_delete_inode,
	//.clear_inode	= aufs_clear_inode,

	.show_options	= aufs_show_options,
	.statfs		= aufs_statfs,

	.put_super	= aufs_put_super,
	//void (*write_super) (struct super_block *);
	//int (*sync_fs)(struct super_block *sb, int wait);
	//void (*write_super_lockfs) (struct super_block *);
	//void (*unlockfs) (struct super_block *);
	.remount_fs	= aufs_remount_fs,
	/* depends upon umount flags. also use put_super() (< 2.6.18) */
	.umount_begin	= aufs_umount_begin
};

/* ---------------------------------------------------------------------- */

/*
 * at first mount time.
 */

static int alloc_root(struct super_block *sb)
{
	int err;
	struct inode *inode;
	struct dentry *root;

	AuTraceEnter();

	err = -ENOMEM;
	inode = iget(sb, AUFS_ROOT_INO);
	//if (LktrCond) {iput(inode); inode = NULL;}
	if (unlikely(!inode))
		goto out;
	err = PTR_ERR(inode);
	if (IS_ERR(inode))
		goto out;
	err = -ENOMEM;
	if (unlikely(is_bad_inode(inode)))
		goto out_iput;

	inode->i_op = &aufs_dir_iop;
	inode->i_fop = &aufs_dir_fop;
	inode->i_mode = S_IFDIR;
	root = d_alloc_root(inode);
	//if (LktrCond) {igrab(inode); dput(root); root = NULL;}
	if (unlikely(!root))
		goto out_iput;
	err = PTR_ERR(root);
	if (IS_ERR(root))
		goto out_iput;

	err = au_alloc_dinfo(root);
	if (!err) {
		sb->s_root = root;
		return 0; /* success */
	}
	dput(root);
	goto out; /* do not iput */

 out_iput:
	iput(inode);
 out:
	AuTraceErr(err);
	return err;

}

static int aufs_fill_super(struct super_block *sb, void *raw_data, int silent)
{
	int err;
	struct dentry *root;
	struct inode *inode;
	struct au_opts opts;
	char *arg = raw_data;

	if (unlikely(!arg || !*arg)) {
		err = -EINVAL;
		AuErr("no arg\n");
		goto out;
	}
	LKTRTrace("%s, silent %d\n", arg, silent);

	err = -ENOMEM;
	memset(&opts, 0, sizeof(opts));
	opts.opt = (void *)__get_free_page(GFP_NOFS);
	if (unlikely(!opts.opt))
		goto out;
	opts.max_opt = PAGE_SIZE / sizeof(*opts.opt);
	opts.sb_flags = sb->s_flags;

	err = au_si_alloc(sb);
	if (unlikely(err))
		goto out_opts;
	SiMustWriteLock(sb);

	/* all timestamps always follow the ones on the branch */
	sb->s_flags |= MS_NOATIME | MS_NODIRATIME;
	sb->s_op = &aufs_sop;
	sb->s_magic = AUFS_SUPER_MAGIC;
	au_export_init(sb);

	err = alloc_root(sb);
	if (unlikely(err)) {
		AuDebugOn(sb->s_root);
		si_write_unlock(sb);
		goto out_info;
	}
	root = sb->s_root;
	DiMustWriteLock(root);
	inode = root->d_inode;
	inode->i_nlink = 2;

	/*
	 * actually we can parse options regardless aufs lock here.
	 * but at remount time, parsing must be done before aufs lock.
	 * so we follow the same rule.
	 */
	ii_write_lock_parent(inode);
	au_dbg_locked_si_reg(sb, 1);
	aufs_write_unlock(root);
	err = au_opts_parse(sb, arg, &opts);
	if (unlikely(err))
		goto out_root;

	/* lock vfs_inode first, then aufs. */
	vfsub_i_lock(inode);
	inode->i_op = &aufs_dir_iop;
	inode->i_fop = &aufs_dir_fop;
	aufs_write_lock(root);

	sb->s_maxbytes = 0;
	err = au_opts_mount(sb, &opts);
	au_opts_free(&opts);
	if (unlikely(err))
		goto out_unlock;
	AuDebugOn(!sb->s_maxbytes);

	aufs_write_unlock(root);
	vfsub_i_unlock(inode);
	goto out_opts; /* success */

 out_unlock:
	aufs_write_unlock(root);
	vfsub_i_unlock(inode);
 out_root:
	dput(root);
	sb->s_root = NULL;
 out_info:
	au_si_free(sb, /*err*/1);
	sb->s_fs_info = NULL;
 out_opts:
	free_page((unsigned long)opts.opt);
 out:
	AuTraceErr(err);
	err = cvt_err(err);
	AuTraceErr(err);
	return err;
}

/* ---------------------------------------------------------------------- */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18)
static int aufs_get_sb(struct file_system_type *fs_type, int flags,
		       const char *dev_name, void *raw_data,
		       struct vfsmount *mnt)
{
	int err;

	/* all timestamps always follow the ones on the branch */
	/* mnt->mnt_flags |= MNT_NOATIME | MNT_NODIRATIME; */
	err = get_sb_nodev(fs_type, flags, raw_data, aufs_fill_super, mnt);
	if (!err) {
		struct super_block *sb = mnt->mnt_sb;
		struct au_sbinfo *sbinfo = au_sbi(sb);
		sbinfo->si_mnt = mnt;

		au_sbilist_lock();
		au_sbilist_add(sbinfo);
		si_write_lock(sb);
		sysaufs_sbi_add(sb);
		si_write_unlock(sb);
		au_sbilist_unlock();
	}
	return err;
}
#else
static struct super_block *aufs_get_sb(struct file_system_type *fs_type,
				       int flags, const char *dev_name,
				       void *raw_data)
{
	return get_sb_nodev(fs_type, flags, raw_data, aufs_fill_super);
}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18) */

struct file_system_type aufs_fs_type = {
	.name		= AUFS_FSTYPE,
	.fs_flags	=
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
		FS_RENAME_DOES_D_MOVE |	/* a race between rename and others */
#endif
		FS_REVAL_DOT,		/* for NFS branch and udba */
	.get_sb		= aufs_get_sb,
	.kill_sb	= generic_shutdown_super,
	/* no need to __module_get() and module_put(). */
	.owner		= THIS_MODULE,
};
