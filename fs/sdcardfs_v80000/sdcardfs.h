/*
 * fs/sdcardfs/sdcardfs.h
 *
 * The sdcardfs v2.0 
 *   This file system replaces the sdcard daemon on Android 
 *   On version 2.0, some of the daemon functions have been ported  
 *   to support the multi-user concepts of Android 4.4
 *
 * Copyright (c) 2013 Samsung Electronics Co. Ltd
 *   Authors: Daeho Jeong, Woojoong Lee, Seunghwan Hyun, 
 *               Sunghwan Yun, Sungjong Seo
 *                      
 * This program has been developed as a stackable file system based on
 * the WrapFS which written by 
 *
 * Copyright (c) 1998-2011 Erez Zadok
 * Copyright (c) 2009     Shrikar Archak
 * Copyright (c) 2003-2011 Stony Brook University
 * Copyright (c) 2003-2011 The Research Foundation of SUNY
 *
 * This file is dual licensed.  It may be redistributed and/or modified
 * under the terms of the Apache 2.0 License OR version 2 of the GNU
 * General Public License.
 */

#ifndef _SDCARDFS_H_
#define _SDCARDFS_H_

#include <linux/dcache.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/aio.h>
#include <linux/kref.h>
#include <linux/mm.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/seq_file.h>
#include <linux/statfs.h>
#include <linux/fs_stack.h>
#include <linux/magic.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/security.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/ratelimit.h>
#include <linux/android_aid.h>
#include "multiuser.h"

/* the file system name */
#define SDCARDFS_NAME "sdcardfs"

/* sdcardfs root inode number */
#define SDCARDFS_ROOT_INO     1

/* useful for tracking code reachability */
#define UDBG printk(KERN_DEFAULT "DBG:%s:%s:%d\n", __FILE__, __func__, __LINE__)

#define SDCARDFS_DIRENT_SIZE 256

/* temporary static uid settings for development */ 
#define AID_ROOT             0	/* uid for accessing /mnt/sdcard & extSdcard */
#define AID_MEDIA_RW      1023	/* internal media storage write access */

#define AID_SDCARD_RW     1015	/* external storage write access */
#define AID_SDCARD_R      1028	/* external storage read access */
#define AID_SDCARD_PICS   1033	/* external storage photos access */
#define AID_SDCARD_AV     1034	/* external storage audio/video access */
#define AID_SDCARD_ALL    1035	/* access all users external storage */
#define AID_MEDIA_OBB     1059  /* obb files */

#define AID_SDCARD_IMAGE  1057

#define AID_PACKAGE_INFO  1027


/*
 * Permissions are handled by our permission function.
 * We don't want anyone who happens to look at our inode value to prematurely
 * block access, so store more permissive values. These are probably never
 * used.
 */
#define fixup_tmp_permissions(x)	\
	do {						\
		(x)->i_uid = make_kuid(&init_user_ns,	\
				SDCARDFS_I(x)->data->d_uid);	\
		(x)->i_gid = make_kgid(&init_user_ns, AID_SDCARD_RW);	\
		(x)->i_mode = ((x)->i_mode & S_IFMT) | 0775;\
	} while (0)

/* Android 5.0 support */

/* Permission mode for a specific node. Controls how file permissions
 * are derived for children nodes. */
typedef enum {
	/* Nothing special; this node should just inherit from its parent. */
	PERM_INHERIT,
	/* This node is one level above a normal root; used for legacy layouts
	 * which use the first level to represent user_id. */
	PERM_PRE_ROOT,
	/* This node is "/" */
	PERM_ROOT,
	/* This node is "/Android" */
	PERM_ANDROID,
	/* This node is "/Android/data" */
	PERM_ANDROID_DATA,
	/* This node is "/Android/obb" */
	PERM_ANDROID_OBB,
	/* This node is "/Android/media" */
	PERM_ANDROID_MEDIA,
	/* This node is "/Android/[data|media|obb]/[package]" */
	PERM_ANDROID_PACKAGE,
	/* This node is "/Android/[data|media|obb]/[package]/cache" */
	PERM_ANDROID_PACKAGE_CACHE,
	/*
	 * The knox directory has different uses depending on whether it's
	 * used for external storage or secondary storage.
	 *
	 * 1. external storage
	 * It's used for Andorid For Work(AFW) to provide SDP feature.
	 * /mnt/shell/enc_emulated/10 will be bind mounted on it.
	 *
	 * 2. Secondary storage(external SD Card)
	 * Knox doesn't encrypt files in secondary storage. Instead,
	 * it restricts access to Knox files by DAC.
	 */
	/* This node is /knox */
	PERM_KNOX_PRE_ROOT,
	/* This node is /knox/[userid] */
	PERM_KNOX_ROOT,
	/* This node is /knox/[userid]/Android */
	PERM_KNOX_ANDROID,
	/* This node is /knox/[userid]/Android/[data|shared] */
	PERM_KNOX_ANDROID_DATA,
	PERM_KNOX_ANDROID_SHARED,
	/* This node is /knox/[userid]/Android/[data|shared]/[package] */
	PERM_KNOX_ANDROID_PACKAGE,
} perm_t;

struct sdcardfs_sb_info;
struct sdcardfs_mount_options;
struct sdcardfs_inode_info;
struct sdcardfs_inode_data;

/* Do not directly use this function. Use OVERRIDE_CRED() instead. */
const struct cred *override_fsids(struct sdcardfs_sb_info *sbi,
			struct sdcardfs_inode_data *data);
/* Do not directly use this function, use REVERT_CRED() instead. */
void revert_fsids(const struct cred * old_cred);

/* operations vectors defined in specific files */
extern const struct file_operations sdcardfs_main_fops;
extern const struct file_operations sdcardfs_dir_fops;
extern const struct inode_operations sdcardfs_main_iops;
extern const struct inode_operations sdcardfs_dir_iops;
extern const struct inode_operations sdcardfs_symlink_iops;
extern const struct super_operations sdcardfs_sops;
extern const struct dentry_operations sdcardfs_ci_dops;
extern const struct address_space_operations sdcardfs_aops, sdcardfs_dummy_aops;
extern const struct vm_operations_struct sdcardfs_vm_ops;

extern int sdcardfs_init_inode_cache(void);
extern void sdcardfs_destroy_inode_cache(void);
extern int sdcardfs_init_dentry_cache(void);
extern void sdcardfs_destroy_dentry_cache(void);
extern int new_dentry_private_data(struct dentry *dentry);
extern void free_dentry_private_data(struct dentry *dentry);
extern struct dentry *sdcardfs_lookup(struct inode *dir, struct dentry *dentry,
				unsigned int flags);
extern struct inode *sdcardfs_iget(struct super_block *sb,
				 struct inode *lower_inode, userid_t id);
extern int sdcardfs_interpose(struct dentry *dentry, struct super_block *sb,
			    struct path *lower_path, userid_t id);

/* file private data */
struct sdcardfs_file_info {
	struct file *lower_file;
	const struct vm_operations_struct *lower_vm_ops;
};

struct sdcardfs_inode_data {
	struct kref refcount;
	bool abandoned;

	perm_t perm;
	userid_t userid;
	uid_t d_uid;
	bool under_android;
	bool under_cache;
	bool under_obb;

	bool under_knox;
};

/* sdcardfs inode data in memory */
struct sdcardfs_inode_info {
	struct inode *lower_inode;
	/* state derived based on current position in hierarchy */
	struct sdcardfs_inode_data *data;

	/* top folder for ownership */
	struct sdcardfs_inode_data *top_data;

	struct inode vfs_inode;
};


/* sdcardfs dentry data in memory */
struct sdcardfs_dentry_info {
	spinlock_t lock;	/* protects lower_path */
	struct path lower_path;
	struct path orig_path;
};

struct sdcardfs_mount_options {
	uid_t fs_low_uid;
	gid_t fs_low_gid;
	userid_t fs_user_id;
	bool multiuser;
	bool gid_derivation;
	unsigned int reserved_mb;
};

struct sdcardfs_vfsmount_options {
	gid_t gid;
	mode_t mask;
};

extern int parse_options_remount(struct super_block *sb, char *options, int silent,
		struct sdcardfs_vfsmount_options *vfsopts);

/* sdcardfs super-block data in memory */
struct sdcardfs_sb_info {
	struct super_block *sb;
	struct super_block *lower_sb;
	/* derived perm policy : some of options have been added
	 * to sdcardfs_mount_options (Android 4.4 support)
	 */
	struct sdcardfs_mount_options options;
	spinlock_t lock;	/* protects obbpath */
	char *obbpath_s;
	struct path obbpath;
	void *pkgl_id;
	struct list_head list;
};

/*
 * inode to private data
 *
 * Since we use containers and the struct inode is _inside_ the
 * sdcardfs_inode_info structure, SDCARDFS_I will always (given a non-NULL
 * inode pointer), return a valid non-NULL pointer.
 */
static inline struct sdcardfs_inode_info *SDCARDFS_I(const struct inode *inode)
{
	return container_of(inode, struct sdcardfs_inode_info, vfs_inode);
}

/* dentry to private data */
#define SDCARDFS_D(dent) ((struct sdcardfs_dentry_info *)(dent)->d_fsdata)

/* superblock to private data */
#define SDCARDFS_SB(super) ((struct sdcardfs_sb_info *)(super)->s_fs_info)

/* file to private Data */
#define SDCARDFS_F(file) ((struct sdcardfs_file_info *)((file)->private_data))

/* file to lower file */
static inline struct file *sdcardfs_lower_file(const struct file *f)
{
	return SDCARDFS_F(f)->lower_file;
}

static inline void sdcardfs_set_lower_file(struct file *f, struct file *val)
{
	SDCARDFS_F(f)->lower_file = val;
}

/* inode to lower inode. */
static inline struct inode *sdcardfs_lower_inode(const struct inode *i)
{
	return SDCARDFS_I(i)->lower_inode;
}

static inline void sdcardfs_set_lower_inode(struct inode *i, struct inode *val)
{
	SDCARDFS_I(i)->lower_inode = val;
}

/* superblock to lower superblock */
static inline struct super_block *sdcardfs_lower_super(
	const struct super_block *sb)
{
	return SDCARDFS_SB(sb)->lower_sb;
}

static inline void sdcardfs_set_lower_super(struct super_block *sb,
					  struct super_block *val)
{
	SDCARDFS_SB(sb)->lower_sb = val;
}

/* path based (dentry/mnt) macros */
static inline void pathcpy(struct path *dst, const struct path *src)
{
	dst->dentry = src->dentry;
	dst->mnt = src->mnt;
}

/* sdcardfs_get_pname functions calls path_get()
 * therefore, the caller must call "proper" path_put functions 
 */
#define SDCARDFS_DENT_FUNC(pname) \
static inline void sdcardfs_get_##pname(const struct dentry *dent, \
					struct path *pname) \
{ \
	spin_lock(&SDCARDFS_D(dent)->lock); \
	pathcpy(pname, &SDCARDFS_D(dent)->pname); \
	path_get(pname); \
	spin_unlock(&SDCARDFS_D(dent)->lock); \
	return; \
} \
static inline void sdcardfs_put_##pname(const struct dentry *dent, \
					struct path *pname) \
{ \
	path_put(pname); \
	return; \
} \
static inline void sdcardfs_set_##pname(const struct dentry *dent, \
					struct path *pname) \
{ \
	spin_lock(&SDCARDFS_D(dent)->lock); \
	pathcpy(&SDCARDFS_D(dent)->pname, pname); \
	spin_unlock(&SDCARDFS_D(dent)->lock); \
	return; \
} \
static inline void sdcardfs_reset_##pname(const struct dentry *dent) \
{ \
	spin_lock(&SDCARDFS_D(dent)->lock); \
	SDCARDFS_D(dent)->pname.dentry = NULL; \
	SDCARDFS_D(dent)->pname.mnt = NULL; \
	spin_unlock(&SDCARDFS_D(dent)->lock); \
	return; \
} \
static inline void sdcardfs_put_reset_##pname(const struct dentry *dent) \
{ \
	struct path pname; \
	spin_lock(&SDCARDFS_D(dent)->lock); \
	if(SDCARDFS_D(dent)->pname.dentry) { \
		pathcpy(&pname, &SDCARDFS_D(dent)->pname); \
		SDCARDFS_D(dent)->pname.dentry = NULL; \
		SDCARDFS_D(dent)->pname.mnt = NULL; \
		spin_unlock(&SDCARDFS_D(dent)->lock); \
		path_put(&pname); \
	} else \
		spin_unlock(&SDCARDFS_D(dent)->lock); \
	return; \
} 

SDCARDFS_DENT_FUNC(lower_path) 
SDCARDFS_DENT_FUNC(orig_path)  

static inline bool sbinfo_has_sdcard_magic(struct sdcardfs_sb_info *sbinfo)
{
	return sbinfo && sbinfo->sb
			&& sbinfo->sb->s_magic == SDCARDFS_SUPER_MAGIC;
}

static inline struct sdcardfs_inode_data *data_get(
		struct sdcardfs_inode_data *data)
{
	if (data)
		kref_get(&data->refcount);
	return data;
}

static inline struct sdcardfs_inode_data *top_data_get(
		struct sdcardfs_inode_info *info)
{
	return data_get(info->top_data);
}

extern void data_release(struct kref *ref);

static inline void data_put(struct sdcardfs_inode_data *data)
{
	kref_put(&data->refcount, data_release);
}

static inline void release_own_data(struct sdcardfs_inode_info *info)
{
	/*
	 * This happens exactly once per inode. At this point, the inode that
	 * originally held this data is about to be freed, and all references
	 * to it are held as a top value, and will likely be released soon.
	 */
	info->data->abandoned = true;
	data_put(info->data);
}

static inline void set_top(struct sdcardfs_inode_info *info,
			struct sdcardfs_inode_data *top)
{
	struct sdcardfs_inode_data *old_top = info->top_data;

	if (top)
		data_get(top);
	info->top_data = top;
	if (old_top)
		data_put(old_top);
}

static inline int get_gid(struct vfsmount *mnt,
		struct sdcardfs_inode_data *data)
{
	struct sdcardfs_vfsmount_options *opts = mnt->data;

	if (data->under_knox) {
		switch (data->perm) {
		case PERM_KNOX_PRE_ROOT:
			return AID_SDCARD_R;
		case PERM_KNOX_ROOT:
		case PERM_KNOX_ANDROID:
		case PERM_KNOX_ANDROID_DATA:
		case PERM_KNOX_ANDROID_PACKAGE:
			return multiuser_get_uid(data->userid, AID_SDCARD_R);
		case PERM_KNOX_ANDROID_SHARED:
			return AID_SDCARD_RW;
		default:
			break;
		}
	}

	if (opts->gid == AID_SDCARD_RW)
		/* As an optimization, certain trusted system components only run
		 * as owner but operate across all users. Since we're now handing
		 * out the sdcard_rw GID only to trusted apps, we're okay relaxing
		 * the user boundary enforcement for the default view. The UIDs
		 * assigned to app directories are still multiuser aware.
		 */
		return AID_SDCARD_RW;
	else
		return multiuser_get_uid(data->userid, opts->gid);
}

static inline int get_mode(struct vfsmount *mnt,
		struct sdcardfs_inode_info *info,
		struct sdcardfs_inode_data *data)
{
	int owner_mode;
	int filtered_mode;
	struct sdcardfs_vfsmount_options *opts = mnt->data;
	int visible_mode = 0775 & ~opts->mask;


	if (data->perm == PERM_PRE_ROOT) {
		/* Top of multi-user view should always be visible to ensure
		* secondary users can traverse inside.
		*/
		visible_mode = 0711;
	} else if (data->under_android) {
		/* Block "other" access to Android directories, since only apps
		* belonging to a specific user should be in there; we still
		* leave +x open for the default view.
		*/
		if (opts->gid == AID_SDCARD_RW)
			visible_mode = visible_mode & ~0006;
		else
			visible_mode = visible_mode & ~0007;
	} else if (data->perm == PERM_KNOX_ANDROID_PACKAGE) {
		visible_mode = visible_mode & ~0006;
	}
	owner_mode = info->lower_inode->i_mode & 0700;
	filtered_mode = visible_mode & (owner_mode | (owner_mode >> 3) | (owner_mode >> 6));
	return filtered_mode;
}

static inline int has_graft_path(const struct dentry *dent)
{
	int ret = 0;

	spin_lock(&SDCARDFS_D(dent)->lock); 
	if (SDCARDFS_D(dent)->orig_path.dentry != NULL)
		ret = 1;
	spin_unlock(&SDCARDFS_D(dent)->lock);

	return ret;
}

static inline void sdcardfs_get_real_lower(const struct dentry *dent,
						struct path *real_lower)
{
	/* in case of a local obb dentry 
	 * the orig_path should be returned 
	 */
	if(has_graft_path(dent)) 
		sdcardfs_get_orig_path(dent, real_lower);
	else 
		sdcardfs_get_lower_path(dent, real_lower);
}

static inline void sdcardfs_put_real_lower(const struct dentry *dent,
						struct path *real_lower)
{
	if(has_graft_path(dent)) 
		sdcardfs_put_orig_path(dent, real_lower);
	else 
		sdcardfs_put_lower_path(dent, real_lower);
}

extern struct mutex sdcardfs_super_list_lock;
extern struct list_head sdcardfs_super_list;

/* for packagelist.c */
extern appid_t get_appid(const char *app_name);
extern appid_t get_ext_gid(const char *app_name);
extern appid_t is_excluded(const char *app_name, userid_t userid);
extern int check_caller_access_to_name(struct inode *parent_node, const struct qstr *name);
extern int packagelist_init(void);
extern void packagelist_exit(void);

/* for derived_perm.c */
#define BY_NAME		(1 << 0)
#define BY_USERID	(1 << 1)
struct limit_search {
	unsigned int flags;
	struct qstr name;
	userid_t userid;
};

extern void setup_derived_state(struct inode *inode, perm_t perm,
		userid_t userid, uid_t uid, bool under_android,
		struct sdcardfs_inode_data *top);
extern void get_derived_permission(struct dentry *parent, struct dentry *dentry);
extern void get_derived_permission_new(struct dentry *parent, struct dentry *dentry, const struct qstr *name);
extern void fixup_perms_recursive(struct dentry *dentry, struct limit_search *limit);

extern void update_derived_permission_lock(struct dentry *dentry);
void fixup_lower_ownership(struct dentry *dentry, const char *name);
extern int need_graft_path(struct dentry *dentry);
extern int is_base_obbpath(struct dentry *dentry);
extern int is_obbpath_invalid(struct dentry *dentry);
extern int setup_obb_dentry(struct dentry *dentry, struct path *lower_path);

/* locking helpers */
static inline struct dentry *lock_parent(struct dentry *dentry)
{
	struct dentry *dir = dget_parent(dentry);
	mutex_lock_nested(&dir->d_inode->i_mutex, I_MUTEX_PARENT);
	return dir;
}

static inline void unlock_dir(struct dentry *dir)
{
	mutex_unlock(&dir->d_inode->i_mutex);
	dput(dir);
}

static inline int prepare_dir(const char *path_s, uid_t uid, gid_t gid, mode_t mode)
{
	int err;
	struct dentry *dent;
	struct iattr attrs;
	struct path parent;

	dent = kern_path_locked(path_s, &parent);
	if (IS_ERR(dent)) {
		err = PTR_ERR(dent);
		if (err == -EEXIST)
			err = 0;
		goto out_unlock;
	}

	err = vfs_mkdir2(parent.mnt, parent.dentry->d_inode, dent, mode);
	if (err) {
		if (err == -EEXIST)
			err = 0;
		goto out_dput;
	}

	attrs.ia_uid = make_kuid(&init_user_ns, uid);
	attrs.ia_gid = make_kgid(&init_user_ns, gid);
	attrs.ia_valid = ATTR_UID | ATTR_GID;
	mutex_lock(&dent->d_inode->i_mutex);
	notify_change2(parent.mnt, dent, &attrs);
	mutex_unlock(&dent->d_inode->i_mutex);

out_dput:
	dput(dent);

out_unlock:
	/* parent dentry locked by lookup_create */
	mutex_unlock(&parent.dentry->d_inode->i_mutex);
	path_put(&parent);
	return err;
}

/*
 * Return 1, if a disk has enough free space, otherwise 0.
 * We assume that any files can not be overwritten.
 */
static inline int check_min_free_space(struct dentry *dentry, size_t size, int dir)
{
	int err;
	struct path lower_path;
	struct kstatfs statfs;
	u64 avail;
	struct sdcardfs_sb_info *sbi = SDCARDFS_SB(dentry->d_sb);
	
	if (uid_eq(GLOBAL_ROOT_UID, current_fsuid()) ||
			capable(CAP_SYS_RESOURCE) ||
			in_group_p(AID_USE_ROOT_RESERVED) ||
			in_group_p(AID_USE_SEC_RESERVED))
		return 1;

	if (sbi->options.reserved_mb) {
		/* Get fs stat of lower filesystem. */
		sdcardfs_get_lower_path(dentry->d_sb->s_root, &lower_path);
		err = vfs_statfs(&lower_path, &statfs);
		sdcardfs_put_lower_path(dentry->d_sb->s_root, &lower_path);

		if (unlikely(err))
			goto out_invalid;

		/* Invalid statfs informations. */
		if (unlikely(statfs.f_bsize == 0))
			goto out_invalid;

		/* if you are checking directory, set size to f_bsize. */
		if (unlikely(dir))
			size = statfs.f_bsize;

		/* available size */
		avail = statfs.f_bavail * statfs.f_bsize;

		/* not enough space */
		if ((u64)size > avail)
			goto out_nospc;

		/* enough space */
		if ((avail - size) > (sbi->options.reserved_mb * 1024 * 1024))
			return 1;

		goto out_nospc;
	} else
		return 1;

out_invalid:
	pr_info("sdcardfs: statfs error  : %d\n", err);
	pr_info("sdcardfs: f_type        : 0x%X\n", (u32) statfs.f_type);
	pr_info("sdcardfs: f_blocks      : %llu blocks\n", statfs.f_blocks);
	pr_info("sdcardfs: f_bfree       : %llu blocks\n", statfs.f_bfree);
	pr_info("sdcardfs: f_files       : %llu\n", statfs.f_files);
	pr_info("sdcardfs: f_ffree       : %llu\n", statfs.f_ffree);
	pr_info("sdcardfs: f_fsid.val[1] : 0x%X\n", (u32) statfs.f_fsid.val[1]);
	pr_info("sdcardfs: f_fsid.val[0] : 0x%X\n", (u32) statfs.f_fsid.val[0]);
	pr_info("sdcardfs: f_namelen     : %ld\n", statfs.f_namelen);
	pr_info("sdcardfs: f_frsize      : %ld\n", statfs.f_frsize);
	pr_info("sdcardfs: f_flags       : %ld\n", statfs.f_flags);
	pr_info("sdcardfs: reserved_mb   : %u\n", sbi->options.reserved_mb);

out_nospc:
	pr_info_ratelimited("sdcardfs: f_bavail: %llu f_bsize: %ld required: %llu\n",
		statfs.f_bavail, statfs.f_bsize, (u64) size);
	return 0;
}

/*
 * Copies attrs and maintains sdcardfs managed attrs
 * Since our permission check handles all special permissions, set those to be open
 */
static inline void sdcardfs_copy_and_fix_attrs(struct inode *dest, const struct inode *src)
{
	dest->i_mode = (src->i_mode  & S_IFMT) | S_IRWXU | S_IRWXG |
			S_IROTH | S_IXOTH; /* 0775 */
	dest->i_uid = make_kuid(&init_user_ns, SDCARDFS_I(dest)->data->d_uid);
	dest->i_gid = make_kgid(&init_user_ns, AID_SDCARD_RW);
	dest->i_rdev = src->i_rdev;
	dest->i_atime = src->i_atime;
	dest->i_mtime = src->i_mtime;
	dest->i_ctime = src->i_ctime;
	dest->i_blkbits = src->i_blkbits;
	dest->i_flags = src->i_flags;
	set_nlink(dest, src->i_nlink);
}

static inline bool str_case_eq(const char *s1, const char *s2)
{
	return !strcasecmp(s1, s2);
}

static inline bool str_n_case_eq(const char *s1, const char *s2, size_t len)
{
	return !strncasecmp(s1, s2, len);
}

static inline bool qstr_case_eq(const struct qstr *q1, const struct qstr *q2)
{
	return q1->len == q2->len && str_case_eq(q1->name, q2->name);
}

static inline bool qstr_n_case_eq(const struct qstr *q1, const struct qstr *q2)
{
	return q1->len == q2->len && str_n_case_eq(q1->name, q2->name, q1->len);
}

/* */
#define QSTR_LITERAL(string) QSTR_INIT(string, sizeof(string)-1)

#endif	/* not _SDCARDFS_H_ */
