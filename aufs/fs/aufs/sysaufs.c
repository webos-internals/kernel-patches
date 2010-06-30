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
 * $Id: sysaufs.c,v 1.46 2009/01/26 06:24:45 sfjro Exp $
 */

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/sysfs.h>
#include "aufs.h"

/* ---------------------------------------------------------------------- */

/* super_blocks list is not exported */
DEFINE_MUTEX(au_sbilist_mtx);
LIST_HEAD(au_sbilist);

static decl_subsys(aufs, NULL, NULL);
enum {
	Sbi, Stat, Config,
#ifdef CONFIG_AUFS_DEBUG
	Debug,
#endif
	_Last
};

static ssize_t config_show(au_subsys_t *subsys, char *buf)
{
	//struct kset *kset = &au_subsys_to_kset(*subsys);
#define conf_bool(name)	"CONFIG_AUFS_" #name "=y\n"
	static const char opt[] =
#ifdef CONFIG_AUFS
		"CONFIG_AUFS=y\n"
#else
		"CONFIG_AUFS=m\n"
#endif
#ifdef CONFIG_AUFS_BRANCH_MAX_127
		conf_bool(BRANCH_MAX_127)
#elif defined(CONFIG_AUFS_BRANCH_MAX_511)
		conf_bool(BRANCH_MAX_511)
#elif defined(CONFIG_AUFS_BRANCH_MAX_1023)
		conf_bool(BRANCH_MAX_1023)
#elif defined(CONFIG_AUFS_BRANCH_MAX_32767)
		conf_bool(BRANCH_MAX_32767)
#endif
		conf_bool(SYSAUFS)
#ifdef CONFIG_AUFS_HINOTIFY
		conf_bool(HINOTIFY)
#endif
#ifdef CONFIG_AUFS_EXPORT
		conf_bool(EXPORT)
#endif
#ifdef CONFIG_AUFS_INO_T_64
		conf_bool(INO_T_64)
#endif
#ifdef CONFIG_AUFS_ROBR
		conf_bool(ROBR)
#endif
#ifdef CONFIG_AUFS_DLGT
		conf_bool(DLGT)
#endif
#ifdef CONFIG_AUFS_HIN_OR_DLGT
		conf_bool(HIN_OR_DLGT)
#endif
#ifdef CONFIG_AUFS_SHWH
		conf_bool(SHWH)
#endif
#ifdef CONFIG_AUFS_RR_SQUASHFS
		conf_bool(RR_SQUASHFS)
#endif
#ifdef CONFIG_AUFS_SEC_PERM_PATCH
		conf_bool(SEC_PERM_PATCH)
#endif
#ifdef CONFIG_AUFS_SPLICE_PATCH
		conf_bool(SPLICE_PATCH)
#endif
#ifdef CONFIG_AUFS_LHASH_PATCH
		conf_bool(LHASH_PATCH)
#endif
#ifdef CONFIG_AUFS_PUT_FILP_PATCH
		conf_bool(PUT_FILP_PATCH)
#endif
#ifdef CONFIG_AUFS_BR_NFS
		conf_bool(BR_NFS)
#endif
#ifdef CONFIG_AUFS_BR_NFS_V4
		conf_bool(BR_NFS_V4)
#endif
#ifdef CONFIG_AUFS_BR_XFS
		conf_bool(BR_XFS)
#endif
#ifdef CONFIG_AUFS_FSYNC_SUPER_PATCH
		conf_bool(FSYNC_SUPER_PATCH)
#endif
#ifdef CONFIG_AUFS_DENY_WRITE_ACCESS_PATCH
		conf_bool(DENY_WRITE_ACCESS_PATCH)
#endif
#ifdef CONFIG_AUFS_KSIZE_PATCH
		conf_bool(KSIZE_PATCH)
#endif
#ifdef CONFIG_AUFS_WORKAROUND_FUSE
		conf_bool(WORKAROUND_FUSE)
#endif
#ifdef CONFIG_AUFS_GETATTR
		conf_bool(GETATTR)
#endif
#ifdef CONFIG_AUFS_DEBUG
		conf_bool(DEBUG)
#endif
#ifdef CONFIG_AUFS_MAGIC_SYSRQ
		conf_bool(MAGIC_SYSRQ)
#endif
#ifdef CONFIG_AUFS_DEBUG_LOCK
		conf_bool(DEBUG_LOCK)
#endif
#ifdef CONFIG_AUFS_COMPAT
		conf_bool(COMPAT)
#endif
#ifdef CONFIG_AUFS_UNIONFS23_PATCH
		conf_bool(UNIONFS23_PATCH)
#endif
		;
#undef conf_bool

	char *p = buf;
	const char *end = buf + PAGE_SIZE;

	p += snprintf(p, end - p, "%s", opt);
#ifdef DbgUdbaRace
	if (p < end)
		p += snprintf(p, end - p, "DbgUdbaRace=%d\n", DbgUdbaRace);
#endif
	if (p < end)
		return p - buf;
	else
		return -EFBIG;
}

static ssize_t stat_show(au_subsys_t *subsys, char *buf)
{
	//struct kset *kset = &au_subsys_to_kset(*subsys);
	char *p = buf;
	const char *end = buf + PAGE_SIZE;
	int i;

	p += snprintf(p, end - p, "wkq max_busy:");
	for (i = 0; p < end && i < aufs_nwkq; i++)
		p += snprintf(p, end - p, " %u", au_wkq[i].max_busy);
	if (p < end)
		p += snprintf(p, end - p, ", %u(generic)\n",
			      au_wkq[aufs_nwkq].max_busy);

	if (p < end)
		return p - buf;
	else
		return -EFBIG;
}

static ssize_t sbi_show(au_subsys_t *subsys, char *buf)
{
	//struct kset *kset = &au_subsys_to_kset(*subsys);
	char *p = buf;
	const char *end = buf + PAGE_SIZE;
	struct au_sbinfo *sbinfo;

	/* this order is important */
	mutex_lock(&au_sbilist_mtx);
	list_for_each_entry(sbinfo, &au_sbilist, si_list)
		if (p < end)
			p += snprintf(p, end - p, "%p\n", sbinfo);
	mutex_unlock(&au_sbilist_mtx);

	if (p < end)
		return p - buf;
	else
		return -EFBIG;
}

#ifdef CONFIG_AUFS_DEBUG
static ssize_t debug_show(au_subsys_t *subsys, char *buf)
{
	//struct kset *kset = &au_subsys_to_kset(*subsys);
	return sprintf(buf, "%d\n", au_debug_test());
}

static ssize_t debug_store(au_subsys_t *subsys, const char *buf, size_t sz)
{
	//struct kset *kset = &au_subsys_to_kset(*subsys);

	LKTRTrace("%.*s\n", (unsigned int)sz, buf);

	if (unlikely(!sz || (*buf != '0' && *buf != '1')))
		return -EOPNOTSUPP;

	if (*buf == '0')
		au_debug_off();
	else if (*buf == '1')
		au_debug_on();
	return sz;
}
#endif

static struct subsys_attribute sysaufs_entry[] = {
	[Config]	 = __ATTR_RO(config),
	[Stat]		 = __ATTR_RO(stat),
	[Sbi]		 = __ATTR_RO(sbi),
#ifdef CONFIG_AUFS_DEBUG
	[Debug]		 = __ATTR(debug, S_IRUGO | S_IWUSR, debug_show,
				  debug_store)
#endif
};
static struct attribute *sysaufs_attr[_Last + 1]; /* last NULL */
static struct attribute_group sysaufs_attr_group = {
	.attrs = sysaufs_attr,
};

/* ---------------------------------------------------------------------- */

typedef int (*sysaufs_sbi_sub_show_t)(struct seq_file *seq,
				      struct super_block *sb);

static int sysaufs_sbi_xi(struct seq_file *seq, struct file *xf, int dlgt,
			  int print_path)
{
	int err;
	struct kstat st;

	err = vfsub_getattr(xf->f_vfsmnt, xf->f_dentry, &st, dlgt);
	if (!err) {
		seq_printf(seq, "%llux%lu %lld",
			   st.blocks, st.blksize, (long long)st.size);
		if (print_path) {
			seq_putc(seq, ' ');
			seq_path(seq, xf->f_vfsmnt, xf->f_dentry, au_esc_chars);
		}
		seq_putc(seq, '\n');
	} else
		seq_printf(seq, "err %d\n", err);

	AuTraceErr(err);
	return err;
}

static int sysaufs_sbi_xino(struct seq_file *seq, struct super_block *sb)
{
	int err;
	unsigned int mnt_flags;
	unsigned char dlgt, xinodir;
	aufs_bindex_t bend, bindex;
	struct kstat st;
	struct au_sbinfo *sbinfo;
	struct file *xf;

	AuTraceEnter();

	sbinfo = au_sbi(sb);
	mnt_flags = au_mntflags(sb);
	xinodir = !!au_opt_test(mnt_flags, XINODIR);
	if (!au_opt_test(mnt_flags, XINO)) {
#ifdef CONFIG_AUFS_DEBUG
		AuDebugOn(sbinfo->si_xib);
		bend = au_sbend(sb);
		for (bindex = 0; bindex <= bend; bindex++)
			AuDebugOn(au_sbr(sb, bindex)->br_xino.xi_file);
#endif
		err = 0;
		goto out; /* success */
	}

	dlgt = !!au_test_dlgt(mnt_flags);
	err = sysaufs_sbi_xi(seq, sbinfo->si_xib, dlgt, xinodir);

	bend = au_sbend(sb);
	for (bindex = 0; !err && bindex <= bend; bindex++) {
		xf = au_sbr(sb, bindex)->br_xino.xi_file;
		if (!xf)
			continue;
		seq_printf(seq, "%d: ", bindex);
		err = vfsub_getattr(xf->f_vfsmnt, xf->f_dentry, &st, dlgt);
		if (!err)
			seq_printf(seq, "%d, %Lux%lu %Ld\n",
				   file_count(xf), st.blocks, st.blksize,
				   (long long)st.size);
		else
			seq_printf(seq, "err %d\n", err);
	}

 out:
	AuTraceErr(err);
	return err;
}

#ifdef CONFIG_AUFS_EXPORT
static int sysaufs_sbi_xigen(struct seq_file *seq, struct super_block *sb)
{
	int err;
	unsigned int mnt_flags;
	struct au_sbinfo *sbinfo;

	AuTraceEnter();

	err = 0;
	sbinfo = au_sbi(sb);
	mnt_flags = au_mntflags(sb);
	if (au_opt_test_xino(mnt_flags))
		err = sysaufs_sbi_xi(seq, sbinfo->si_xigen,
				     !!au_opt_test(mnt_flags, DLGT),
				     !!au_opt_test(mnt_flags, XINODIR));

	AuTraceErr(err);
	return err;
}
#endif

static int sysaufs_sbi_mntpnt1(struct seq_file *seq, struct super_block *sb)
{
	AuTraceEnter();

	seq_path(seq, au_sbi(sb)->si_mnt, sb->s_root, au_esc_chars);
	seq_putc(seq, '\n');

	return 0;
}

#define SysaufsBr_PREFIX "br"
static int sysaufs_sbi_br(struct seq_file *seq, struct super_block *sb,
			  aufs_bindex_t bindex)
{
	int err;
	struct dentry *root;
	struct au_branch *br;

	LKTRTrace("b%d\n", bindex);

	err = -ENOENT;
	if (unlikely(au_sbend(sb) < bindex))
		goto out;

	err = 0;
	root = sb->s_root;
	di_read_lock_parent(root, !AuLock_IR);
	br = au_sbr(sb, bindex);
	seq_path(seq, br->br_mnt, au_h_dptr(root, bindex), au_esc_chars);
	di_read_unlock(root, !AuLock_IR);
	seq_printf(seq, "=%s\n", au_optstr_br_perm(br->br_perm));

 out:
	AuTraceErr(err);
	return err;
}

/* ---------------------------------------------------------------------- */

static struct sysaufs_sbi_entry {
	char *name;
	sysaufs_sbi_sub_show_t show;
} sysaufs_sbi_entry[SysaufsSb_Last] = {
	[SysaufsSb_XINO]	= {
		.name	= "xino",
		.show	= sysaufs_sbi_xino
	},
#ifdef CONFIG_AUFS_EXPORT
	[SysaufsSb_XIGEN]	= {
		.name	= "xigen",
		.show	= sysaufs_sbi_xigen
	},
#endif
	[SysaufsSb_MNTPNT1]	= {
		.name	= "mntpnt1",
		.show	= sysaufs_sbi_mntpnt1
	}
};

static struct super_block *find_sb_lock(struct kobject *kobj)
{
	struct super_block *sb;
	struct au_sbinfo *sbinfo;

	AuTraceEnter();
	MtxMustLock(&au_sbilist_mtx);

	sb = NULL;
	list_for_each_entry(sbinfo, &au_sbilist, si_list) {
		if (&au_subsys_to_kset(sbinfo->si_sa->subsys).kobj != kobj)
			continue;
		sb = sbinfo->si_mnt->mnt_sb;
		si_read_lock(sb, !AuLock_FLUSH);
		break;
	}
	return sb;
}

static struct seq_file *au_seq(char *p, ssize_t len)
{
	struct seq_file *seq;

	seq = kzalloc(sizeof(*seq), GFP_NOFS);
	if (seq) {
		//mutex_init(&seq.lock);
		seq->buf = p;
		seq->count = 0;
		seq->size = len;
		return seq; /* success */
	}

	seq = ERR_PTR(-ENOMEM);
	AuTraceErrPtr(seq);
	return seq;
}

//todo: file size may exceed PAGE_SIZE
static ssize_t sysaufs_sbi_show(struct kobject *kobj, struct attribute *attr,
				char *buf)
{
	ssize_t err;
	struct super_block *sb;
	struct seq_file *seq;
	char *name;
	int i;

	LKTRTrace("%s/%s\n", kobject_name(kobj), attr->name);

	err = -ENOENT;
	mutex_lock(&au_sbilist_mtx);
	sb = find_sb_lock(kobj);
	if (unlikely(!sb))
		goto out;

	seq = au_seq(buf, PAGE_SIZE);
	err = PTR_ERR(seq);
	if (IS_ERR(seq))
		goto out;

	name = (void *)attr->name;
	for (i = 0; i < SysaufsSb_Last; i++) {
		if (!strcmp(name, sysaufs_sbi_entry[i].name)) {
			err = sysaufs_sbi_entry[i].show(seq, sb);
			goto out_seq;
		}
	}

	if (!strncmp(name, SysaufsBr_PREFIX, sizeof(SysaufsBr_PREFIX) - 1)) {
		name += sizeof(SysaufsBr_PREFIX) - 1;
		err = sysaufs_sbi_br(seq, sb, simple_strtol(name, NULL, 10));
		goto out_seq;
	}
	BUG();

 out_seq:
	if (!err) {
		err = seq->count;
		/* sysfs limit */
		if (unlikely(err == PAGE_SIZE))
			err = -EFBIG;
	}
	kfree(seq);
 out:
	si_read_unlock(sb);
	mutex_unlock(&au_sbilist_mtx);
	AuTraceErr(err);
	return err;
}

static struct sysfs_ops sysaufs_sbi_ops = {
	.show   = sysaufs_sbi_show
};

static struct kobj_type sysaufs_sbi_ktype = {
	//.release	= dir_release,
	.sysfs_ops	= &sysaufs_sbi_ops
};

/* ---------------------------------------------------------------------- */

static void sysaufs_br_free(struct kref *ref)
{
	AuTraceEnter();
	//AuDbg("here\n");
	kfree(container_of(ref, struct sysaufs_br, ref));
}

struct sysaufs_br *sysaufs_br_alloc(void)
{
	struct sysaufs_br *sabr;

	AuTraceEnter();
	//AuDbg("here\n");

	sabr = kcalloc(sizeof(*sabr), 1, GFP_NOFS);
	if (sabr)
		kref_init(&sabr->ref);

	AuTraceErrPtr(sabr);
	return sabr;
}

void sysaufs_br_get(struct au_branch *br)
{
	kref_get(&br->br_sabr->ref);
}

void sysaufs_br_put(struct au_branch *br)
{
	kref_put(&br->br_sabr->ref, sysaufs_br_free);
}

//todo: they may take a long time
void sysaufs_brs_del(struct super_block *sb)
{
	aufs_bindex_t bindex, bend;
	struct sysaufs_sbinfo *sa;
	struct sysaufs_br *sabr;
	struct au_branch *br;

	AuTraceEnter();

	sa = au_sbi(sb)->si_sa;
	if (!sysaufs_brs || !sa->added)
		return;

	bend = au_sbend(sb);
	for (bindex = 0; bindex <= bend; bindex++) {
		br = au_sbr(sb, bindex);
		sabr = br->br_sabr;
		if (unlikely(!sabr->added))
			continue;

		sabr->added = 0;
		sysfs_remove_file(&au_subsys_to_kset(sa->subsys).kobj,
				  &sabr->attr);
		sysaufs_br_put(br);
		/* for older sysfs */
		module_put(THIS_MODULE);
	}
}

void sysaufs_brs_add(struct super_block *sb)
{
	int err;
	aufs_bindex_t bindex, bend;
	struct sysaufs_br *sabr;
	struct sysaufs_sbinfo *sa;
	struct au_branch *br;

	AuTraceEnter();

	sa = au_sbi(sb)->si_sa;
	if (!sysaufs_brs || !sa->added)
		return;

	bend = au_sbend(sb);
	for (bindex = 0; bindex <= bend; bindex++) {
		br = au_sbr(sb, bindex);
		sysaufs_br_get(br);
		sabr = br->br_sabr;
		AuDebugOn(sabr->added);
		snprintf(sabr->name, sizeof(sabr->name),
			 SysaufsBr_PREFIX "%d", bindex);
		sabr->attr.name = sabr->name;
		sabr->attr.mode = 0444;
		sabr->attr.owner = THIS_MODULE;
		/* for older sysfs */
		BUG_ON(!try_module_get(THIS_MODULE));
		err = sysfs_create_file(&au_subsys_to_kset(sa->subsys).kobj,
					&sabr->attr);
		if (!err)
			sabr->added = 1;
		else
			AuWarn("failed %s under sysfs(%d)\n", sabr->name, err);
	}
}

/* ---------------------------------------------------------------------- */

static void sysaufs_sbi_free(struct kref *ref)
{
	AuTraceEnter();
	//AuDbg("here\n");
	kfree(container_of(ref, struct sysaufs_sbinfo, ref));
}

int sysaufs_si_init(struct au_sbinfo *sbinfo)
{
	sbinfo->si_sa = kcalloc(sizeof(*sbinfo->si_sa), 1, GFP_NOFS);
	if (sbinfo->si_sa) {
		kref_init(&sbinfo->si_sa->ref);
		return 0;
	}
	AuTraceErr(-ENOMEM);
	return -ENOMEM;
}

void sysaufs_sbi_get(struct super_block *sb)
{
	kref_get(&au_sbi(sb)->si_sa->ref);
}

void sysaufs_sbi_put(struct super_block *sb)
{
	kref_put(&au_sbi(sb)->si_sa->ref, sysaufs_sbi_free);
}

void sysaufs_sbi_add(struct super_block *sb)
{
	int err, i;
	const char *name;
	struct au_sbinfo *sbinfo = au_sbi(sb);
	struct sysaufs_sbinfo *sa = sbinfo->si_sa;

	AuTraceEnter();
	MtxMustLock(&au_sbilist_mtx);

	err = kobject_set_name(&au_subsys_to_kset(sa->subsys).kobj,
			       "sbi_%p", sbinfo);
	if (unlikely(err))
		goto out;
	name = kobject_name(&au_subsys_to_kset(sa->subsys).kobj);
	subsys_get(&aufs_subsys);
	kobj_set_kset_s(&au_subsys_to_kset(sa->subsys), aufs_subsys);
	//au_subsys_to_kset(sa->subsys).kobj.ktype = &sysaufs_ktype;
	err = subsystem_register(&sa->subsys);
	if (unlikely(err))
		goto out_put;
#if 1
	if (au_subsys_to_kset(sa->subsys).kobj.ktype)
		sysaufs_sbi_ktype = *(au_subsys_to_kset(sa->subsys).kobj.ktype);
	sysaufs_sbi_ktype.sysfs_ops = &sysaufs_sbi_ops;
	au_subsys_to_kset(sa->subsys).kobj.ktype = &sysaufs_sbi_ktype;
#endif

	for (i = 0; i < SysaufsSb_Last; i++) {
		sa->attr[i].name = sysaufs_sbi_entry[i].name;
		sa->attr[i].mode = S_IRUGO;
		sa->attr[i].owner = THIS_MODULE;
		/* for older sysfs */
		BUG_ON(!try_module_get(THIS_MODULE));
		err = sysfs_create_file(&au_subsys_to_kset(sa->subsys).kobj,
					sa->attr + i);
		if (unlikely(err))
			goto out_reg;
	}

	/* compatibility for some aufs scripts */
	strcpy(sa->compat_name, "si_");
	strcat(sa->compat_name, name + 4);
	err = sysfs_create_link(&au_subsys_to_kset(aufs_subsys).kobj,
				&au_subsys_to_kset(sa->subsys).kobj,
				sa->compat_name);
	if (unlikely(err)) {
		AuWarn("failed adding %s for compatibility(%d), ignored.\n",
		       sa->compat_name, err);
		err = 0;
	}

	sa->added = 1;
	sysaufs_brs_add(sb);
	goto out; /* success */

 out_reg:
	for (; i >= 0; i--) {
		sysfs_remove_file(&au_subsys_to_kset(sa->subsys).kobj,
				  sa->attr + i);
		/* for older sysfs */
		module_put(THIS_MODULE);
	}
	subsystem_unregister(&sa->subsys);
 out_put:
	subsys_put(&aufs_subsys);
 out:
	if (!err)
		sysaufs_sbi_get(sb);
	else
		AuWarn("failed adding sysfs (%d)\n", err);
}

void sysaufs_sbi_del(struct super_block *sb)
{
	struct au_sbinfo *sbinfo = au_sbi(sb);
	struct sysaufs_sbinfo *sa = sbinfo->si_sa;
	int i;

	AuTraceEnter();
	MtxMustLock(&au_sbilist_mtx);

	sysaufs_brs_del(sb);
	for (i = 0; i < SysaufsSb_Last; i++) {
		sysfs_remove_file(&au_subsys_to_kset(sa->subsys).kobj,
				  sa->attr + i);
		/* for older sysfs */
		module_put(THIS_MODULE);
	}
	sa->added = 0;
	sysfs_remove_link(&au_subsys_to_kset(aufs_subsys).kobj,
			  sa->compat_name);
	subsystem_unregister(&sa->subsys);
	subsys_put(&aufs_subsys);
	sysaufs_sbi_put(sb);
}

/* ---------------------------------------------------------------------- */

int __init sysaufs_init(void)
{
	int err, i;

	for (i = 0; i < _Last; i++)
		sysaufs_attr[i] = &sysaufs_entry[i].attr;

	subsys_get(&fs_subsys);
	kobj_set_kset_s(&au_subsys_to_kset(aufs_subsys), fs_subsys);
	err = subsystem_register(&aufs_subsys);
	if (unlikely(err))
		goto out_put;
	err = sysfs_create_group(&au_subsys_to_kset(aufs_subsys).kobj,
				 &sysaufs_attr_group);
	if (!err)
		return err; /* success */

	subsystem_unregister(&aufs_subsys);
 out_put:
	subsys_put(&fs_subsys);
	AuTraceErr(err);
	return err;
}

void sysaufs_fin(void)
{
	AuDebugOn(!list_empty(&au_sbilist));
	sysfs_remove_group(&au_subsys_to_kset(aufs_subsys).kobj,
			   &sysaufs_attr_group);
	subsystem_unregister(&aufs_subsys);
	subsys_put(&fs_subsys);
}
